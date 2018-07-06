#include "Headers/SceneManager.h"
#include "Headers/AIManager.h"

#include "SceneList.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/ProfileTimer.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Rendering/Headers/ForwardPlusRenderer.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"

namespace Divide {

namespace {
    // if culled count exceeds this limit, culling becomes multithreaded in the next frame
    const U32 g_asyncCullThreshold = 75;
};

SceneManager::SceneManager()
    : FrameListener(),
      _GUI(nullptr),
      _activeScene(nullptr),
      _renderPassCuller(nullptr),
      _renderPassManager(nullptr),
      _defaultMaterial(nullptr),
      _processInput(false),
      _init(false),
      _elapsedTime(0ULL),
      _elapsedTimeMS(0)

{
    AI::AIManager::createInstance();
}

SceneManager::~SceneManager()
{
    _sceneShaderData->destroy();
    AI::AIManager::destroyInstance();
    Time::REMOVE_TIMER(_sceneGraphCullTimer);
    UNREGISTER_FRAME_LISTENER(&(this->getInstance()));
    Console::printfn(Locale::get("STOP_SCENE_MANAGER"));
    // Console::printfn(Locale::get("SCENE_MANAGER_DELETE"));
    Console::printfn(Locale::get("SCENE_MANAGER_REMOVE_SCENES"));
    MemoryManager::DELETE_HASHMAP(_sceneMap);
    MemoryManager::DELETE(_renderPassCuller);
    _renderer.reset(nullptr);
}

bool SceneManager::init(GUI* const gui) {
    REGISTER_FRAME_LISTENER(&(this->getInstance()), 1);

    // Load default material
    Console::printfn(Locale::get("LOAD_DEFAULT_MATERIAL"));
    _defaultMaterial = XML::loadMaterialXML(
        ParamHandler::getInstance().getParam<stringImpl>("scriptLocation") +
            "/defaultMaterial",
        false);
    _defaultMaterial->dumpToFile(false);

    _GUI = gui;
    _renderPassCuller = MemoryManager_NEW RenderPassCuller();
    _renderPassManager = &RenderPassManager::getInstance();
    _sceneGraphCullTimer = Time::ADD_TIMER("SceneGraph cull timer");
    _sceneShaderData.reset(GFX_DEVICE.newSB("sceneShaderData", 1, false, false, BufferUpdateFrequency::OFTEN));
    _sceneShaderData->create(1, sizeof(SceneShaderData));
    _sceneShaderData->bind(ShaderBufferLocation::SCENE_DATA);
    _sceneData.shadowingSettings(0.0000002f, 0.0002f, 150.0f, 250.0f);

    _init = true;
    return true;
}

bool SceneManager::load(const stringImpl& sceneName,
                        const vec2<U16>& resolution) {
    assert(_init == true && _GUI != nullptr);
    Console::printfn(Locale::get("SCENE_MANAGER_LOAD_SCENE_DATA"));
    XML::loadScene(sceneName, *this);
    if (!_activeScene) {
        return false;
    }

    return Attorney::SceneManager::load(*_activeScene, sceneName, _GUI);
}

Scene* SceneManager::createScene(const stringImpl& name) {
    Scene* scene = nullptr;

    if (!name.empty()) {
        scene = _sceneFactory[name]();
    }

    if (scene != nullptr) {
        hashAlg::emplace(_sceneMap, name, scene);
    }

    return scene;
}

bool SceneManager::unloadCurrentScene() {
    AI::AIManager::getInstance().pauseUpdate(true);
    RemoveResource(_defaultMaterial);
    bool state = Attorney::SceneManager::unload(*_activeScene);
    if (state) {
        state = Attorney::SceneManager::deinitializeAI(*_activeScene);
        _sceneMap.erase(_sceneMap.find(_activeScene->getName()));
        _activeScene.reset(nullptr);
    }
    return state;
}

void SceneManager::initPostLoadState() {
    _processInput = true;
}

bool SceneManager::frameStarted(const FrameEvent& evt) {
    _sceneShaderData->setData(&_sceneData);
    return Attorney::SceneManager::frameStarted(*_activeScene);
}

bool SceneManager::frameEnded(const FrameEvent& evt) {
    return Attorney::SceneManager::frameEnded(*_activeScene);
}

void SceneManager::updateSceneState(const U64 deltaTime) {
    // Update internal timers
    _elapsedTime += deltaTime;
    _elapsedTimeMS = Time::MicrosecondsToMilliseconds<U32>(_elapsedTime);

    ParamHandler& par = ParamHandler::getInstance();
    LightManager& lightMgr = LightManager::getInstance();

    // Shadow splits are only visible in debug builds
    _sceneData.enableDebugRender(par.getParam<bool>("rendering.debug.displayShadowDebugInfo"));
    // Time, fog, ambient light
    _sceneData.elapsedTime(_elapsedTimeMS);

    const vec3<F32>& ambient = lightMgr.getAmbientLight();
    _sceneData.lightDetails(ambient.r, ambient.g, ambient.b, lightMgr.getActiveLightCount());
    _sceneData.toggleShadowMapping(lightMgr.shadowMappingEnabled());

    _sceneData.fogDensity(par.getParam<bool>("rendering.enableFog")
                            ? par.getParam<F32>("rendering.sceneState.fogDensity")
                            : -1.0f);

    const SceneState& activeSceneState = _activeScene->state();
    _sceneData.windDetails(activeSceneState.windDirX(),
                           0.0f,
                           activeSceneState.windDirZ(),
                           activeSceneState.windSpeed());

    _activeScene->updateSceneState(deltaTime);
}

/// Update fog values
void SceneManager::enableFog(F32 density, const vec3<F32>& color) {
    ParamHandler& par = ParamHandler::getInstance();
    par.setParam("rendering.sceneState.fogColor.r", color.r);
    par.setParam("rendering.sceneState.fogColor.g", color.g);
    par.setParam("rendering.sceneState.fogColor.b", color.b);
    par.setParam("rendering.sceneState.fogDensity", density);

    _sceneData.fogDetails(color.r, color.g, color.b,
                          par.getParam<bool>("rendering.enableFog") ? density : -1.0f);
}

const RenderPassCuller::VisibleNodeList&  SceneManager::cullSceneGraph(RenderStage stage) {
    auto cullingFunction = [](const SceneGraphNode& node) -> bool {
        if (node.getNode()->getType() == SceneNodeType::TYPE_OBJECT3D) {
            Object3D::ObjectType type = node.getNode<Object3D>()->getObjectType();
            return (type == Object3D::ObjectType::FLYWEIGHT);
        }
        return false;
    };

    auto meshCullingFunction = [](SceneGraphNode_wptr node) -> bool {
        SceneGraphNode_ptr sgnNode = node.lock();
        if (sgnNode->getNode()->getType() == SceneNodeType::TYPE_OBJECT3D) {
            Object3D::ObjectType type = sgnNode->getNode<Object3D>()->getObjectType();
            return (type == Object3D::ObjectType::MESH);
        }

        return false;
    };

    // If we are rendering a high node count, we might want to use async frustum culling
    bool cullAsync = _renderPassManager->getLastTotalBinSize(stage) > g_asyncCullThreshold;
    _renderPassCuller->frustumCull(GET_ACTIVE_SCENEGRAPH(), 
                                   _activeScene->state(),
                                   stage,
                                   cullAsync,
                                   cullingFunction);
    RenderPassCuller::VisibleNodeList& visibleNodes = _renderPassCuller->getNodeCache(stage);

    visibleNodes.erase(std::remove_if(std::begin(visibleNodes),
                                   std::end(visibleNodes),
                                   meshCullingFunction),
                       std::end(visibleNodes));

    return visibleNodes;
}

void SceneManager::updateVisibleNodes(RenderStage stage, bool refreshNodeData, U32 pass) {
    RenderQueue& queue = _renderPassManager->getQueue();

    RenderPassCuller::VisibleNodeList& visibleNodes = _renderPassCuller->getNodeCache(stage);

    if (refreshNodeData) {
        queue.refresh();
        const vec3<F32>& eyePos = _activeScene->renderState().getCameraConst().getEye();
        for (SceneGraphNode_wptr node : visibleNodes) {
            SceneGraphNode_ptr sgn = node.lock();
            queue.addNodeToQueue(*sgn, eyePos);
        }
    }
    
    queue.sort(stage);
    std::sort(std::begin(visibleNodes), std::end(visibleNodes),
        [](SceneGraphNode_wptr nodeA, SceneGraphNode_wptr nodeB) {
            RenderingComponent* renderableA = nodeA.lock()->getComponent<RenderingComponent>();
            RenderingComponent* renderableB = nodeB.lock()->getComponent<RenderingComponent>();
            return renderableA->drawOrder() < renderableB->drawOrder();
    });

    if (refreshNodeData) {
        getRenderer().preRender();
    }

    GFX_DEVICE.buildDrawCommands(visibleNodes, _activeScene->renderState(), refreshNodeData, pass);
}

void SceneManager::renderVisibleNodes(RenderStage stage, bool refreshNodeData, U32 pass) {
    if (refreshNodeData) {
        Time::ScopedTimer timer(*_sceneGraphCullTimer);
        cullSceneGraph(stage);
    }
    updateVisibleNodes(stage, refreshNodeData, pass);

    RenderStage renderStage = GFX_DEVICE.getRenderStage();
    SceneRenderState& renderState = _activeScene->renderState();
    if (renderState.drawGeometry()) {
        RenderQueue& renderQueue = RenderPassManager::getInstance().getQueue();
        U16 renderBinCount = renderQueue.getRenderQueueBinSize();
        for (U16 i = 0; i < renderBinCount; ++i) {
            renderQueue.getBinSorted(i)->render(renderState, renderStage, refreshNodeData, pass);
        }
    }
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
        _renderer.reset(new ForwardPlusRenderer());
    } break;
    case RendererType::RENDERER_DEFERRED_SHADING: {
        _renderer.reset(new DeferredShadingRenderer());
    } break;
    }
}

void SceneManager::onCameraUpdate(Camera& camera) {
    Attorney::SceneManager::onCameraUpdate(*_activeScene, camera);
}

///--------------------------Input Management-------------------------------------///

bool SceneManager::onKeyDown(const Input::KeyEvent& key) {
    if (!_processInput) {
        return false;
    }
    return _activeScene->input().onKeyDown(key);
}

bool SceneManager::onKeyUp(const Input::KeyEvent& key) {
    if (!_processInput) {
        return false;
    }

    return _activeScene->input().onKeyUp(key);
}

bool SceneManager::mouseMoved(const Input::MouseEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return _activeScene->input().mouseMoved(arg);
}

bool SceneManager::mouseButtonPressed(const Input::MouseEvent& arg,
                                      Input::MouseButton button) {
    if (!_processInput) {
        return false;
    }
    return _activeScene->input().mouseButtonPressed(arg, button);
}

bool SceneManager::mouseButtonReleased(const Input::MouseEvent& arg,
                                       Input::MouseButton button) {
    if (!_processInput) {
        return false;
    }

    return _activeScene->input().mouseButtonReleased(arg, button);
}

bool SceneManager::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    if (!_processInput) {
        return false;
    }
    return _activeScene->input().joystickAxisMoved(arg, axis);
}

bool SceneManager::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    if (!_processInput) {
        return false;
    }
    return _activeScene->input().joystickPovMoved(arg, pov);
}

bool SceneManager::joystickButtonPressed(const Input::JoystickEvent& arg,
                                         Input::JoystickButton button) {
    if (!_processInput) {
        return false;
    }
    return _activeScene->input().joystickButtonPressed(arg, button);
}

bool SceneManager::joystickButtonReleased(const Input::JoystickEvent& arg,
                                          Input::JoystickButton button) {
    if (!_processInput) {
        return false;
    }
    return _activeScene->input().joystickButtonReleased(arg, button);
}

bool SceneManager::joystickSliderMoved(const Input::JoystickEvent& arg,
                                       I8 index) {
    if (!_processInput) {
        return false;
    }
    return _activeScene->input().joystickSliderMoved(arg, index);
}

bool SceneManager::joystickVector3DMoved(const Input::JoystickEvent& arg,
                                         I8 index) {
    if (!_processInput) {
        return false;
    }
    return _activeScene->input().joystickVector3DMoved(arg, index);
}
};