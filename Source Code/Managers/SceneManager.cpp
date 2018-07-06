#include "Headers/SceneManager.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/ParamHandler.h"
#include "Scenes/Headers/ScenePool.h"
#include "Scenes/Headers/SceneShaderData.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Rendering/Headers/ForwardPlusRenderer.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"
#include "AI/PathFinding/Headers/DivideRecast.h"

#include "Core/Debugging/Headers/DebugInterface.h"
namespace Divide {

/*
    ToDo:
    - Add IMPrimities per Scene and clear on scene unload
    - Cleanup load flags and use ResourceState enum instead
    - Sort out camera loading/unloading issues with parameteres (position, orientation, etc)
*/

namespace {
    // if culled count exceeds this limit, culling becomes multithreaded in the next frame
    const U32 g_asyncCullThreshold = 75;

    struct VisibleNodesFrontToBack {
        VisibleNodesFrontToBack(const vec3<F32>& camPos) : _camPos(camPos)
        {
        }

        bool operator()(const RenderPassCuller::VisibleNode& a, const RenderPassCuller::VisibleNode& b) const {
            F32 distASQ = a.second.lock()->get<BoundsComponent>()->getBoundingSphere()
                .getCenter().distanceSquared(_camPos);
            F32 distBSQ = b.second.lock()->get<BoundsComponent>()->getBoundingSphere()
                .getCenter().distanceSquared(_camPos);
            return distASQ < distBSQ;
        }

        const vec3<F32> _camPos;
    };
};

bool SceneManager::initStaticData() {
    return Attorney::SceneManager::initStaticData();
}

SceneManager::SceneManager()
    : FrameListener(),
      _GUI(nullptr),
      _renderer(nullptr),
      _renderPassCuller(nullptr),
      _renderPassManager(nullptr),
      _defaultMaterial(nullptr),
      _processInput(false),
      _scenePool(nullptr),
      _init(false),
      _elapsedTime(0ULL),
      _elapsedTimeMS(0),
      _saveTimer(0ULL),
      _sceneGraphCullTimer(Time::ADD_TIMER("SceneGraph cull timer"))
{
    ADD_FILE_DEBUG_GROUP();
    ADD_DEBUG_VAR_FILE(&_elapsedTime, CallbackParam::TYPE_LARGE_INTEGER, false);
    AI::Navigation::DivideRecast::createInstance();

    _sceneData = MemoryManager_NEW SceneShaderData();
    _scenePool = MemoryManager_NEW ScenePool(*this);
}

SceneManager::~SceneManager()
{
    UNREGISTER_FRAME_LISTENER(&(this->instance()));
    Console::printfn(Locale::get(_ID("STOP_SCENE_MANAGER")));
    // Console::printfn(Locale::get("SCENE_MANAGER_DELETE"));
    Console::printfn(Locale::get(_ID("SCENE_MANAGER_REMOVE_SCENES")));
    MemoryManager::DELETE(_scenePool);
    MemoryManager::DELETE(_sceneData);
    MemoryManager::DELETE(_renderPassCuller);
    MemoryManager::DELETE(_renderer);
    AI::Navigation::DivideRecast::destroyInstance();
}

Scene& SceneManager::getActiveScene() {
    return _scenePool->activeScene();
}

const Scene& SceneManager::getActiveScene() const {
    return _scenePool->activeScene();
}

void SceneManager::idle() {
    if (_sceneSwitchTarget.isSet()) {
        switchScene(_sceneSwitchTarget.targetSceneName(),
                    _sceneSwitchTarget.unloadPreviousScene(),
                    _sceneSwitchTarget.loadInSeparateThread());
        WaitForAllTasks(true, true);
    } else {
        getActiveScene().idle();
    }
}

bool SceneManager::init(GUI* const gui) {
    REGISTER_FRAME_LISTENER(&(this->instance()), 1);

    // Load default material
    Console::printfn(Locale::get(_ID("LOAD_DEFAULT_MATERIAL")));
    _defaultMaterial = XML::loadMaterialXML(
        ParamHandler::instance().getParam<stringImpl>(_ID("scriptLocation")) +
            "/defaultMaterial",
        false);
    _defaultMaterial->dumpToFile(false);

    _GUI = gui;
    _renderPassCuller = MemoryManager_NEW RenderPassCuller();
    _renderPassManager = &RenderPassManager::instance();
    _sceneData->init();
    _scenePool->init();
    _init = true;
    return true;
}

Scene* SceneManager::load(stringImpl sceneName) {
    bool foundInCache = false;
    Scene* loadingScene = _scenePool->getOrCreateScene(sceneName, foundInCache);

    if (!loadingScene) {
        Console::errorfn(Locale::get(_ID("ERROR_XML_LOAD_INVALID_SCENE")));
        return nullptr;
    }

    ParamHandler::instance().setParam(_ID("currentScene"), sceneName);

    bool state = false;
    bool sceneNotLoaded = loadingScene->getState() != ResourceState::RES_LOADED;

    if (sceneNotLoaded) {
        XML::loadScene(sceneName, loadingScene);
        state = Attorney::SceneManager::load(*loadingScene, sceneName);
        if (state) {
            Attorney::SceneManager::postLoad(*loadingScene);
        }
    } else {
        state = true;
    }

    return state ? loadingScene : nullptr;
}

bool SceneManager::unloadScene(Scene* scene) {
    assert(scene != nullptr);
    
    if (Attorney::SceneManager::deinitializeAI(*scene)) {
        _GUI->onUnloadScene(scene);
        return Attorney::SceneManager::unload(*scene);
    }

    return false;
}

void SceneManager::setActiveScene(Scene* const scene) {
    assert(scene != nullptr);

    if (!_scenePool->defaultSceneActive()) {
        Attorney::SceneManager::onRemoveActive(getActiveScene());
    }
    Attorney::SceneManager::onSetActive(*scene);
    _scenePool->activeScene(*scene);
    ShadowMap::resetShadowMaps();
    _GUI->onChangeScene(scene);
    ParamHandler::instance().setParam(_ID("activeScene"), scene->getName());
}

bool SceneManager::switchScene(const stringImpl& name, bool unloadPrevious, bool threaded) {
    assert(!name.empty());

    Scene* sceneToUnload = nullptr;
    if (!_scenePool->defaultSceneActive()) {
        sceneToUnload = &_scenePool->activeScene();
    }

    CreateTask(
        [this, name, unloadPrevious, &sceneToUnload](const std::atomic_bool& stopRequested)
        {
            // Load first, unload after to make sure we don't reload common resources
            if (load(name) != nullptr) {
                if (unloadPrevious && sceneToUnload) {
                    Attorney::SceneManager::onRemoveActive(*sceneToUnload);
                    unloadScene(sceneToUnload);
                }
            }
        },
        [this, name]()
        {
            bool foundInCache = false;
            Scene* loadedScene = _scenePool->getOrCreateScene(name, foundInCache);
            assert(loadedScene != nullptr && foundInCache);
            if(loadedScene->getState() == ResourceState::RES_LOADING) {
                Attorney::SceneManager::postLoadMainThread(*loadedScene);
                if (loadedScene->getGUID() != _scenePool->defaultScene().getGUID())
                {
                    SceneGUIElements* gui = Attorney::SceneManager::gui(*loadedScene);
                    gui->addButton(_ID_RT("Back"),
                        "Back",
                        vec2<I32>(15, 15),
                        vec2<U32>(50, 25),
                        [this](I64 btnGUID)
                    {
                        _sceneSwitchTarget.set(_scenePool->defaultScene().getName(), true, false);
                    });
                }
            }
            assert(loadedScene->getState() == ResourceState::RES_LOADED);
            LoadSave::loadScene(*loadedScene);
            setActiveScene(loadedScene);

            _renderPassCuller->clear();
            
        })._task->startTask(threaded ? Task::TaskPriority::HIGH
                                     : Task::TaskPriority::REALTIME_WITH_CALLBACK,
                            to_const_uint(Task::TaskFlags::SYNC_WITH_GPU));

    _sceneSwitchTarget.reset();

    return true;
}

vectorImpl<stringImpl> SceneManager::sceneNameList(bool sorted) const {
    return _scenePool->sceneNameList(sorted);
}

void SceneManager::initPostLoadState() {
    _processInput = true;
}

bool SceneManager::frameStarted(const FrameEvent& evt) {
    _sceneData->uploadToGPU();
    return Attorney::SceneManager::frameStarted(getActiveScene());
}

bool SceneManager::frameEnded(const FrameEvent& evt) {
    return Attorney::SceneManager::frameEnded(getActiveScene());
}

void SceneManager::onCameraUpdate(Camera& camera) {
    getActiveScene().sceneGraph().onCameraUpdate(camera);
}

void SceneManager::updateSceneState(const U64 deltaTime) {
    Scene& activeScene = getActiveScene();
    assert(activeScene.getState() == ResourceState::RES_LOADED);
    // Update internal timers
    _elapsedTime += deltaTime;
    _elapsedTimeMS = Time::MicrosecondsToMilliseconds<U32>(_elapsedTime);

    ParamHandler& par = ParamHandler::instance();
    LightPool* lightPool = Attorney::SceneManager::lightPool(activeScene);

    // Shadow splits are only visible in debug builds
    _sceneData->enableDebugRender(par.getParam<bool>(_ID("rendering.debug.displayShadowDebugInfo")));
    // Time, fog, etc
    _sceneData->elapsedTime(_elapsedTimeMS);
    _sceneData->deltaTime(Time::MicrosecondsToSeconds<F32>(deltaTime));
    _sceneData->lightCount(LightType::DIRECTIONAL, lightPool->getActiveLightCount(LightType::DIRECTIONAL));
    _sceneData->lightCount(LightType::POINT, lightPool->getActiveLightCount(LightType::POINT));
    _sceneData->lightCount(LightType::SPOT, lightPool->getActiveLightCount(LightType::SPOT));

    _sceneData->setRendererFlag(getRenderer().getFlag());

    _sceneData->toggleShadowMapping(lightPool->shadowMappingEnabled());

    FogDescriptor& fog = activeScene.state().fogDescriptor();
    bool fogEnabled = par.getParam<bool>(_ID("rendering.enableFog"));
    if (fog.dirty() || fogEnabled != fog.active()) {
        const vec3<F32>& colour = fog.colour();
        F32 density = fogEnabled ? fog.density() : 0.0f;
        _sceneData->fogDetails(colour.r, colour.g, colour.b, density);
        fog.clean();
        fog.active(fogEnabled);
    }

    const SceneState& activeSceneState = activeScene.state();
    _sceneData->windDetails(activeSceneState.windDirX(),
                            0.0f,
                            activeSceneState.windDirZ(),
                            activeSceneState.windSpeed());

    activeScene.updateSceneState(deltaTime);

    _saveTimer += deltaTime;

    if (_saveTimer >= Time::SecondsToMicroseconds(5)) {
        LoadSave::saveScene(activeScene);
        _saveTimer = 0ULL;
    }
}

void SceneManager::preRender() {
    Scene& activeScene = getActiveScene();
    LightPool* lightPool = Attorney::SceneManager::lightPool(activeScene);
    lightPool->updateAndUploadLightData(activeScene.renderState().getCameraConst().getEye(), GFX_DEVICE.getMatrix(MATRIX::VIEW));
    getRenderer().preRender(*lightPool);
    PostFX::instance().cacheDisplaySettings(GFX_DEVICE);
}

bool SceneManager::generateShadowMaps() {
    Scene& activeScene = getActiveScene();
    LightPool* lightPool = Attorney::SceneManager::lightPool(activeScene);
    assert(lightPool != nullptr);
    return lightPool->generateShadowMaps(activeScene.renderState());
}

const RenderPassCuller::VisibleNodeList&
SceneManager::getSortedCulledNodes(std::function<bool(const RenderPassCuller::VisibleNode&)> cullingFunction) {
    const SceneRenderState& renderState = getActiveScene().state().renderState();
    const vec3<F32>& camPos = renderState.getCameraConst().getEye();

    // Get list of nodes in view from the previous frame
    RenderPassCuller::VisibleNodeList& nodeCache = getVisibleNodesCache(RenderStage::Z_PRE_PASS);

    RenderPassCuller::VisibleNodeList waterNodes;
    _tempNodesCache.resize(0);
    _tempNodesCache.insert(std::begin(_tempNodesCache), std::begin(nodeCache), std::end(nodeCache));

    // Cull nodes that are not valid reflection targets
    _tempNodesCache.erase(std::remove_if(std::begin(_tempNodesCache),
                                         std::end(_tempNodesCache),
                                         cullingFunction),
                                         std::end(_tempNodesCache));

    // Sort the nodes from front to back
    std::sort(std::begin(_tempNodesCache),
              std::end(_tempNodesCache),
              VisibleNodesFrontToBack(camPos));

    return _tempNodesCache;
}

const RenderPassCuller::VisibleNodeList& SceneManager::getSortedReflectiveNodes() {
    auto cullingFunction = [](const RenderPassCuller::VisibleNode& node) -> bool {
        SceneGraphNode_cptr sgnNode = node.second.lock();
        if (sgnNode->getNode()->getType() != SceneNodeType::TYPE_OBJECT3D &&
            sgnNode->getNode()->getType() != SceneNodeType::TYPE_WATER) {
            return true;
        } else {
            if (sgnNode->getNode()->getType() == SceneNodeType::TYPE_OBJECT3D) {
                return sgnNode->getNode<Object3D>()->getObjectType() == Object3D::ObjectType::FLYWEIGHT;
            }
        }
        //if (not reflective) {
        //    return true;
        //}

        return false;
    };

    return getSortedCulledNodes(cullingFunction);
}

const RenderPassCuller::VisibleNodeList& SceneManager::getSortedRefractiveNodes() {
    auto cullingFunction = [](const RenderPassCuller::VisibleNode& node) -> bool {
        SceneGraphNode_cptr sgnNode = node.second.lock();
        if (sgnNode->getNode()->getType() != SceneNodeType::TYPE_OBJECT3D &&
            sgnNode->getNode()->getType() != SceneNodeType::TYPE_WATER) {
            return true;
        }
        else {
            if (sgnNode->getNode()->getType() == SceneNodeType::TYPE_OBJECT3D) {
                return sgnNode->getNode<Object3D>()->getObjectType() == Object3D::ObjectType::FLYWEIGHT;
            }
        }
        //if (not refractive) {
        //    return true;
        //}

        return false;
    };

    return getSortedCulledNodes(cullingFunction);
}


const RenderPassCuller::VisibleNodeList& SceneManager::cullSceneGraph(RenderStage stage) {
    Scene& activeScene = getActiveScene();

    auto cullingFunction = [](const SceneGraphNode& node) -> bool {
        if (node.getNode()->getType() == SceneNodeType::TYPE_OBJECT3D) {
            Object3D::ObjectType type = node.getNode<Object3D>()->getObjectType();
            return (type == Object3D::ObjectType::FLYWEIGHT);
        }
        return false;
    };

    auto meshCullingFunction = [](const RenderPassCuller::VisibleNode& node) -> bool {
        SceneGraphNode_cptr sgnNode = node.second.lock();
        if (sgnNode->getNode()->getType() == SceneNodeType::TYPE_OBJECT3D) {
            Object3D::ObjectType type = sgnNode->getNode<Object3D>()->getObjectType();
            return (type == Object3D::ObjectType::MESH);
        }
        return false;
    };

    auto shadowCullingFunction = [](const RenderPassCuller::VisibleNode& node) -> bool {
        SceneNodeType type = node.second.lock()->getNode()->getType();
        return type == SceneNodeType::TYPE_LIGHT || type == SceneNodeType::TYPE_TRIGGER;
    };

    // If we are rendering a high node count, we might want to use async frustum culling
    bool cullAsync = _renderPassManager->getLastTotalBinSize(stage) > g_asyncCullThreshold;
    _renderPassCuller->frustumCull(activeScene.sceneGraph(),
                                   activeScene.state(),
                                   stage,
                                   cullAsync,
                                   cullingFunction);
    RenderPassCuller::VisibleNodeList& visibleNodes = _renderPassCuller->getNodeCache(stage);

    visibleNodes.erase(std::remove_if(std::begin(visibleNodes),
                                      std::end(visibleNodes),
                                      meshCullingFunction),
                       std::end(visibleNodes));

    if (stage == RenderStage::SHADOW) {
        visibleNodes.erase(std::remove_if(std::begin(visibleNodes),
                                          std::end(visibleNodes),
                                          shadowCullingFunction),
                           std::end(visibleNodes));
        
    }

    return visibleNodes;
}

RenderPassCuller::VisibleNodeList& 
SceneManager::getVisibleNodesCache(RenderStage stage) {
    return _renderPassCuller->getNodeCache(stage);
}

void SceneManager::updateVisibleNodes(RenderStage stage, bool refreshNodeData, U32 pass) {
    Scene& activeScene = getActiveScene();

    RenderQueue& queue = _renderPassManager->getQueue();

    RenderPassCuller::VisibleNodeList& visibleNodes = _renderPassCuller->getNodeCache(stage);

    if (refreshNodeData) {
        queue.refresh();
        const vec3<F32>& eyePos = activeScene.renderState().getCameraConst().getEye();
        for (RenderPassCuller::VisibleNode& node : visibleNodes) {
            queue.addNodeToQueue(*node.second.lock(), stage, eyePos);
        }
    }
    
    queue.sort(stage);
    for (RenderPassCuller::VisibleNode& node : visibleNodes) {
        node.first = node.second.lock()->get<RenderingComponent>()->drawOrder();
    }

    std::sort(std::begin(visibleNodes), std::end(visibleNodes),
        [](const RenderPassCuller::VisibleNode& nodeA,
           const RenderPassCuller::VisibleNode& nodeB)
        {
            return nodeA.first < nodeB.first;
        }
    );

    SceneRenderState& renderState = activeScene.renderState();
    RenderPass::BufferData& bufferData = 
        RenderPassManager::instance().getBufferData(stage, renderState.currentStagePass(), pass);

    GFX_DEVICE.buildDrawCommands(visibleNodes, renderState, bufferData, refreshNodeData);
}

void SceneManager::renderVisibleNodes(RenderStage stage, bool refreshNodeData, U32 pass) {
    if (refreshNodeData) {
        Time::ScopedTimer timer(_sceneGraphCullTimer);
        cullSceneGraph(stage);
    }

    updateVisibleNodes(stage, refreshNodeData, pass);

    if (getActiveScene().renderState().drawGeometry()) {
        RenderPassManager::instance().getQueue().populateRenderQueues(stage);
    }

    GFX_DEVICE.flushRenderQueues();
}

Renderer& SceneManager::getRenderer() const {
    DIVIDE_ASSERT(_renderer != nullptr,
        "SceneManager error: Renderer requested but not created!");
    return *_renderer;
}

void SceneManager::setRenderer(RendererType rendererType) {
    DIVIDE_ASSERT(rendererType != RendererType::COUNT,
        "SceneManager error: Tried to create an invalid renderer!");

    switch (rendererType) {
    case RendererType::RENDERER_FORWARD_PLUS: {
        MemoryManager::SAFE_UPDATE(_renderer, MemoryManager_NEW ForwardPlusRenderer());
    } break;
    case RendererType::RENDERER_DEFERRED_SHADING: {
        MemoryManager::SAFE_UPDATE(_renderer, MemoryManager_NEW DeferredShadingRenderer());
    } break;
    }
}

void SceneManager::onLostFocus() {
    getActiveScene().onLostFocus();
}
///--------------------------Input Management-------------------------------------///

bool SceneManager::onKeyDown(const Input::KeyEvent& key) {
    if (!_processInput) {
        return false;
    }
    return getActiveScene().input().onKeyDown(key);
}

bool SceneManager::onKeyUp(const Input::KeyEvent& key) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().onKeyUp(key);
}

bool SceneManager::mouseMoved(const Input::MouseEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().mouseMoved(arg);
}

bool SceneManager::mouseButtonPressed(const Input::MouseEvent& arg,
                                      Input::MouseButton button) {
    if (!_processInput) {
        return false;
    }
    return getActiveScene().input().mouseButtonPressed(arg, button);
}

bool SceneManager::mouseButtonReleased(const Input::MouseEvent& arg,
                                       Input::MouseButton button) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().mouseButtonReleased(arg, button);
}

bool SceneManager::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    if (!_processInput) {
        return false;
    }
    return getActiveScene().input().joystickAxisMoved(arg, axis);
}

bool SceneManager::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    if (!_processInput) {
        return false;
    }
    return getActiveScene().input().joystickPovMoved(arg, pov);
}

bool SceneManager::joystickButtonPressed(const Input::JoystickEvent& arg,
                                         Input::JoystickButton button) {
    if (!_processInput) {
        return false;
    }
    return getActiveScene().input().joystickButtonPressed(arg, button);
}

bool SceneManager::joystickButtonReleased(const Input::JoystickEvent& arg,
                                          Input::JoystickButton button) {
    if (!_processInput) {
        return false;
    }
    return getActiveScene().input().joystickButtonReleased(arg, button);
}

bool SceneManager::joystickSliderMoved(const Input::JoystickEvent& arg,
                                       I8 index) {
    if (!_processInput) {
        return false;
    }
    return getActiveScene().input().joystickSliderMoved(arg, index);
}

bool SceneManager::joystickVector3DMoved(const Input::JoystickEvent& arg,
                                         I8 index) {
    if (!_processInput) {
        return false;
    }
    return getActiveScene().input().joystickVector3DMoved(arg, index);
}

bool LoadSave::loadScene(Scene& activeScene) {
    if (activeScene.state().saveLoadDisabled()) {
        return true;
    }

    const stringImpl& sceneName = activeScene.getName();

    stringImpl path = Util::StringFormat("SaveData/%s_", sceneName.c_str());
    stringImpl savePath = path + "current_save.sav";
    stringImpl bakSavePath = path + "save.bak";

    std::ifstream src(savePath.c_str(), std::ios::binary);
    if (src.eof() || src.fail())
    {
        src.close();
        std::ifstream srcBak(bakSavePath.c_str(), std::ios::binary);
        if(srcBak.eof() || srcBak.fail()) {
            srcBak.close();
            return true;
        } else {
            std::ofstream dst(savePath.c_str(), std::ios::binary);
            dst << srcBak.rdbuf();
            dst.close();
            srcBak.close();
        }
    }

    ByteBuffer save;
    save.loadFromFile(savePath);

    return Attorney::SceneLoadSave::load(activeScene, save);
}

bool LoadSave::saveScene(const Scene& activeScene) {
    if (activeScene.state().saveLoadDisabled()) {
        return true;
    }

    const stringImpl& sceneName = activeScene.getName();
    stringImpl path = Util::StringFormat("SaveData/%s_", sceneName.c_str());
    stringImpl savePath = path + "current_save.sav";
    stringImpl bakSavePath = path + "save.bak";

    std::ifstream src(savePath.c_str(), std::ios::binary);
    if (!src.eof() && !src.fail())
    {
        std::ofstream dst(bakSavePath.c_str(), std::ios::out | std::ios::binary);
        dst.clear();
        dst << src.rdbuf();
        dst.close();
    }
    src.close();

    ByteBuffer save;
    if (Attorney::SceneLoadSave::save(activeScene, save)) {
        return save.dumpToFile(savePath);
    }

    return false;
}

};