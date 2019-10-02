#include "stdafx.h"

#include "Headers/SceneManager.h"
#include "Headers/RenderPassManager.h"
#include "Headers/FrameListenerManager.h"

#include "Core/Headers/Kernel.h"
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

#include "ECS/Components/Headers/UnitComponent.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/SelectionComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

bool SceneManager::onStartup() {
    if (Material::onStartup()) {
        return Attorney::SceneManager::onStartup();
    }

    return false;
}

bool SceneManager::onShutdown() {
    if (Material::onShutdown()) {
        return Attorney::SceneManager::onShutdown();
    }

    return false;
}

SceneManager::SceneManager(Kernel& parentKernel)
    : FrameListener(),
      Input::InputAggregatorInterface(),
      KernelComponent(parentKernel),
      _platformContext(nullptr),
      _resourceCache(nullptr),
      _sceneData(nullptr),
      _renderPassCuller(nullptr),
      _processInput(false),
      _scenePool(nullptr),
      _init(false),
      _elapsedTime(0ULL),
      _elapsedTimeMS(0),
      _activePlayerCount(0),
      _currentPlayerPass(0),
      _saveTimer(0ULL)
{
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
        _parent.platformContext().gfx().postFX().setFadeOut(UColour3(0), 1000.0, 0.0);
        switchScene(_sceneSwitchTarget.targetSceneName(),
                    _sceneSwitchTarget.unloadPreviousScene(),
                    _sceneSwitchTarget.targetViewRect(),
                    _sceneSwitchTarget.loadInSeparateThread());
        WaitForAllTasks(getActiveScene().context(), true, true, false);
        _parent.platformContext().gfx().postFX().setFadeIn(2750.0);
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

        AI::Navigation::DivideRecast::instance();

        _scenePool = MemoryManager_NEW ScenePool(*this);

        _sceneGraphCullTimers[to_U32(RenderStage::DISPLAY)] = &Time::ADD_TIMER(Util::StringFormat("SceneGraph cull timer: Display").c_str());
        _sceneGraphCullTimers[to_U32(RenderStage::REFLECTION)] = &Time::ADD_TIMER(Util::StringFormat("SceneGraph cull timer: Reflection").c_str());
        _sceneGraphCullTimers[to_U32(RenderStage::REFRACTION)] = &Time::ADD_TIMER(Util::StringFormat("SceneGraph cull timer: Refraction").c_str());
        _sceneGraphCullTimers[to_U32(RenderStage::SHADOW)] = &Time::ADD_TIMER(Util::StringFormat("SceneGraph cull timer: Shadow").c_str());

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
        AI::Navigation::DivideRecast::instance().destroy();
        _platformContext = nullptr;
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

    bool sceneNotLoaded = loadingScene->getState() != ResourceState::RES_LOADED;

    if (sceneNotLoaded) {
        
        // Load the main scene from XML
        if (!Attorney::SceneManager::loadXML(*loadingScene, sceneName)) {
            return nullptr;
        }

        if (Attorney::SceneManager::load(*loadingScene, sceneName)) {
            Attorney::SceneManager::postLoad(*loadingScene);
        } else {
            return nullptr;
        }
    }

    return loadingScene;
}

bool SceneManager::unloadScene(Scene* scene) {
    assert(scene != nullptr);
    if (_saveTask != nullptr) {
        Wait(*_saveTask);
    }

    _platformContext->gui().onUnloadScene(scene);
    Attorney::SceneManager::onRemoveActive(*scene);
    return Attorney::SceneManager::unload(*scene);
}

void SceneManager::setActiveScene(Scene* const scene) {
    assert(scene != nullptr);
    if (_saveTask != nullptr) {
        Wait(*_saveTask);
    }

    Attorney::SceneManager::onRemoveActive(_scenePool->defaultSceneActive() ? _scenePool->defaultScene()
                                                                            : getActiveScene());

    _scenePool->activeScene(*scene);
    Attorney::SceneManager::onSetActive(*scene);
    if (!LoadSave::loadScene(*scene)) {
        //corrupt save
    }

    ShadowMap::resetShadowMaps();

    _platformContext->gui().onChangeScene(scene);
    ParamHandler::instance().setParam(_ID("activeScene"), scene->resourceName());
}

bool SceneManager::switchScene(const stringImpl& name, bool unloadPrevious, const Rect<U16>& targetRenderViewport, bool threaded) {
    assert(!name.empty());

    Scene* sceneToUnload = nullptr;
    if (!_scenePool->defaultSceneActive()) {
        sceneToUnload = &_scenePool->activeScene();
    }

    // We use our rendering task pool for scene changes because we might be creating / loading GPU assets (shaders, textures, buffers, etc)
    Start(*CreateTask(_platformContext->taskPool(TaskPoolType::HIGH_PRIORITY),
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
        "Switch scene task"),
        threaded ? TaskPriority::DONT_CARE : TaskPriority::REALTIME, 
        [this, name, &targetRenderViewport, unloadPrevious, &sceneToUnload]()
        {
            bool foundInCache = false;
            Scene* loadedScene = _scenePool->getOrCreateScene(*_platformContext, parent().resourceCache(), *this, name, foundInCache);
            assert(loadedScene != nullptr && foundInCache);

            if(loadedScene->getState() == ResourceState::RES_LOADING) {
                Attorney::SceneManager::postLoadMainThread(*loadedScene, targetRenderViewport);
                if (loadedScene->getGUID() != _scenePool->defaultScene().getGUID())
                {
                    SceneGUIElements* gui = Attorney::SceneManager::gui(*loadedScene);
                    GUIButton* btn = gui->addButton(_ID("Back"),
                                                    "Back",
                                                    pixelPosition(15, 15),
                                                    pixelScale(50, 25));
                    
                    btn->setEventCallback(GUIButton::Event::MouseClick,
                        [this, &targetRenderViewport](I64 btnGUID)
                        {
                            _sceneSwitchTarget.set(_scenePool->defaultScene().resourceName(), targetRenderViewport, true, false);
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

                Attorney::SceneManager::onPlayerRemove(parentScene, player);
                break;
            }
        }

        player.reset();
    }
}

vectorEASTL<SceneGraphNode*> SceneManager::getNodesInScreenRect(const Rect<F32>& screenRect, const Camera& camera) const {
    auto IsNodeInRect = [&screenRect, &camera](const SceneGraphNode* node) {
        assert(node != nullptr);
        if (node->getNode().type() == SceneNodeType::TYPE_OBJECT3D)
        {
            U8 objectType = node->getNode<Object3D>().getObjectType()._value;
            while (objectType == ObjectType::SUBMESH) {
                node = node->getParent();
                if (node) {
                    objectType = node->getNode<Object3D>().getObjectType()._value;
                } else {
                    return false;
                }
            }

            if (node->get<SelectionComponent>() &&
                node->get<SelectionComponent>()->enabled() &&
                objectType != ObjectType::DECAL &&
                objectType != ObjectType::SUBMESH &&
                objectType != ObjectType::TERRAIN)
            {
                BoundsComponent* bComp = node->get<BoundsComponent>();
                if (bComp != nullptr) {
                    vec3<F32> center = bComp->getBoundingBox().getCenter();
                    center = camera.project(center);
                    return screenRect.contains(center.xy());
                }
            }
        }
        return false;
    };

    vectorEASTL<SceneGraphNode*> ret;
    const VisibleNodeList& visNodes = _renderPassCuller->getNodeCache(RenderStage::DISPLAY);
    for (auto& it : visNodes) {
        if (IsNodeInRect(it._node)) {
            ret.push_back(it._node);
        }
    }
    return ret;
}

bool SceneManager::frameStarted(const FrameEvent& evt) {
    _sceneData->uploadToGPU();

    return Attorney::SceneManager::frameStarted(getActiveScene());
}

bool SceneManager::frameEnded(const FrameEvent& evt) {
    return Attorney::SceneManager::frameEnded(getActiveScene());
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
    _sceneData->deltaTime(Time::MicrosecondsToMilliseconds<F32>(deltaTimeUS));

    FogDescriptor& fog = activeScene.state().renderState().fogDescriptor();
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

    U8 index = 0;
    
    vector<WaterDetails>& waterBodies = activeScene.state().globalWaterBodies();
    for (auto body : waterBodies) {
        vec3<F32> posW (0.0f, body._heightOffset, 0.0f);
        vec3<F32> dim(std::numeric_limits<I16>::max(),
                      std::numeric_limits<I16>::max(),
                      body._depth);
        _sceneData->waterDetails(index++, posW, dim);
    }

    const vectorEASTL<SceneGraphNode*>& waterNodes = activeScene.sceneGraph().getNodesByType(SceneNodeType::TYPE_WATER);
    for (SceneGraphNode* body : waterNodes) {
        _sceneData->waterDetails(index++,
                                 body->get<TransformComponent>()->getPosition(),
                                 body->getNode<WaterPlane>().getDimensions());
    }

    activeScene.updateSceneState(deltaTimeUS);

    _saveTimer += deltaTimeUS;

    if (_saveTimer >= Time::SecondsToMicroseconds(5)) {
        saveActiveScene(true);
        _saveTimer = 0ULL;
    }
}

void SceneManager::preRender(RenderStagePass stagePass, const Camera& camera, RenderTargetID target, GFX::CommandBuffer& bufferInOut) {
    _platformContext->gfx().getRenderer().preRender(stagePass, target, getActiveScene().lightPool(), camera, bufferInOut);
}

void SceneManager::postRender(RenderStagePass stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    SceneRenderState& activeSceneRenderState = getActiveScene().renderState();
    parent().renderPassManager().getQueue().postRender(activeSceneRenderState, stagePass, bufferInOut);
}

void SceneManager::preRenderAllPasses(const Camera& playerCamera) {
    getActiveScene().lightPool().preRenderAllPasses(playerCamera);
}

void SceneManager::postRenderAllPasses(const Camera& playerCamera) {
    getActiveScene().lightPool().postRenderAllPasses(playerCamera);
}

void SceneManager::drawCustomUI(GFX::CommandBuffer& bufferInOut) {
    Attorney::SceneManager::drawCustomUI(getActiveScene(), bufferInOut);
}

void SceneManager::debugDraw(RenderStagePass stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    Scene& activeScene = getActiveScene();

    Attorney::SceneManager::debugDraw(activeScene, camera, stagePass, bufferInOut);
    // Draw bounding boxes, skeletons, axis gizmo, etc.
    _platformContext->gfx().debugDraw(activeScene.renderState(), camera, bufferInOut);
}

void SceneManager::generateShadowMaps(GFX::CommandBuffer& bufferInOut) {
    if (_platformContext->config().rendering.shadowMapping.enabled) {
        getActiveScene().lightPool().generateShadowMaps(*playerCamera(), bufferInOut);
    }
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
    Attorney::SceneManager::currentPlayerPass(getActiveScene(), _currentPlayerPass);
    playerCamera()->updateLookAt();
}

VisibleNodeList SceneManager::getSortedReflectiveNodes(const Camera& camera, RenderStage stage, bool inView) const {
    const SceneGraph& activeSceneGraph = getActiveScene().sceneGraph();
    const vectorEASTL<SceneGraphNode*>& waterNodes = activeSceneGraph.getNodesByType(SceneNodeType::TYPE_WATER);
    const vectorEASTL<SceneGraphNode*>& otherNodes = activeSceneGraph.getNodesByType(SceneNodeType::TYPE_OBJECT3D);

    vectorEASTL<SceneGraphNode*> allNodes;
    allNodes.reserve(waterNodes.size() + otherNodes.size());

    eastl::copy(eastl::cbegin(waterNodes),
                eastl::cend(waterNodes),
                eastl::back_inserter(allNodes));

    eastl::copy_if(eastl::cbegin(otherNodes),
                   eastl::cend(otherNodes),
                   eastl::back_inserter(allNodes),
                   [](SceneGraphNode* node) -> bool {
                        RenderingComponent* rComp = node->get<RenderingComponent>();
                        return rComp->getMaterialInstance() && rComp->getMaterialInstance()->isReflective();
                   });

    if (inView) {
        NodeCullParams cullParams = {};
        cullParams._threaded = true;
        cullParams._lodThresholds = getActiveScene().state().renderState().lodThresholds();
        cullParams._stage = stage;
        cullParams._currentCamera = &camera;
        cullParams._cullMaxDistanceSq = SQUARED(camera.getZPlanes().y);

        return _renderPassCuller->frustumCull(cullParams, allNodes);
    }

    return _renderPassCuller->toVisibleNodes(camera, allNodes);
}

VisibleNodeList SceneManager::getSortedRefractiveNodes(const Camera& camera, RenderStage stage, bool inView) const {
    const SceneGraph& activeSceneGraph = getActiveScene().sceneGraph();
    const vectorEASTL<SceneGraphNode*>& waterNodes = activeSceneGraph.getNodesByType(SceneNodeType::TYPE_WATER);
    const vectorEASTL<SceneGraphNode*>& otherNodes = activeSceneGraph.getNodesByType(SceneNodeType::TYPE_OBJECT3D);

    vectorEASTL<SceneGraphNode*> allNodes;
    allNodes.reserve(waterNodes.size() + otherNodes.size());

    eastl::copy(eastl::cbegin(waterNodes),
                eastl::cend(waterNodes),
                eastl::back_inserter(allNodes));

    eastl::copy_if(eastl::cbegin(otherNodes),
                   eastl::cend(otherNodes),
                   eastl::back_inserter(allNodes),
                   [](SceneGraphNode* node) -> bool {
                        RenderingComponent* rComp = node->get<RenderingComponent>();
                        return rComp->getMaterialInstance() && rComp->getMaterialInstance()->isRefractive();
                   });

    if (inView) {
        NodeCullParams cullParams = {};
        cullParams._threaded = true;
        cullParams._lodThresholds = getActiveScene().state().renderState().lodThresholds();
        cullParams._stage = stage;
        cullParams._currentCamera = &camera;
        cullParams._cullMaxDistanceSq = SQUARED(camera.getZPlanes().y);

        return _renderPassCuller->frustumCull(cullParams, allNodes);
    }

    return _renderPassCuller->toVisibleNodes(camera, allNodes);
}

namespace {
    // Return true if the node type is capable of generating draw commands
    FORCE_INLINE bool generatesDrawCommands(SceneNodeType nodeType) {
        return nodeType != SceneNodeType::TYPE_ROOT &&
               nodeType != SceneNodeType::TYPE_TRANSFORM &&
               nodeType != SceneNodeType::TYPE_TRIGGER &&
               nodeType != SceneNodeType::TYPE_EMPTY;
    }

    // Return true if this node should be removed from a shadow pass
    bool doesNotCastShadows(RenderStage stage, const SceneGraphNode& node, const SceneNode& sceneNode) {
        if (stage == RenderStage::SHADOW) {
            SceneNodeType type = sceneNode.type();
            if (type == SceneNodeType::TYPE_SKY || 
                type == SceneNodeType::TYPE_WATER ||
                type == SceneNodeType::TYPE_INFINITEPLANE) {
                return true;
            }
            if (type == SceneNodeType::TYPE_OBJECT3D && static_cast<const Object3D&>(sceneNode).getObjectType()._value == ObjectType::DECAL) {
                return true;
            }
            RenderingComponent* rComp = node.get<RenderingComponent>();
            assert(rComp != nullptr);
            return !rComp->renderOptionEnabled(RenderingComponent::RenderOptions::CAST_SHADOWS);
        }

        return false;
    }
};

const VisibleNodeList& SceneManager::cullSceneGraph(RenderStage stage, const Camera& camera, I32 minLoD, const vec3<F32>& minExtents) {
    Time::ScopedTimer timer(*_sceneGraphCullTimers[to_U32(stage)]);

    Scene& activeScene = getActiveScene();
    SceneState& sceneState = activeScene.state();

    RenderPassCuller::CullParams cullParams = {};
    cullParams._context = &activeScene.context();
    cullParams._sceneGraph = &activeScene.sceneGraph();
    cullParams._sceneState = &sceneState;
    cullParams._stage = stage;
    cullParams._camera = &camera;
    cullParams._threaded = true;
    cullParams._minLoD = minLoD;
    cullParams._minExtents = minExtents;
    cullParams._visibilityDistanceSq = std::numeric_limits<F32>::max();

    if (stage != RenderStage::SHADOW) {
        cullParams._visibilityDistanceSq = SQUARED(sceneState.renderState().generalVisibility());
    }

    // Cull everything except 3D objects
    cullParams._cullFunction = [stage](const SceneGraphNode& node, const SceneNode& sceneNode) -> bool {
        if (generatesDrawCommands(sceneNode.type())) {
            // only checks nodes and can return true for a shadow stage
            return doesNotCastShadows(stage, node, sceneNode);
        }

        return true;
    };

    return _renderPassCuller->frustumCull(cullParams);
}

void SceneManager::prepareLightData(RenderStage stage, const Camera& camera) {
    if (stage != RenderStage::SHADOW) {
        getActiveScene().lightPool().prepareLightData(stage, camera.getEye(), camera.getViewMatrix());
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

bool SceneManager::mouseMoved(const Input::MouseMoveEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().mouseMoved(arg);
}

bool SceneManager::mouseButtonPressed(const Input::MouseButtonEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().mouseButtonPressed(arg);
}

bool SceneManager::mouseButtonReleased(const Input::MouseButtonEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().mouseButtonReleased(arg);
}

bool SceneManager::joystickAxisMoved(const Input::JoystickEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().joystickAxisMoved(arg);
}

bool SceneManager::joystickPovMoved(const Input::JoystickEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().joystickPovMoved(arg);
}

bool SceneManager::joystickButtonPressed(const Input::JoystickEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().joystickButtonPressed(arg);
}

bool SceneManager::joystickButtonReleased(const Input::JoystickEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().joystickButtonReleased(arg);
}

bool SceneManager::joystickBallMoved(const Input::JoystickEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().joystickBallMoved(arg);
}

bool SceneManager::joystickAddRemove(const Input::JoystickEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().joystickAddRemove(arg);
}

bool SceneManager::joystickRemap(const Input::JoystickEvent &arg) {
    if (!_processInput) {
        return false;
    }

    return getActiveScene().input().joystickRemap(arg);
}

bool SceneManager::onUTF8(const Input::UTF8Event& arg) {
    ACKNOWLEDGE_UNUSED(arg);

    return false;
}

bool LoadSave::loadScene(Scene& activeScene) {
    if (activeScene.state().saveLoadDisabled()) {
        return true;
    }

    const stringImpl& sceneName = activeScene.resourceName();

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

bool LoadSave::saveScene(const Scene& activeScene, bool toCache) {
    if (!toCache) {
        return activeScene.saveXML();
    }

    if (activeScene.state().saveLoadDisabled()) {
        return true;
    }

    const stringImpl& sceneName = activeScene.resourceName();
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

bool SceneManager::saveActiveScene(bool toCache, bool deferred) {
    const Scene& activeScene = getActiveScene();

    if (_saveTask != nullptr) {
        Wait(*_saveTask);
    }

    TaskPool& pool = parent().platformContext().taskPool(TaskPoolType::LOW_PRIORITY);
    _saveTask = CreateTask(pool,
                           nullptr,
                           [&activeScene, toCache](const Task& parentTask) {
                               LoadSave::saveScene(activeScene, toCache);
                           },
                           "Save scene task");
    Start(*_saveTask, deferred ? TaskPriority::DONT_CARE : TaskPriority::REALTIME);

    return true;
}

bool SceneManager::networkUpdate(U32 frameCount) {

    getActiveScene().sceneGraph();

    return true;
}

};