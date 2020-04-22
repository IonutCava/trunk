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
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "AI/PathFinding/Headers/DivideRecast.h"

#include "Environment/Water/Headers/Water.h"
#include "Environment/Vegetation/Headers/Vegetation.h"

#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Dynamics/Entities/Units/Headers/Player.h"
#include "Core/Debugging/Headers/DebugInterface.h"

#include "ECS/Components/Headers/UnitComponent.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/SelectionComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

bool SceneManager::onStartup() {
    if (RenderPassCuller::onStartup()) {
        return Attorney::SceneManager::onStartup();
    }
    
    return false;
}

bool SceneManager::onShutdown() {
    if (RenderPassCuller::onShutdown()) {
        return Attorney::SceneManager::onShutdown();
    }

    return false;
}

SceneManager::SceneManager(Kernel& parentKernel)
    : FrameListener(),
      Input::InputAggregatorInterface(),
      KernelComponent(parentKernel)
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
    if (_sceneSwitchTarget._isSet) {
        _parent.platformContext().gfx().getRenderer().postFX().setFadeOut(UColour3(0), 1000.0, 0.0);
        switchScene(_sceneSwitchTarget._targetSceneName,
                    _sceneSwitchTarget._unloadPreviousScene,
                    _sceneSwitchTarget._targetViewRect,
                    _sceneSwitchTarget._loadInSeparateThread);
        WaitForAllTasks(getActiveScene().context(), true, true, false);
        _parent.platformContext().gfx().getRenderer().postFX().setFadeIn(2750.0);
    } else {
        if (_playerQueueDirty) {
            while (!_playerAddQueue.empty()) {
                std::pair<Scene*, SceneGraphNode*>& playerToAdd = _playerAddQueue.front();
                addPlayerInternal(*playerToAdd.first, playerToAdd.second);
                _playerAddQueue.pop();
            }
            while (!_playerRemoveQueue.empty()) {
                std::pair<Scene*, SceneGraphNode*>& playerToRemove = _playerRemoveQueue.front();
                removePlayerInternal(*playerToRemove.first, playerToRemove.second);
                _playerRemoveQueue.pop();
            }
            _playerQueueDirty = false;
        }

        getActiveScene().idle();
    }
}

bool SceneManager::init(PlatformContext& platformContext, ResourceCache* cache) {
    if (_platformContext == nullptr) {
        _platformContext = &platformContext;
        _resourceCache = cache;
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
        Vegetation::destroyStaticData();
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

Scene* SceneManager::load(const Str128& sceneName) {
    bool foundInCache = false;
    Scene* loadingScene = _scenePool->getOrCreateScene(*_platformContext, parent().resourceCache(), *this, sceneName, foundInCache);

    if (!loadingScene) {
        Console::errorfn(Locale::get(_ID("ERROR_XML_LOAD_INVALID_SCENE")));
        return nullptr;
    }

    ParamHandler::instance().setParam(_ID_32("currentScene"), stringImpl(sceneName.c_str()));

    const bool sceneNotLoaded = loadingScene->getState() != ResourceState::RES_LOADED;

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
    ParamHandler::instance().setParam(_ID_32("activeScene"), scene->resourceName());
}

bool SceneManager::switchScene(const Str128& name, bool unloadPrevious, const Rect<U16>& targetRenderViewport, bool threaded) {
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
        }),
        threaded ? TaskPriority::DONT_CARE : TaskPriority::REALTIME, 
        [this, name, targetRenderViewport, unloadPrevious, &sceneToUnload]()
        {
            bool foundInCache = false;
            Scene* loadedScene = _scenePool->getOrCreateScene(*_platformContext, parent().resourceCache(), *this, name, foundInCache);
            assert(loadedScene != nullptr && foundInCache);

            if(loadedScene->getState() == ResourceState::RES_LOADING) {
                Attorney::SceneManager::postLoadMainThread(*loadedScene, targetRenderViewport);
                if (loadedScene->getGUID() != _scenePool->defaultScene().getGUID())
                {
                    SceneGUIElements* gui = Attorney::SceneManager::gui(*loadedScene);
                    GUIButton* btn = gui->addButton("Back",
                                                    "Back",
                                                    pixelPosition(15, 15),
                                                    pixelScale(50, 25));
                    
                    btn->setEventCallback(GUIButton::Event::MouseClick,
                        [this, &targetRenderViewport](I64 btnGUID)
                        {
                            _sceneSwitchTarget = {
                                _scenePool->defaultScene().resourceName(),
                                targetRenderViewport, 
                                true,
                                false,
                                true
                            };
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

    _sceneSwitchTarget = {};

    return true;
}

vectorEASTL<Str128> SceneManager::sceneNameList(bool sorted) const {
    return _scenePool->sceneNameList(sorted);
}

void SceneManager::initPostLoadState() {
    _processInput = true;
}

void SceneManager::onSizeChange(const SizeChangeParams& params) {
    if (params.isWindowResize) {
        return;
    }
    const U16 w = params.width;
    const U16 h = params.height;

    const F32 aspectRatio = to_F32(w) / h;

    if (_init) {
        
        const F32 fov = _platformContext->config().runtime.verticalFOV;;
        const vec2<F32> zPlanes(_platformContext->config().runtime.zNear, _platformContext->config().runtime.zFar);

        for (UnitComponent* player : _players) {
            if (player != nullptr) {
                player->getUnit<Player>()->getCamera().setProjection(aspectRatio, fov, zPlanes);
            }
        }

        Camera::utilityCamera(Camera::UtilityCamera::DEFAULT)->setProjection(aspectRatio, fov, zPlanes);
    }
}

void SceneManager::addPlayer(Scene& parentScene, SceneGraphNode* playerNode, bool queue) {
    if (queue) {
        _playerAddQueue.push(std::make_pair(&parentScene, playerNode));
        _playerQueueDirty = true;
    } else {
        addPlayerInternal(parentScene, playerNode);
    }
}

void SceneManager::addPlayerInternal(Scene& parentScene, SceneGraphNode* playerNode) {
    const I64 sgnGUID = playerNode->getGUID();
    for (UnitComponent* crtPlayer : _players) {
        if (crtPlayer && crtPlayer->getSGN().getGUID() == sgnGUID) {
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
        _players[i] = playerNode->get<UnitComponent>();
        _players[i]->setUnit(player);

        ++_activePlayerCount;

        Attorney::SceneManager::onPlayerAdd(parentScene, player);
    }
}

void SceneManager::removePlayer(Scene& parentScene, SceneGraphNode* playerNode, bool queue) {
    if (queue) {
        _playerRemoveQueue.push(std::make_pair(&parentScene, playerNode));
        _playerQueueDirty = true;
    } else {
        removePlayerInternal(parentScene, playerNode);
    }
}

void SceneManager::removePlayerInternal(Scene& parentScene, SceneGraphNode* playerNode) {
    if (playerNode == nullptr) {
        return;
    }

    const I64 targetGUID = playerNode->getGUID();
    for (U32 i = 0; i < Config::MAX_LOCAL_PLAYER_COUNT; ++i) {
        if (_players[i] != nullptr && _players[i]->getSGN().getGUID() == targetGUID) {
            --_activePlayerCount;
            Attorney::SceneManager::onPlayerRemove(parentScene, _players[i]->getUnit<Player>());
            _players[i] = nullptr;
            break;
        }
    }
}

vectorEASTL<SceneGraphNode*> SceneManager::getNodesInScreenRect(const Rect<I32>& screenRect, const Camera& camera, const Rect<I32>& viewport) const {
    OPTICK_EVENT();

    const SceneGraph& sceneGraph = getActiveScene().sceneGraph();
    const vec3<F32>& eye = camera.getEye();
    const vec2<F32>& zPlanes = camera.getZPlanes();

    auto CheckPointLoS = [&eye, &zPlanes, &sceneGraph](const vec3<F32>& point, I64 nodeGUID, I64 parentNodeGUID) -> bool {
        const Ray cameraRay(point, point.direction(eye));
        const F32 distanceToPoint = eye.distance(point);

        vectorEASTL<SGNRayResult> rayResults;
        sceneGraph.intersect(cameraRay, 0.f, zPlanes.y, rayResults);
        for (SGNRayResult& result : rayResults) {
            if (result.sgnGUID == nodeGUID || result.sgnGUID == parentNodeGUID) {
                continue;
            }

            if (result.dist < distanceToPoint) {
                return false;
            }
        }
        return true;
    };

    const auto HasLoSToCamera = [&](SceneGraphNode* ignoreNode, const BoundingBox* bb) -> SceneGraphNode* {
        SceneGraphNode* parent = nullptr;
        I64 parentNodeGUID = -1;

        const I64 nodeGUID = ignoreNode->getGUID();
        if (ignoreNode->getNode().type() == SceneNodeType::TYPE_OBJECT3D) {
            parent = ignoreNode->parent();
            parentNodeGUID = parent->getGUID();
        }

        // Quick raycast to camera
        if (CheckPointLoS(bb->getCenter(), nodeGUID, parentNodeGUID)) {
            return parent == nullptr ? ignoreNode : parent;
        }
        // This is gonna hurt. The raycast failed, but the node might still be visible
        auto points = bb->getPoints();
        for (auto& point : points) {
            if (CheckPointLoS(point, nodeGUID, parentNodeGUID)) {
                return parent == nullptr ? ignoreNode : parent;
            }
        }

        return nullptr;
    };

    const auto IsNodeInRect = [&screenRect, &camera, &viewport, &sceneGraph](SceneGraphNode* node)->std::pair<SceneGraphNode*, const BoundingBox*> {
        assert(node != nullptr);
        const SceneNode& sNode = node->getNode();
        if (sNode.type() == SceneNodeType::TYPE_OBJECT3D) {
            SelectionComponent* sComp = node->get<SelectionComponent>();
            if (sComp == nullptr && 
                (sNode.type() == SceneNodeType::TYPE_OBJECT3D && node->getNode<Object3D>().getObjectType() == to_base(ObjectType::SUBMESH)))
            {
                if (node->parent() != nullptr) {
                    // Already selected. Skip.
                    if (node->parent()->hasFlag(SceneGraphNode::Flags::SELECTED)) {
                        return { nullptr, nullptr };
                    }
                    sComp = node->parent()->get<SelectionComponent>();
                }
            }
            if (sComp != nullptr && sComp->enabled()) {
                BoundsComponent* bComp = node->get<BoundsComponent>();
                if (bComp != nullptr) {
                    const BoundingBox& bb = bComp->getBoundingBox();
                    const vec3<F32>& center = bb.getCenter();
                    if (screenRect.contains(camera.project(center, viewport))) {
                        return { node, &bb };
                    }
                }
            }
        }

        return { nullptr, nullptr };
    };

    vectorEASTL<SceneGraphNode*> ret = {};
    const VisibleNodeList& visNodes = _renderPassCuller->getNodeCache(RenderStage::DISPLAY);
    for (const VisibleNode& it : visNodes) {
        auto [parsedNode, bb] = IsNodeInRect(it._node);
        if (parsedNode != nullptr) {
            parsedNode = HasLoSToCamera(parsedNode, bb);
            if (parsedNode != nullptr) {
                // submeshes will usually return their parent mesh as a result so we try and avoid having the same mesh multiple times
                if (eastl::find(eastl::cbegin(ret), eastl::cend(ret), parsedNode) == eastl::cend(ret)) {
                    ret.push_back(parsedNode);
                }
            }
        }
    }

    return ret;
}

bool SceneManager::frameStarted(const FrameEvent& evt) {
    OPTICK_EVENT();

    _sceneData->uploadToGPU();

    return Attorney::SceneManager::frameStarted(getActiveScene());
}

bool SceneManager::frameEnded(const FrameEvent& evt) {
    OPTICK_EVENT();

    return Attorney::SceneManager::frameEnded(getActiveScene());
}

void SceneManager::updateSceneState(const U64 deltaTimeUS) {
    OPTICK_EVENT();

    Scene& activeScene = getActiveScene();
    assert(activeScene.getState() == ResourceState::RES_LOADED);
    // Update internal timers
    _elapsedTime += deltaTimeUS;
    _elapsedTimeMS = Time::MicrosecondsToMilliseconds<U32>(_elapsedTime);

    // Shadow splits are only visible in debug builds
    _sceneData->enableDebugRender(ParamHandler::instance().getParam<bool>(_ID_32("rendering.debug.displayShadowDebugInfo")));
    // Time, fog, etc
    _sceneData->elapsedTime(_elapsedTimeMS);
    _sceneData->deltaTime(Time::MicrosecondsToMilliseconds<F32>(deltaTimeUS));

    FogDescriptor& fog = activeScene.state().renderState().fogDescriptor();
    const bool fogEnabled = _platformContext->config().rendering.enableFog;
    if (fog.dirty() || fogEnabled != fog.active()) {
        const vec3<F32>& colour = fog.colour();
        const F32 density = fogEnabled ? fog.density() : 0.0f;
        _sceneData->fogDetails(colour.r, colour.g, colour.b, density);
        fog.clean();
        fog.active(fogEnabled);
    }

    const SceneState& activeSceneState = activeScene.state();
    _sceneData->windDetails(activeSceneState.windDirX(),
                            0.0f,
                            activeSceneState.windDirZ(),
                            activeSceneState.windSpeed());

    _sceneData->shadowingSettings(activeSceneState.lightBleedBias(),
                                  activeSceneState.minShadowVariance(),
                                  to_F32(activeSceneState.shadowFadeDistance()),
                                  to_F32(activeSceneState.shadowDistance()));
    U8 index = 0;
    
    vectorEASTL<WaterDetails>& waterBodies = activeScene.state().globalWaterBodies();
    for (auto body : waterBodies) {
        const vec3<F32> posW (0.0f, body._heightOffset, 0.0f);
        const vec3<F32> dim(std::numeric_limits<I16>::max(),
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

    if (_saveTimer >= Time::SecondsToMicroseconds(Config::Build::IS_DEBUG_BUILD ? 5 : 10)) {
        saveActiveScene(true, true);
        _saveTimer = 0ULL;
    }
}

void SceneManager::preRender(const RenderStagePass& stagePass, const Camera& camera, const Texture_ptr& hizColourTexture, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

    _platformContext->gfx().getRenderer().preRender(stagePass, hizColourTexture, getActiveScene().lightPool(), camera, bufferInOut);
}

void SceneManager::postRender(const RenderStagePass& stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

    const SceneRenderState& activeSceneRenderState = getActiveScene().renderState();
    parent().renderPassManager()->getQueue().postRender(activeSceneRenderState, stagePass, bufferInOut);
}

void SceneManager::preRenderAllPasses(const Camera& playerCamera) {
    OPTICK_EVENT();

    getActiveScene().lightPool().preRenderAllPasses(playerCamera);
}

void SceneManager::postRenderAllPasses(const Camera& playerCamera) {
    OPTICK_EVENT();

    getActiveScene().lightPool().postRenderAllPasses(playerCamera);
}

void SceneManager::drawCustomUI(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
    //Set a 2D camera for rendering
    GFX::EnqueueCommand(bufferInOut, GFX::SetCameraCommand{ Camera::utilityCamera(Camera::UtilityCamera::_2D)->snapshot() });
    GFX::EnqueueCommand(bufferInOut, GFX::SetViewportCommand{ targetViewport });

    Attorney::SceneManager::drawCustomUI(getActiveScene(), targetViewport, bufferInOut);
}

void SceneManager::debugDraw(const RenderStagePass& stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

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
        overrideCamera = &_players[idx]->getUnit<Player>()->getCamera();
    }

    return overrideCamera;
}

Camera* SceneManager::playerCamera() const {
    return playerCamera(_currentPlayerPass);
}

void SceneManager::currentPlayerPass(PlayerIndex idx) {
    OPTICK_EVENT();

    _currentPlayerPass = idx;
    Attorney::SceneManager::currentPlayerPass(getActiveScene(), _currentPlayerPass);
    playerCamera()->updateLookAt();
}

void SceneManager::moveCameraToNode(SceneGraphNode* targetNode, F32 targetDistanceFromNode) const {
    if (targetNode != nullptr) {
        TransformComponent* tComp = targetNode->get<TransformComponent>();
        if (tComp != nullptr) {
            vec3<F32> targetPos = tComp->getPosition();
            if (targetDistanceFromNode > 0.0f) {
                targetPos -= Normalized(Rotate(WORLD_Z_NEG_AXIS, playerCamera()->getOrientation())) * targetDistanceFromNode;
            }
            playerCamera()->setEye(targetPos);
        }
    }
}

VisibleNodeList SceneManager::getSortedReflectiveNodes(const Camera& camera, RenderStage stage, bool inView) const {
    OPTICK_EVENT();

    static vectorEASTL<SceneGraphNode*> allNodes = {};
    getActiveScene().sceneGraph().getNodesByType({ SceneNodeType::TYPE_WATER, SceneNodeType::TYPE_OBJECT3D }, allNodes);

    eastl::erase_if(allNodes,
                   [](SceneGraphNode* node) noexcept ->  bool {
                        const Material_ptr& mat = node->get<RenderingComponent>()->getMaterialInstance();
                        return node->getNode().type() != SceneNodeType::TYPE_WATER && (mat == nullptr || !mat->isReflective());
                   });

    if (inView) {
        NodeCullParams cullParams = {};
        cullParams._lodThresholds = getActiveScene().state().renderState().lodThresholds();
        cullParams._stage = stage;
        cullParams._currentCamera = &camera;
        cullParams._cullMaxDistanceSq = SQUARED(camera.getZPlanes().y);

        return _renderPassCuller->frustumCull(cullParams, allNodes);
    }

    return _renderPassCuller->toVisibleNodes(camera, allNodes);
}

VisibleNodeList SceneManager::getSortedRefractiveNodes(const Camera& camera, RenderStage stage, bool inView) const {
    OPTICK_EVENT();

    static vectorEASTL<SceneGraphNode*> allNodes = {};
    getActiveScene().sceneGraph().getNodesByType({ SceneNodeType::TYPE_WATER, SceneNodeType::TYPE_OBJECT3D }, allNodes);

    eastl::erase_if(allNodes,
                   [](SceneGraphNode* node) noexcept ->  bool {
                        const Material_ptr& mat = node->get<RenderingComponent>()->getMaterialInstance();
                        return node->getNode().type() != SceneNodeType::TYPE_WATER && (mat == nullptr || !mat->isRefractive());
                   });
    if (inView) {
        NodeCullParams cullParams = {};
        cullParams._lodThresholds = getActiveScene().state().renderState().lodThresholds();
        cullParams._stage = stage;
        cullParams._currentCamera = &camera;
        cullParams._cullMaxDistanceSq = SQUARED(camera.getZPlanes().y);

        return _renderPassCuller->frustumCull(cullParams, allNodes);
    }

    return _renderPassCuller->toVisibleNodes(camera, allNodes);
}

const VisibleNodeList& SceneManager::cullSceneGraph(RenderStage stage, const Camera& camera, I32 minLoD, const vec3<F32>& minExtents, I64* ignoredGUIDS, size_t ignoredGUIDSCount) {
    OPTICK_EVENT();

    Time::ScopedTimer timer(*_sceneGraphCullTimers[to_U32(stage)]);

    Scene& activeScene = getActiveScene();
    SceneState& sceneState = activeScene.state();

    NodeCullParams cullParams = {};
    cullParams._lodThresholds = sceneState.renderState().lodThresholds(stage);
    cullParams._minExtents = minExtents;
    cullParams._ignoredGUIDS = std::make_pair(ignoredGUIDS, ignoredGUIDSCount);
    cullParams._currentCamera = &camera;
    cullParams._cullMaxDistanceSq = std::numeric_limits<F32>::max();
    cullParams._minLoD = minLoD;
    cullParams._stage = stage;
    if (stage != RenderStage::SHADOW) {
        cullParams._cullMaxDistanceSq = SQUARED(sceneState.renderState().generalVisibility());
    } else {
        cullParams._cullMaxDistanceSq = std::min(cullParams._cullMaxDistanceSq, SQUARED(camera.getZPlanes().y));
    }

    return _renderPassCuller->frustumCull(cullParams, activeScene.sceneGraph(), sceneState, _parent.platformContext());
}

void SceneManager::prepareLightData(RenderStage stage, const vec3<F32>& cameraPos, const mat4<F32>& viewMatrix) {
    OPTICK_EVENT();

    if (stage != RenderStage::SHADOW) {
        getActiveScene().lightPool().prepareLightData(stage, cameraPos, viewMatrix);
    }
}

void SceneManager::onLostFocus() {
    if (!_init) {
        return;
    }

    getActiveScene().onLostFocus();
}

void SceneManager::onGainFocus() {
    if (!_init) {
        return;
    }

    getActiveScene().onGainFocus();
}

void SceneManager::resetSelection(PlayerIndex idx) {
    Attorney::SceneManager::resetSelection(getActiveScene(), idx);
    for (auto& cbk : _selectionChangeCallbacks) {
        cbk(idx, {});
    }
}

void SceneManager::setSelected(PlayerIndex idx, const vectorEASTL<SceneGraphNode*>& sgns) {
    Attorney::SceneManager::setSelected(getActiveScene(), idx, sgns);
    for (auto& cbk : _selectionChangeCallbacks) {
        cbk(idx, sgns);
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

    const Str128& sceneName = activeScene.resourceName();

    const Str256 path = Paths::g_saveLocation +  sceneName + "/";
    const char* saveFile = "current_save.sav";
    const char* bakSaveFile = "save.bak";

    bool isLoadFromBackup = false;
    // If file is missing, restore from bak
    if (!fileExists((path + saveFile).c_str())) {
        isLoadFromBackup = true;

        // Save file might be deleted if it was corrupted
        if (fileExists((path + bakSaveFile).c_str())) {
            copyFile(path.c_str(), bakSaveFile, path.c_str(), saveFile, false);
        }
    }

    ByteBuffer save;
    if (save.loadFromFile(path.c_str(), saveFile)) {
        if (!Attorney::SceneLoadSave::load(activeScene, save)) {
            //Remove the save and try the backup
            deleteFile(path.c_str(), saveFile);
            if (!isLoadFromBackup) {
                return loadScene(activeScene);
            }
        }
    }
    return false;
}

bool LoadSave::saveScene(const Scene& activeScene, bool toCache, DELEGATE<void, const char*> msgCallback, DELEGATE<void, bool> finishCallback) {
    if (!toCache) {
        return activeScene.saveXML(msgCallback, finishCallback);
    }

    bool ret = false;
    if (activeScene.state().saveLoadDisabled()) {
        ret = true;
    } else {
        const Str128& sceneName = activeScene.resourceName();
        const Str256 path = Paths::g_saveLocation + sceneName + "/";
        const char* saveFile = "current_save.sav";
        const char* bakSaveFile = "save.bak";

        if (fileExists((path + saveFile).c_str())) {
            copyFile(path.c_str(), saveFile, path.c_str(), bakSaveFile, true);
        }

        ByteBuffer save;
        if (Attorney::SceneLoadSave::save(activeScene, save)) {
            ret = save.dumpToFile(path.c_str(), saveFile);
        }
    }
    if (finishCallback) {
        finishCallback(ret);
    }
    return ret;
}

bool SceneManager::saveActiveScene(bool toCache, bool deferred, DELEGATE<void, const char*> msgCallback, DELEGATE<void, bool> finishCallback) {
    OPTICK_EVENT();

    const Scene& activeScene = getActiveScene();

    if (_saveTask != nullptr) {
        if (!Finished(*_saveTask)) {
            if (toCache) {
                return false;
            } else {
                if_constexpr(Config::Build::IS_DEBUG_BUILD) {
                    DebugBreak();
                }
            }
        }
        Wait(*_saveTask);
    }

    TaskPool& pool = parent().platformContext().taskPool(TaskPoolType::LOW_PRIORITY);
    _saveTask = CreateTask(pool,
                           nullptr,
                           [&activeScene, msgCallback, finishCallback, toCache](const Task& parentTask) {
                               LoadSave::saveScene(activeScene, toCache, msgCallback, finishCallback);
                           });
    Start(*_saveTask, deferred ? TaskPriority::DONT_CARE : TaskPriority::REALTIME);

    return true;
}

bool SceneManager::networkUpdate(U32 frameCount) {

    getActiveScene().sceneGraph();

    return true;
}

};