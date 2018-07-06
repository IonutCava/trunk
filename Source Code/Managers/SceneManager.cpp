#include "stdafx.h"

#include "Headers/SceneManager.h"
#include "Headers/RenderPassManager.h"
#include "Headers/FrameListenerManager.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIButton.h"
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
      _activePlayerCount(0),
      _currentPlayerPass(0),
      _saveTimer(0ULL),
      _camUpdateListenerID(0)
{
    I64 group = _parent.platformContext().debug().addDebugGroup("SceneManager", -1);

    DebugInterface::DebugVarDescriptor elapsedTimeDescriptor;
    elapsedTimeDescriptor._displayName = "Elapsed Time MS";
    elapsedTimeDescriptor._debugGroup = group;
    elapsedTimeDescriptor._type = CallbackParam::TYPE_LARGE_UNSIGNED_INTEGER;
    elapsedTimeDescriptor._variable = &_elapsedTimeMS;

    _parent.platformContext().debug().addDebugVar(elapsedTimeDescriptor);
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
        PostFX::instance().setFadeOut(UColour(0), 1000.0, 0.0);
        switchScene(_sceneSwitchTarget.targetSceneName(),
                    _sceneSwitchTarget.unloadPreviousScene(),
                    _sceneSwitchTarget.loadInSeparateThread());
        WaitForAllTasks(getActiveScene().context(), true, true, false);
        PostFX::instance().setFadeIn(2750.0);
    } else {
        while (!_playerAddQueue.empty()) {
            std::pair<Scene*, SceneGraphNode*>& playerToAdd = _playerAddQueue.front();
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
    _saveTask.wait();

    Attorney::SceneManager::onRemoveActive(_scenePool->defaultSceneActive() ? _scenePool->defaultScene()
                                                                            : getActiveScene());

    _scenePool->activeScene(*scene);
    Attorney::SceneManager::onSetActive(*scene);
    if (!LoadSave::loadScene(*scene)) {
        //corrupt save
    }

    ShadowMap::resetShadowMaps(_platformContext->gfx());
    _platformContext->gui().onChangeScene(scene);
    ParamHandler::instance().setParam(_ID("activeScene"), scene->name());

    if (_camUpdateListenerID != 0) {
        Camera::removeUpdateListener(_camUpdateListenerID);
    }
    _camUpdateListenerID = Camera::addUpdateListener([this](const Camera& cam) {
        onCameraUpdate(cam);
    });
}

bool SceneManager::switchScene(const stringImpl& name, bool unloadPrevious, bool threaded) {
    assert(!name.empty());

    Scene* sceneToUnload = nullptr;
    if (!_scenePool->defaultSceneActive()) {
        sceneToUnload = &_scenePool->activeScene();
    }

    CreateTask(*_platformContext,
        [this, name, unloadPrevious, &sceneToUnload](const Task& parentTask)
        {
            // Load first, unload after to make sure we don't reload common resources
            if (load(name) != nullptr) {
                if (unloadPrevious && sceneToUnload) {
                    Attorney::SceneManager::onRemoveActive(*sceneToUnload);
                    unloadScene(sceneToUnload);
                }
            }
        }).startTask(threaded ? TaskPriority::DONT_CARE : TaskPriority::REALTIME, 
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
                    GUIButton* btn = gui->addButton(_ID_RT("Back"),
                                                    "Back",
                                                    pixelPosition(15, 15),
                                                    pixelScale(50, 25));
                    
                    btn->setEventCallback(GUIButton::Event::MouseClick,
                        [this](I64 btnGUID)
                        {
                            _sceneSwitchTarget.set(_scenePool->defaultScene().name(), true, false);
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
            
        });

    _sceneSwitchTarget.reset();

    return true;
}

vector<stringImpl> SceneManager::sceneNameList(bool sorted) const {
    return _scenePool->sceneNameList(sorted);
}

void SceneManager::initPostLoadState() {
    _processInput = true;
}

void SceneManager::onSizeChange(const SizeChangeParams& params) {
    if (params.isWindowResize) {
        return;
    }
    U16 w = params.width;
    U16 h = params.height;

    F32 aspectRatio = to_F32(w) / h;

    if (_init) {
        
        F32 fov = _platformContext->config().runtime.verticalFOV;;
        vec2<F32> zPlanes(_platformContext->config().runtime.zNear, _platformContext->config().runtime.zFar);

        for (const Player_ptr& player : _players) {
            if (player != nullptr) {
                player->getCamera().setProjection(aspectRatio, fov, zPlanes);
            }
        }

        Camera::utilityCamera(Camera::UtilityCamera::DEFAULT)->setProjection(aspectRatio, fov, zPlanes);
    }
}

void SceneManager::addPlayer(Scene& parentScene, SceneGraphNode* playerNode, bool queue) {
    if (queue) {
        _playerAddQueue.push(std::make_pair(&parentScene, playerNode));
    } else {
        addPlayerInternal(parentScene, playerNode);
    }
}

void SceneManager::addPlayerInternal(Scene& parentScene, SceneGraphNode* playerNode) {
    I64 sgnGUID = playerNode->getGUID();
    for (const Player_ptr& crtPlayer : _players) {
        if (crtPlayer && crtPlayer->getBoundNode()->getGUID() == sgnGUID) {
            return;
        }
    }

    U32 i = 0;
    for (; i < Config::MAX_LOCAL_PLAYER_COUNT; ++i) {
        if (_players[i] == nullptr) {
            break;
        }
    }

    if (i < Config::MAX_LOCAL_PLAYER_COUNT) {
        Player_ptr player = std::make_shared<Player>(to_U8(i));
        player->getCamera().fromCamera(*Camera::utilityCamera(Camera::UtilityCamera::DEFAULT));
        player->getCamera().setFixedYawAxis(true);
        playerNode->get<UnitComponent>()->setUnit(player);

        _players[i] = player;
        ++_activePlayerCount;

        _platformContext->gfx().resizeHistory(_activePlayerCount);
        Attorney::SceneManager::onPlayerAdd(parentScene, player);
    }
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
        for (U32 i = 0; i < Config::MAX_LOCAL_PLAYER_COUNT; ++i) {
            if (_players[i] != nullptr && _players[i]->getGUID() == targetGUID) {

                _players[i] = nullptr;
                --_activePlayerCount;

                _platformContext->gfx().resizeHistory(_activePlayerCount);
                Attorney::SceneManager::onPlayerRemove(parentScene, player);
                break;
            }
        }

        player.reset();
    }
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

void SceneManager::updateSceneState(const U64 deltaTimeUS) {
    Scene& activeScene = getActiveScene();
    assert(activeScene.getState() == ResourceState::RES_LOADED);
    // Update internal timers
    _elapsedTime += deltaTimeUS;
    _elapsedTimeMS = Time::MicrosecondsToMilliseconds<U32>(_elapsedTime);

    // Shadow splits are only visible in debug builds
    _sceneData->enableDebugRender(ParamHandler::instance().getParam<bool>(_ID("rendering.debug.displayShadowDebugInfo")));
    // Time, fog, etc
    _sceneData->elapsedTime(_elapsedTimeMS);
    _sceneData->deltaTime(Time::MicrosecondsToSeconds<F32>(deltaTimeUS));
    _sceneData->setRendererFlag(_platformContext->gfx().getRenderer().getFlag());
    _sceneData->detailLevel(_platformContext->gfx().renderDetailLevel(), _platformContext->gfx().shadowDetailLevel());

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

    const vectorEASTL<SceneGraphNode*>& waterBodies = activeScene.sceneGraph().getNodesByType(SceneNodeType::TYPE_WATER);
    U8 index = 0;
    for (SceneGraphNode* body : waterBodies) {
        const SceneGraphNode* water(body);
            
        _sceneData->waterDetails(index,
                                    water->get<TransformComponent>()->getPosition(),
                                    water->getNode<WaterPlane>()->getDimensions());
        ++index;
        if (index == 1) {//<- temp
            break;
        }
    }

    activeScene.updateSceneState(deltaTimeUS);

    _saveTimer += deltaTimeUS;

    if (_saveTimer >= Time::SecondsToMicroseconds(5)) {
        _saveTask.wait();
        _saveTask = CreateTask(*_platformContext,
            [&activeScene](const Task& parentTask) {
                LoadSave::saveScene(activeScene);
            }
        );
        _saveTask.startTask();
        _saveTimer = 0ULL;
    }
}

void SceneManager::preRender(RenderStagePass stagePass, const Camera& camera, RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    const GFXDevice& gfx = _platformContext->gfx();

    LightPool* lightPool = Attorney::SceneManager::lightPool(getActiveScene());
    gfx.getRenderer().preRender(stagePass, target, *lightPool, bufferInOut);
}

void SceneManager::postRender(RenderStagePass stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    SceneRenderState& activeSceneRenderState = getActiveScene().renderState();
    parent().renderPassManager().getQueue().postRender(activeSceneRenderState, stagePass, bufferInOut);
}

void SceneManager::debugDraw(RenderStagePass stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    Scene& activeScene = getActiveScene();
    SceneRenderState& activeSceneRenderState = activeScene.renderState();

    Attorney::SceneManager::debugDraw(activeScene, camera, stagePass, bufferInOut);
    // Draw bounding boxes, skeletons, axis gizmo, etc.
    _platformContext->gfx().debugDraw(activeSceneRenderState, camera, bufferInOut);
}

bool SceneManager::generateShadowMaps(GFX::CommandBuffer& bufferInOut) {
    Scene& activeScene = getActiveScene();
    LightPool* lightPool = Attorney::SceneManager::lightPool(activeScene);
    assert(lightPool != nullptr);
    return lightPool->generateShadowMaps(activeScene.renderState(), *playerCamera(), bufferInOut);
}

Camera* SceneManager::playerCamera(PlayerIndex idx) const {
    if (getActivePlayerCount() <= idx) {
        return nullptr;
    }

    Camera* overrideCamera = getActiveScene().state().playerState(idx).overrideCamera();
    if (overrideCamera == nullptr) {
        overrideCamera = &_players[idx]->getCamera();
    }

    return overrideCamera;
}

Camera* SceneManager::playerCamera() const {
    return playerCamera(_currentPlayerPass);
}

void SceneManager::currentPlayerPass(PlayerIndex idx) {
    _currentPlayerPass = idx;
    _platformContext->gfx().historyIndex(_currentPlayerPass, true);
    Attorney::SceneManager::currentPlayerPass(getActiveScene(), _currentPlayerPass);
}

RenderPassCuller::VisibleNodeList SceneManager::getSortedReflectiveNodes(const Camera& camera, RenderStage stage, bool inView) const {
    const SceneGraph& activeSceneGraph = getActiveScene().sceneGraph();
    vectorEASTL<SceneGraphNode*> allNodes = activeSceneGraph.getNodesByType(SceneNodeType::TYPE_WATER);
    vectorEASTL<SceneGraphNode*> otherNodes = activeSceneGraph.getNodesByType(SceneNodeType::TYPE_OBJECT3D);
    otherNodes.erase(eastl::remove_if(eastl::begin(otherNodes),
                                      eastl::end(otherNodes),
                                      [](SceneGraphNode* node) -> bool {
                                          RenderingComponent* rComp = node->get<RenderingComponent>();
                                          return !(rComp->getMaterialInstance() && rComp->getMaterialInstance()->isReflective());
                                      }),
                    eastl::end(otherNodes));

    allNodes.insert(eastl::end(allNodes), eastl::begin(otherNodes), eastl::end(otherNodes));

    if (inView) {
        return _renderPassCuller->frustumCull(camera, camera.getZPlanes().y, stage, allNodes);
    }

    return _renderPassCuller->toVisibleNodes(camera, allNodes);
}

RenderPassCuller::VisibleNodeList SceneManager::getSortedRefractiveNodes(const Camera& camera, RenderStage stage, bool inView) const {
    const SceneGraph& activeSceneGraph = getActiveScene().sceneGraph();
    vectorEASTL<SceneGraphNode*> allNodes = activeSceneGraph.getNodesByType(SceneNodeType::TYPE_WATER);
    vectorEASTL<SceneGraphNode*> otherNodes = activeSceneGraph.getNodesByType(SceneNodeType::TYPE_OBJECT3D);
    otherNodes.erase(eastl::remove_if(eastl::begin(otherNodes),
                                      eastl::end(otherNodes),
                                      [](SceneGraphNode* node) -> bool {
                                          RenderingComponent* rComp = node->get<RenderingComponent>();
                                          return !(rComp->getMaterialInstance() && rComp->getMaterialInstance()->isRefractive());
                                      }),
                    eastl::end(otherNodes));

    allNodes.insert(eastl::end(allNodes), eastl::begin(otherNodes), eastl::end(otherNodes));

    if (inView) {
        return _renderPassCuller->frustumCull(camera, camera.getZPlanes().y, stage, allNodes);
    }

    return _renderPassCuller->toVisibleNodes(camera, allNodes);
}

namespace {
    // Return true if the node type is capable of generating draw commands
    bool generatesDrawCommands(SceneNodeType nodeType) {
        STUBBED("ToDo: Use some additional flag type for these! -Ionut");
        return nodeType != SceneNodeType::TYPE_ROOT &&
               nodeType != SceneNodeType::TYPE_TRANSFORM &&
               nodeType != SceneNodeType::TYPE_LIGHT &&
               nodeType != SceneNodeType::TYPE_TRIGGER;
    }

    // Return true if this node should be removed from a shadow pass
    bool doesNotCastShadows(RenderStage stage, const SceneGraphNode& node) {
        if (stage == RenderStage::SHADOW) {
            SceneNodeType type = node.getNode()->getType();
            if (type == SceneNodeType::TYPE_SKY) {
                return true;
            }
            if (type == SceneNodeType::TYPE_OBJECT3D && node.getNode<Object3D>()->getObjectType() == Object3D::ObjectType::DECAL) {
                return true;
            }
            RenderingComponent* rComp = node.get<RenderingComponent>();
            assert(rComp != nullptr);
            return !rComp->renderOptionEnabled(RenderingComponent::RenderOptions::CAST_SHADOWS);
        }

        return false;
    }
};

const RenderPassCuller::VisibleNodeList& SceneManager::cullSceneGraph(RenderStagePass stage, const Camera& camera) {
    Time::ScopedTimer timer(*_sceneGraphCullTimers[to_U32(stage._passType)][to_U32(stage._stage)]);

    Scene& activeScene = getActiveScene();
    SceneState& sceneState = activeScene.state();

    RenderStage renderStage = stage._stage;

    RenderPassCuller::CullParams cullParams;
    cullParams._context = &activeScene.context();
    cullParams._sceneGraph = &activeScene.sceneGraph();
    cullParams._sceneState = &sceneState;
    cullParams._stage = renderStage;
    cullParams._camera = &camera;
    if (renderStage == RenderStage::SHADOW) {
        cullParams._visibilityDistanceSq = std::numeric_limits<F32>::max();
    } else {
        cullParams._visibilityDistanceSq = SQUARED(sceneState.renderState().generalVisibility());
    }
    // Cull everything except 3D objects
    cullParams._cullFunction = [renderStage](const SceneGraphNode& node) -> bool {
        if (generatesDrawCommands(node.getNode()->getType())) {
            // only checks nodes and can return true for a shadow stage
            return doesNotCastShadows(renderStage, node);
        }

        return true;
    };

    return  _renderPassCuller->frustumCull(cullParams);
}

RenderPassCuller::VisibleNodeList& SceneManager::getVisibleNodesCache(RenderStage stage) {
    return _renderPassCuller->getNodeCache(stage);
}

void SceneManager::prepareLightData(RenderStage stage, const Camera& camera) {
    if (stage != RenderStage::SHADOW) {
        LightPool* lightPool = Attorney::SceneManager::lightPool(getActiveScene());
        lightPool->prepareLightData(stage, camera.getEye(), camera.getViewMatrix());
    }
}

void SceneManager::onLostFocus() {
    getActiveScene().onLostFocus();
}

void SceneManager::resetSelection(PlayerIndex idx) {
    Attorney::SceneManager::resetSelection(getActiveScene(), idx);
    for (DELEGATE_CBK<void, U8, SceneGraphNode*>& cbk : _selectionChangeCallbacks) {
        cbk(idx, nullptr);
    }
}

void SceneManager::setSelected(PlayerIndex idx, SceneGraphNode& sgn) {
    Attorney::SceneManager::setSelected(getActiveScene(), idx, sgn);
    for (DELEGATE_CBK<void, U8, SceneGraphNode*>& cbk : _selectionChangeCallbacks) {
        cbk(idx, &sgn);
    }
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

bool SceneManager::joystickvector3Moved(const Input::JoystickEvent& arg,
                                         I8 index) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().joystickvector3Moved(arg, index);
}

bool LoadSave::loadScene(Scene& activeScene) {
    if (activeScene.state().saveLoadDisabled()) {
        return true;
    }

    const stringImpl& sceneName = activeScene.name();

    stringImpl path = Paths::g_saveLocation +  sceneName + "/";
    stringImpl saveFile = "current_save.sav";
    stringImpl bakSaveFile = "save.bak";

    bool isLoadFromBackup = false;
    // If file is missing, restore from bak
    if (!fileExists((path + saveFile).c_str())) {
        isLoadFromBackup = true;

        // Save file might be deleted if it was corrupted
        if (fileExists((path + bakSaveFile).c_str())) {
            copyFile(path, bakSaveFile, path, saveFile, false);
        }
    }

    ByteBuffer save;
    if (save.loadFromFile(path, saveFile)) {
        if (!Attorney::SceneLoadSave::load(activeScene, save)) {
            //Remove the save and try the backup
            deleteFile(path, saveFile);
            if (!isLoadFromBackup) {
                return loadScene(activeScene);
            }
        }
    }
    return false;
}

bool LoadSave::saveScene(const Scene& activeScene) {
    if (activeScene.state().saveLoadDisabled()) {
        return true;
    }

    const stringImpl& sceneName = activeScene.name();
    stringImpl path = Paths::g_saveLocation + sceneName + "/";
    stringImpl saveFile = "current_save.sav";
    stringImpl bakSaveFile = "save.bak";

    if (fileExists((path + saveFile).c_str())) {
        copyFile(path, saveFile, path, bakSaveFile, true);
    }

    ByteBuffer save;
    if (Attorney::SceneLoadSave::save(activeScene, save)) {
        return save.dumpToFile(path, saveFile);
    }

    return false;
}

bool SceneManager::networkUpdate(U32 frameCount) {

    getActiveScene().sceneGraph();

    return true;
}

};