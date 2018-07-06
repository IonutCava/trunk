#include "stdafx.h"

#include "Headers/SceneManager.h"
#include "Headers/RenderPassManager.h"
#include "Headers/FrameListenerManager.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Scenes/Headers/ScenePool.h"
#include "Scenes/Headers/SceneShaderData.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "AI/PathFinding/Headers/DivideRecast.h"

#include "Environment/Water/Headers/Water.h"
#include "Geometry/Importer/Headers/DVDConverter.h"

#include "Dynamics/Entities/Units/Headers/Player.h"

#include "Core/Debugging/Headers/DebugInterface.h"

namespace Divide {

/*
    ToDo:
    - Add IMPrimities per Scene and clear on scene unload
    - Cleanup load flags and use ResourceState enum instead
    - Sort out camera loading/unloading issues with parameteres (position, orientation, etc)
*/

namespace {
    struct VisibleNodesFrontToBack {
        VisibleNodesFrontToBack(const vec3<F32>& camPos) : _camPos(camPos)
        {
        }

        bool operator()(const RenderPassCuller::VisibleNode& a, const RenderPassCuller::VisibleNode& b) const {
            F32 distASQ = a.second->get<BoundsComponent>()->getBoundingSphere().getCenter().distanceSquared(_camPos);
            F32 distBSQ = b.second->get<BoundsComponent>()->getBoundingSphere().getCenter().distanceSquared(_camPos);
            return distASQ < distBSQ;
        }

        const vec3<F32> _camPos;
    };
};

bool SceneManager::onStartup() {
    return Attorney::SceneManager::onStartup();
}

bool SceneManager::onShutdown() {
    return Attorney::SceneManager::onShutdown();
}

SceneManager::SceneManager(Kernel& parentKernel)
    : FrameListener(),
      Input::InputAggregatorInterface(),
      KernelComponent(parentKernel),
      _platformContext(nullptr),
      _resourceCache(nullptr),
      _sceneData(nullptr),
      _renderPassCuller(nullptr),
      _defaultMaterial(nullptr),
      _processInput(false),
      _scenePool(nullptr),
      _init(false),
      _elapsedTime(0ULL),
      _elapsedTimeMS(0),
      _currentPlayerPass(0),
      _saveTimer(0ULL),
      _camUpdateListenerID(0),
      _camChangeListenerID(0)
{
    ADD_FILE_DEBUG_GROUP();
    ADD_DEBUG_VAR_FILE(&_elapsedTime, CallbackParam::TYPE_LARGE_INTEGER, false);
}

SceneManager::~SceneManager()
{
    destroy();
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
        WaitForAllTasks(true, true, false);
    } else {
        while (!_playerAddQueue.empty()) {
            std::pair<Scene*, SceneGraphNode_ptr>& playerToAdd = _playerAddQueue.front();
            addPlayerInternal(*playerToAdd.first, playerToAdd.second);
            _playerAddQueue.pop();
        }
        while (!_playerRemoveQueue.empty()) {
            std::pair<Scene*, Player_ptr>& playerToRemove = _playerRemoveQueue.front();
            removePlayerInternal(*playerToRemove.first, playerToRemove.second);
            _playerRemoveQueue.pop();
        }
        getActiveScene().idle();
    }
}

bool SceneManager::init(PlatformContext& platformContext, ResourceCache& cache) {
    if (_platformContext == nullptr) {
        _platformContext = &platformContext;
        _resourceCache = &cache;
        REGISTER_FRAME_LISTENER(this, 1);

        AI::Navigation::DivideRecast::createInstance();

        _scenePool = MemoryManager_NEW ScenePool(*this);

        for (U8 i = 0; i < to_base(RenderPassType::COUNT); ++i) {
            _sceneGraphCullTimers[i][to_U32(RenderStage::DISPLAY)] = &Time::ADD_TIMER(Util::StringFormat("SceneGraph cull timer: Display [pass: %d]", i).c_str());
            _sceneGraphCullTimers[i][to_U32(RenderStage::REFLECTION)] = &Time::ADD_TIMER(Util::StringFormat("SceneGraph cull timer: Reflection [pass: %d]", i).c_str());
            _sceneGraphCullTimers[i][to_U32(RenderStage::REFRACTION)] = &Time::ADD_TIMER(Util::StringFormat("SceneGraph cull timer: Refraction [pass: %d]", i).c_str());
            _sceneGraphCullTimers[i][to_U32(RenderStage::SHADOW)] = &Time::ADD_TIMER(Util::StringFormat("SceneGraph cull timer: Shadow [pass: %d]", i).c_str());
        }

        // Load default material
        Console::printfn(Locale::get(_ID("LOAD_DEFAULT_MATERIAL")));
        _defaultMaterial = XML::loadMaterialXML(*_platformContext,
                                                Paths::g_xmlDataLocation + "defaultMaterial",
                                                false);
        _defaultMaterial->dumpToFile(false);
        _sceneData = MemoryManager_NEW SceneShaderData(platformContext.gfx());
        _renderPassCuller = MemoryManager_NEW RenderPassCuller();
        _scenePool->init();

        _init = true;
    } else {
        _init = false;
    }
    return _init;
}

void SceneManager::destroy() {
    if (_init) {
        MemoryManager::SAFE_DELETE(_sceneData);
        UNREGISTER_FRAME_LISTENER(this);
        Console::printfn(Locale::get(_ID("STOP_SCENE_MANAGER")));
        // Console::printfn(Locale::get("SCENE_MANAGER_DELETE"));
        Console::printfn(Locale::get(_ID("SCENE_MANAGER_REMOVE_SCENES")));
        MemoryManager::DELETE(_scenePool);
        MemoryManager::DELETE(_renderPassCuller);
        AI::Navigation::DivideRecast::destroyInstance();
        _platformContext = nullptr;
        _defaultMaterial.reset();
        _init = false;
    }
}

Scene* SceneManager::load(stringImpl sceneName) {
    bool foundInCache = false;
    Scene* loadingScene = _scenePool->getOrCreateScene(*_platformContext, parent().resourceCache(), *this, sceneName, foundInCache);

    if (!loadingScene) {
        Console::errorfn(Locale::get(_ID("ERROR_XML_LOAD_INVALID_SCENE")));
        return nullptr;
    }

    ParamHandler::instance().setParam(_ID("currentScene"), sceneName);

    bool state = false;
    bool sceneNotLoaded = loadingScene->getState() != ResourceState::RES_LOADED;

    if (sceneNotLoaded) {
        XML::loadScene(Paths::g_xmlDataLocation + Paths::g_scenesLocation, sceneName, loadingScene, _platformContext->config());
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
        _platformContext->gui().onUnloadScene(scene);
        Attorney::SceneManager::onRemoveActive(*scene);
        return Attorney::SceneManager::unload(*scene);
    }

    return false;
}

void SceneManager::setActiveScene(Scene* const scene) {
    assert(scene != nullptr);

    Attorney::SceneManager::onRemoveActive(_scenePool->defaultSceneActive() ? _scenePool->defaultScene()
                                                                            : getActiveScene());

    _scenePool->activeScene(*scene);
    Attorney::SceneManager::onSetActive(*scene);
    LoadSave::loadScene(*scene);

    ShadowMap::resetShadowMaps(_platformContext->gfx());
    _platformContext->gui().onChangeScene(scene);
    ParamHandler::instance().setParam(_ID("activeScene"), scene->getName());

    if (_camUpdateListenerID != 0) {
        Camera::removeUpdateListener(_camUpdateListenerID);
    }
    _camUpdateListenerID = Camera::addUpdateListener([this](const Camera& cam) {
        onCameraUpdate(cam);
    });

    if (_camChangeListenerID != 0) {
        Camera::removeUpdateListener(_camChangeListenerID);
    }
    _camChangeListenerID = Camera::addChangeListener([this](const Camera& cam) {
        onCameraChange(cam);
    });
}

bool SceneManager::switchScene(const stringImpl& name, bool unloadPrevious, bool threaded) {
    assert(!name.empty());

    Scene* sceneToUnload = nullptr;
    if (!_scenePool->defaultSceneActive()) {
        sceneToUnload = &_scenePool->activeScene();
    }

    CreateTask(
        [this, name, unloadPrevious, &sceneToUnload](const Task& parentTask)
        {
            // Load first, unload after to make sure we don't reload common resources
            if (load(name) != nullptr) {
                if (unloadPrevious && sceneToUnload) {
                    Attorney::SceneManager::onRemoveActive(*sceneToUnload);
                    unloadScene(sceneToUnload);
                }
            }
        },
        [this, name, unloadPrevious, &sceneToUnload]()
        {
            bool foundInCache = false;
            Scene* loadedScene = _scenePool->getOrCreateScene(*_platformContext, parent().resourceCache(), *this, name, foundInCache);
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
            setActiveScene(loadedScene);

            if (unloadPrevious) {
                _scenePool->deleteScene(sceneToUnload);
            }

            _renderPassCuller->clear();

            Time::ApplicationTimer::instance().resetFPSCounter();
            
        })._task->startTask(threaded ? Task::TaskPriority::HIGH
                                     : Task::TaskPriority::REALTIME_WITH_CALLBACK,
                            to_base(Task::TaskFlags::SYNC_WITH_GPU));

    _sceneSwitchTarget.reset();

    return true;
}

vectorImpl<stringImpl> SceneManager::sceneNameList(bool sorted) const {
    return _scenePool->sceneNameList(sorted);
}

void SceneManager::initPostLoadState() {
    _processInput = true;
}

void SceneManager::onChangeResolution(U16 w, U16 h) {
    F32 aspectRatio = to_F32(w) / h;

    if (_init) {
        
        F32 fov = _platformContext->config().runtime.verticalFOV;;
        vec2<F32> zPlanes(_platformContext->config().runtime.zNear, _platformContext->config().runtime.zFar);

        for (const Player_ptr& player : getPlayers()) {
            player->getCamera().setProjection(aspectRatio, fov, zPlanes);
        }

        Camera& baseCam = Attorney::SceneManager::baseCamera(getActiveScene());
        baseCam.setProjection(aspectRatio, fov, zPlanes);
    }
}

void SceneManager::addPlayer(Scene& parentScene, const SceneGraphNode_ptr& playerNode, bool queue) {
    if (queue) {
        _playerAddQueue.push(std::make_pair(&parentScene, playerNode));
    } else {
        addPlayerInternal(parentScene, playerNode);
    }
}

void SceneManager::addPlayerInternal(Scene& parentScene, const SceneGraphNode_ptr& playerNode) {
    I64 sgnGUID = playerNode->getGUID();
    for (const Player_ptr& crtPlayer : _players) {
        if (crtPlayer->getBoundNode().lock()->getGUID() == sgnGUID) {
            return;
        }
    }

    Player_ptr player = std::make_shared<Player>(to_U8(_players.size()));
    player->getCamera().fromCamera(Attorney::SceneManager::baseCamera(parentScene));
    player->getCamera().setFixedYawAxis(true);
    playerNode->get<UnitComponent>()->setUnit(player);

    _players.push_back(player);
    _platformContext->gfx().resizeHistory(to_U8(_players.size()));
     Attorney::SceneManager::onPlayerAdd(parentScene, player);
}

void SceneManager::removePlayer(Scene& parentScene, Player_ptr& player, bool queue) {
    if (queue) {
        _playerRemoveQueue.push(std::make_pair(&parentScene, player));
    } else {
        removePlayerInternal(parentScene, player);
    }
}

void SceneManager::removePlayerInternal(Scene& parentScene, Player_ptr& player) {
    if (player) {
        I64 targetGUID = player->getGUID();
        vectorAlg::vecSize initialSize = _players.size();

        _players.erase(
            std::remove_if(std::begin(_players),
                           std::end(_players),
                           [&targetGUID](const Player_ptr& crtPlayer) {
                               return crtPlayer->getGUID() == targetGUID;
                           }),
            std::end(_players));

        if (initialSize != _players.size()) {
            _platformContext->gfx().resizeHistory(to_U8(_players.size()));
            Attorney::SceneManager::onPlayerRemove(parentScene, player);
        }
        player.reset();
    }
}

const SceneManager::PlayerList& SceneManager::getPlayers() const {
    return _players;
}

bool SceneManager::frameStarted(const FrameEvent& evt) {
    _sceneData->uploadToGPU();
    return Attorney::SceneManager::frameStarted(getActiveScene());
}

bool SceneManager::frameEnded(const FrameEvent& evt) {
    return Attorney::SceneManager::frameEnded(getActiveScene());
}

void SceneManager::onCameraUpdate(const Camera& camera) {
    getActiveScene().sceneGraph().onCameraUpdate(camera);
}

void SceneManager::onCameraChange(const Camera& camera) {
    getActiveScene().sceneGraph().onCameraChange(camera);
}

void SceneManager::updateSceneState(const U64 deltaTime) {
    Scene& activeScene = getActiveScene();
    assert(activeScene.getState() == ResourceState::RES_LOADED);
    // Update internal timers
    _elapsedTime += deltaTime;
    _elapsedTimeMS = Time::MicrosecondsToMilliseconds<U32>(_elapsedTime);

    LightPool* lightPool = Attorney::SceneManager::lightPool(activeScene);

    // Shadow splits are only visible in debug builds
    _sceneData->enableDebugRender(ParamHandler::instance().getParam<bool>(_ID("rendering.debug.displayShadowDebugInfo")));
    // Time, fog, etc
    _sceneData->elapsedTime(_elapsedTimeMS);
    _sceneData->deltaTime(Time::MicrosecondsToSeconds<F32>(deltaTime));
    _sceneData->lightCount(LightType::DIRECTIONAL, lightPool->getActiveLightCount(LightType::DIRECTIONAL));
    _sceneData->lightCount(LightType::POINT, lightPool->getActiveLightCount(LightType::POINT));
    _sceneData->lightCount(LightType::SPOT, lightPool->getActiveLightCount(LightType::SPOT));

    _sceneData->setRendererFlag(_platformContext->gfx().getRenderer().getFlag());
    _sceneData->detailLevel(_platformContext->gfx().renderDetailLevel(), _platformContext->gfx().shadowDetailLevel());
    _sceneData->toggleShadowMapping(_platformContext->gfx().shadowDetailLevel() != RenderDetailLevel::OFF);


    FogDescriptor& fog = activeScene.state().fogDescriptor();
    bool fogEnabled = _platformContext->config().rendering.enableFog;
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

    const vectorImpl<SceneGraphNode_wptr>& waterBodies = activeScene.sceneGraph().getNodesByType(SceneNodeType::TYPE_WATER);
    if (!waterBodies.empty()) {
        U8 index = 0;
        for (SceneGraphNode_wptr body : waterBodies) {
            const SceneGraphNode_ptr water(body.lock());
            
            _sceneData->waterDetails(index,
                                     water->get<PhysicsComponent>()->getPosition(),
                                     water->getNode<WaterPlane>()->getDimensions());
            index++;
            break;//<- temp
        }
    }

    activeScene.updateSceneState(deltaTime);

    _saveTimer += deltaTime;

    if (_saveTimer >= Time::SecondsToMicroseconds(5)) {
        LoadSave::saveScene(activeScene);
        _saveTimer = 0ULL;
    }
}

void SceneManager::preRender(const Camera& camera, RenderTarget& target) {
    const GFXDevice& gfx = _platformContext->gfx();

    LightPool* lightPool = Attorney::SceneManager::lightPool(getActiveScene());
    gfx.getRenderer().preRender(target, *lightPool);

    if (gfx.getRenderStage().stage() == RenderStage::DISPLAY) {
        PostFX::instance().cacheDisplaySettings(gfx);
    }
}

void SceneManager::postRender(const Camera& camera, RenderSubPassCmds& subPassesInOut) {
    Scene& activeScene = getActiveScene();
    SceneRenderState& activeSceneRenderState = activeScene.renderState();

    const RenderStagePass& stagePass = _platformContext->gfx().getRenderStage();
    parent().renderPassManager().getQueue().postRender(activeSceneRenderState, stagePass, subPassesInOut);
    Attorney::SceneManager::debugDraw(activeScene, camera, stagePass, subPassesInOut);
    // Draw bounding boxes, skeletons, axis gizmo, etc.
    _platformContext->gfx().debugDraw(activeSceneRenderState, camera, subPassesInOut);
}

bool SceneManager::generateShadowMaps() {
    Scene& activeScene = getActiveScene();
    LightPool* lightPool = Attorney::SceneManager::lightPool(activeScene);
    assert(lightPool != nullptr);
    return lightPool->generateShadowMaps(activeScene.renderState());
}

Camera* SceneManager::getActiveCamera() const {
    Camera* overrideCamera = getActiveScene().state().playerState(_currentPlayerPass).overrideCamera();
    if (overrideCamera == nullptr) {
        overrideCamera = &getPlayers().at(_currentPlayerPass)->getCamera();
    }

    return overrideCamera;
}

void SceneManager::currentPlayerPass(U8 playerIndex) {
    _currentPlayerPass = playerIndex;
    _platformContext->gfx().historyIndex(playerIndex, true);
    Camera& playerCam = getPlayers().at(_currentPlayerPass)->getCamera();
    _platformContext->gfx().setSceneZPlanes(playerCam.getZPlanes());
    Camera::activeCamera(&playerCam);
    Attorney::SceneManager::currentPlayerPass(getActiveScene(), playerIndex);
}

const RenderPassCuller::VisibleNodeList&
SceneManager::getSortedCulledNodes(const std::function<bool(const RenderPassCuller::VisibleNode&)>& cullingFunction) {
    const vec3<F32>& camPos = Camera::activeCamera()->getEye();

    // Get list of nodes in view from the previous frame
    RenderPassCuller::VisibleNodeList& nodeCache = getVisibleNodesCache(RenderStage::DISPLAY);

    RenderPassCuller::VisibleNodeList waterNodes;
    _tempNodesCache.resize(0);
    _tempNodesCache.insert(std::begin(_tempNodesCache), std::cbegin(nodeCache), std::cend(nodeCache));

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
        const SceneGraphNode* sgnNode = node.second;
        SceneNodeType type = sgnNode->getNode()->getType();
        if (type != SceneNodeType::TYPE_OBJECT3D &&
            type != SceneNodeType::TYPE_WATER) {
            return true;
        } else {
            if (type == SceneNodeType::TYPE_OBJECT3D) {
                if (sgnNode->getNode<Object3D>()->getObjectType() == Object3D::ObjectType::FLYWEIGHT) {
                    return true;
                } else {
                    if (sgnNode->getNode<Object3D>()->getObjectType() == Object3D::ObjectType::TERRAIN) {
                        return true;
                    }

                    return sgnNode->get<RenderingComponent>()->getMaterialInstance() == nullptr;
                }
            }
        }
        // Enable just for water nodes for now (we should flag mirrors somehow):
        return type != SceneNodeType::TYPE_WATER;
    };

    return getSortedCulledNodes(cullingFunction);
}

const RenderPassCuller::VisibleNodeList& SceneManager::getSortedRefractiveNodes() {
    auto cullingFunction = [](const RenderPassCuller::VisibleNode& node) -> bool {
        const SceneGraphNode* sgnNode = node.second;
        SceneNodeType type = sgnNode->getNode()->getType();
        if (type != SceneNodeType::TYPE_OBJECT3D && type != SceneNodeType::TYPE_WATER) {
            return true;
        } else {
            if (type == SceneNodeType::TYPE_OBJECT3D) {
                return sgnNode->getNode<Object3D>()->getObjectType() == Object3D::ObjectType::FLYWEIGHT;
            }
        }
        //if (not refractive) {
        //    return true;
        //}
        // Enable just for water nodes for now:
        return type != SceneNodeType::TYPE_WATER;
    };

    return getSortedCulledNodes(cullingFunction);
}


const RenderPassCuller::VisibleNodeList& SceneManager::cullSceneGraph(const RenderStagePass& stage) {
    Scene& activeScene = getActiveScene();

    auto cullingFunction = [](const SceneGraphNode& node) -> bool {
        if (node.getNode()->getType() == SceneNodeType::TYPE_OBJECT3D) {
            Object3D::ObjectType type = node.getNode<Object3D>()->getObjectType();
            return (type == Object3D::ObjectType::FLYWEIGHT);
        }

        return node.getNode()->getType() == SceneNodeType::TYPE_TRANSFORM;
    };

    auto meshCullingFunction = [](const RenderPassCuller::VisibleNode& node) -> bool {
        const SceneGraphNode* sgnNode = node.second;
        if (sgnNode->getNode()->getType() == SceneNodeType::TYPE_OBJECT3D) {
            Object3D::ObjectType type = sgnNode->getNode<Object3D>()->getObjectType();
            return (type == Object3D::ObjectType::MESH);
        }
        return false;
    };

    auto shadowCullingFunction = [](const RenderPassCuller::VisibleNode& node) -> bool {
        SceneNodeType type = node.second->getNode()->getType();
        return type == SceneNodeType::TYPE_LIGHT || type == SceneNodeType::TYPE_TRIGGER;
    };

    _renderPassCuller->frustumCull(activeScene.sceneGraph(),
                                   activeScene.state(),
                                   stage.stage(),
                                   cullingFunction);
    RenderPassCuller::VisibleNodeList& visibleNodes = _renderPassCuller->getNodeCache(stage.stage());

    visibleNodes.erase(std::remove_if(std::begin(visibleNodes),
                                      std::end(visibleNodes),
                                      meshCullingFunction),
                       std::end(visibleNodes));

    if (stage.stage() == RenderStage::SHADOW) {
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

void SceneManager::updateVisibleNodes(const RenderStagePass& stage, bool refreshNodeData, U32 pass) {
    RenderQueue& queue = parent().renderPassManager().getQueue();

    RenderPassCuller::VisibleNodeList& visibleNodes = _renderPassCuller->getNodeCache(stage.stage());

    if (refreshNodeData) {
        queue.refresh();
        const vec3<F32>& eyePos = Camera::activeCamera()->getEye();
        for (RenderPassCuller::VisibleNode& node : visibleNodes) {
            queue.addNodeToQueue(*node.second, stage, eyePos);
        }
    }
    
    queue.sort();

    auto setOrder = [&visibleNodes](const Task& parentTask, U32 start, U32 end) { 
        for (U32 i = start; i < end; ++i) {
            RenderPassCuller::VisibleNode& node = visibleNodes[i];
            node.first = node.second->get<RenderingComponent>()->drawOrder();
        }
    };

    parallel_for(setOrder, to_U32(visibleNodes.size()), 10);

    std::sort(std::begin(visibleNodes), std::end(visibleNodes),
        [](const RenderPassCuller::VisibleNode& nodeA,
           const RenderPassCuller::VisibleNode& nodeB)
        {
            return nodeA.first < nodeB.first;
        }
    );

    RenderPass::BufferData& bufferData = parent().renderPassManager().getBufferData(stage.stage(), pass);

    _platformContext->gfx().buildDrawCommands(visibleNodes, getActiveScene().renderState(), bufferData, refreshNodeData);
}

bool SceneManager::populateRenderQueue(const Camera& camera,
                                       bool doCulling,
                                       U32 passIndex) {

    const RenderStagePass& stage = _platformContext->gfx().getRenderStage();

    if (stage.pass() != RenderPassType::DEPTH_PASS) {
        LightPool* lightPool = Attorney::SceneManager::lightPool(getActiveScene());
        lightPool->prepareLightData(camera.getEye(), camera.getViewMatrix());
    }

    if (doCulling) {
        Time::ScopedTimer timer(*_sceneGraphCullTimers[to_U32(stage.pass())][to_U32(stage.stage())]);
        cullSceneGraph(stage);
    }

    updateVisibleNodes(stage, doCulling, passIndex);

    RenderQueue& queue = parent().renderPassManager().getQueue();
    if (getActiveScene().renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY)) {
        queue.populateRenderQueues(stage);
    }

    return queue.getRenderQueueStackSize() > 0;

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

    stringImpl path = Paths::g_saveLocation +  sceneName;
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
    } else {
        src.close();
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
    stringImpl path = Paths::g_saveLocation + sceneName;
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

bool SceneManager::networkUpdate(U32 frameCount) {

    getActiveScene().sceneGraph();

    return true;
}

};