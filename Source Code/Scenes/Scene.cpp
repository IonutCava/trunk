#include "stdafx.h"

#include "Headers/Scene.h"

#include "Editor/Headers/Editor.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

#include "Utility/Headers/XMLParser.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

#include "Environment/Sky/Headers/Sky.h"
#include "Environment/Water/Headers/Water.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Predefined/Headers/Box3D.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"
#include "Geometry/Shapes/Predefined/Headers/Text3D.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIConsole.h"

#include "Dynamics/Entities/Units/Headers/Player.h"

#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Physics/Headers/PhysicsSceneInterface.h"


namespace Divide {

namespace {

vec3<F32> g_PlayerExtents(1.0f, 1.82f, 0.75f);

struct selectionQueueDistanceFrontToBack {
    selectionQueueDistanceFrontToBack(const vec3<F32>& eyePos)
        : _eyePos(eyePos) {}

    bool operator()(SceneGraphNode* a, SceneGraphNode* b) const {
        F32 dist_a =
            a->get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(_eyePos);
        F32 dist_b =
            b->get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(_eyePos);
        return dist_a > dist_b;
    }

   private:
    vec3<F32> _eyePos;
};

constexpr char* g_defaultPlayerName = "Player_%d";
};

Scene::Scene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
    : Resource(ResourceType::DEFAULT, name),
      PlatformContextComponent(context),
      _parent(parent),
      _resCache(cache),
      _LRSpeedFactor(5.0f),
      _loadComplete(false),
      _cookCollisionMeshesScheduled(false),
      _pxScene(nullptr),
      _paramHandler(ParamHandler::instance())
{
    _sceneTimerUS = 0UL;
    _sceneState = MemoryManager_NEW SceneState(*this);
    _input = MemoryManager_NEW SceneInput(*this, _context);
    _sceneGraph = MemoryManager_NEW SceneGraph(*this);
    _aiManager = MemoryManager_NEW AI::AIManager(*this, _context.kernel().taskPool());
    _lightPool = MemoryManager_NEW LightPool(*this, _context.gfx());
    _envProbePool = MemoryManager_NEW SceneEnvironmentProbePool(*this);

    _GUI = MemoryManager_NEW SceneGUIElements(*this, _context.gui());

    _loadingTasks = 0;

    if (Config::Build::IS_DEBUG_BUILD) {
        RenderStateBlock primitiveDescriptor;
        _linesPrimitive = _context.gfx().newIMP();
        _linesPrimitive->name("LinesRayPick");
        PipelineDescriptor pipeDesc;
        pipeDesc._stateHash = primitiveDescriptor.getHash();

        _linesPrimitive->pipeline(_context.gfx().newPipeline(pipeDesc));
        _linesPrimitive->paused(true);
    } else {
        _linesPrimitive = nullptr;
    }

}

Scene::~Scene()
{
    MemoryManager::DELETE(_sceneState);
    MemoryManager::DELETE(_input);
    MemoryManager::DELETE(_sceneGraph);
    MemoryManager::DELETE(_aiManager);
    MemoryManager::DELETE(_lightPool);
    MemoryManager::DELETE(_envProbePool);
    MemoryManager::DELETE(_GUI);
    for (IMPrimitive*& prim : _octreePrimitives) {
        MemoryManager::DELETE(prim);
    }
}

bool Scene::onStartup() {
    return true;
}

bool Scene::onShutdown() {
    return true;
}

bool Scene::frameStarted() {
    UniqueLock lk(_perFrameArenaMutex);
    _perFrameArena.clear();
    return true;
}

bool Scene::frameEnded() {
    return true;
}

bool Scene::idle() {  // Called when application is idle
    if (!_modelDataArray.empty()) {
        loadXMLAssets(true);
    }

    if (!_sceneGraph->getRoot().hasChildren()) {
        return false;
    }

    _sceneGraph->idle();

    Attorney::SceneRenderStateScene::playAnimations(renderState(), _context.config().debug.mesh.playAnimations);

    if (_cookCollisionMeshesScheduled && checkLoadFlag()) {
        if (_context.gfx().getFrameCount() > 1) {
            _sceneGraph->getRoot().get<RigidBodyComponent>()->cookCollisionMesh(_name);
            _cookCollisionMeshesScheduled = false;
        }
    }

    if (Config::Build::IS_DEBUG_BUILD) {
        _linesPrimitive->paused(!renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_DEBUG_LINES));
    }

    _lightPool->idle();

    WriteLock w_lock(_tasksMutex);
    _tasks.erase(std::remove_if(std::begin(_tasks), std::end(_tasks),
                 [](const TaskHandle& handle) -> bool { return !handle._task->isRunning(); }),
        std::end(_tasks));

    return true;
}

void Scene::addMusic(MusicType type, const stringImpl& name, const stringImpl& srcFile) {

    FileWithPath fileResult = splitPathToNameAndLocation(srcFile);
    const stringImpl& musicFile = fileResult._fileName;
    const stringImpl& musicFilePath = fileResult._path;

    ResourceDescriptor music(name);
    music.setResourceName(musicFile);
    music.setResourceLocation(musicFilePath);
    music.setFlag(true);
    hashAlg::insert(state().music(type),
                    _ID_RT(name),
                    CreateResource<AudioDescriptor>(_resCache, music));
}

void Scene::addPatch(vectorImpl<FileData>& data) {
}

void Scene::loadXMLAssets(bool singleStep) {
    constexpr bool terrainThreadedLoading = true;

    static const U32 normalMask = to_base(ComponentType::NAVIGATION) |
                                  to_base(ComponentType::TRANSFORM) |
                                  to_base(ComponentType::RIGID_BODY) |
                                  to_base(ComponentType::BOUNDS) |
                                  to_base(ComponentType::RENDERING) |
                                  to_base(ComponentType::NETWORKING);

    while (!_modelDataArray.empty()) {
        const FileData& it = _modelDataArray.top();
        if (it.type == GeometryType::VEGETATION) {
            _vegetationDataArray.push_back(loadModel(it, false));
        } else  if (it.type == GeometryType::PRIMITIVE) {
            loadGeometry(it, true);
        } else {
            loadModel(it, true);
        }
        _loadingTasks++;

        _modelDataArray.pop();

        if (singleStep) {
            return;
        }
    }

    auto registerTerrain = [this](Resource_wptr res) {
        SceneGraphNode& root = _sceneGraph->getRoot();
        SceneGraphNode* terrainTemp = root.addNode(std::dynamic_pointer_cast<Terrain>(res.lock()), normalMask, PhysicsGroup::GROUP_STATIC);
        terrainTemp->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);

        NavigationComponent* nComp = terrainTemp->get<NavigationComponent>();
        nComp->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);

        _loadingTasks--;
    };

    // Add terrain from XML
    while(!_terrainInfoArray.empty()) {
        const std::shared_ptr<TerrainDescriptor>& terrainInfo = _terrainInfoArray.back();
        ResourceDescriptor terrain(terrainInfo->getVariable("terrainName"));
        terrain.setPropertyDescriptor(*terrainInfo);
        terrain.setThreadedLoading(terrainThreadedLoading);
        terrain.setOnLoadCallback(registerTerrain);
        terrain.setFlag(terrainInfo->getActive());
        CreateResource<Terrain>(_resCache, terrain);
        _loadingTasks++;
        _terrainInfoArray.pop_back();

        if (singleStep) {
            return;
        }
    }
}

Mesh_ptr Scene::loadModel(const FileData& data, bool addToSceneGraph) {
    constexpr bool modelThreadedLoading = true;

    static const U32 normalMask = to_base(ComponentType::NAVIGATION) |
                                  to_base(ComponentType::TRANSFORM) |
                                  to_base(ComponentType::RIGID_BODY) |
                                  to_base(ComponentType::BOUNDS) |
                                  to_base(ComponentType::RENDERING) |
                                  to_base(ComponentType::NETWORKING);

    auto loadModelComplete = [this](Resource_wptr res) {
        ACKNOWLEDGE_UNUSED(res);
        _loadingTasks--;
    };

    ResourceDescriptor model(data.ModelName);
    model.setResourceLocation(data.ModelName);
    model.setFlag(true);
    model.setThreadedLoading(modelThreadedLoading);
    model.setOnLoadCallback(loadModelComplete);
    Mesh_ptr thisObj = CreateResource<Mesh>(_resCache, model);

    if (!thisObj) {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD_MODEL")), data.ModelName.c_str());
    } else {
        if (addToSceneGraph) {
            SceneGraphNode* meshNode =
                _sceneGraph->getRoot().addNode(thisObj,
                                               data.isUnit ? normalMask | to_base(ComponentType::UNIT) : normalMask,
                                               data.physicsUsage ? data.physicsStatic ? PhysicsGroup::GROUP_STATIC
                                                                                      : PhysicsGroup::GROUP_DYNAMIC
                                                                 : PhysicsGroup::GROUP_IGNORE,
                                               data.ItemName);
            meshNode->get<RenderingComponent>()->toggleRenderOption(RenderingComponent::RenderOptions::CAST_SHADOWS, data.castsShadows);
            meshNode->get<RenderingComponent>()->toggleRenderOption(RenderingComponent::RenderOptions::RECEIVE_SHADOWS, data.receivesShadows);
            meshNode->get<TransformComponent>()->setScale(data.scale);
            meshNode->get<TransformComponent>()->setRotation(data.orientation);
            meshNode->get<TransformComponent>()->setPosition(data.position);

            if (data.staticUsage) {
                meshNode->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
            }
            if (data.navigationUsage) {
                meshNode->get<NavigationComponent>()->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);
            }

            if (data.useHighDetailNavMesh) {
                meshNode->get<NavigationComponent>()->navigationDetailOverride(true);
            }
        }
    }

    return thisObj;
}

Object3D_ptr Scene::loadGeometry(const FileData& data, bool addToSceneGraph) {
    static const U32 normalMask = to_base(ComponentType::NAVIGATION) |
                                  to_base(ComponentType::TRANSFORM) |
                                  to_base(ComponentType::RIGID_BODY) |
                                  to_base(ComponentType::BOUNDS) |
                                  to_base(ComponentType::RENDERING) |
                                  to_base(ComponentType::NETWORKING);

    auto loadModelComplete = [this](Resource_wptr res) {
        ACKNOWLEDGE_UNUSED(res);
        _loadingTasks--;
    };

    std::shared_ptr<Object3D> thisObj;
    ResourceDescriptor item(data.ItemName);
    item.setOnLoadCallback(loadModelComplete);
    item.setResourceLocation(data.ModelName);
    if (data.ModelName.compare("Box3D") == 0) {
        item.setPropertyList(Util::StringFormat("%2.2f", data.data));
        thisObj = CreateResource<Box3D>(_resCache, item);
    } else if (data.ModelName.compare("Sphere3D") == 0) {
        thisObj = CreateResource<Sphere3D>(_resCache, item);
        static_cast<Sphere3D*>(thisObj.get())->setRadius(data.data);
    } else if (data.ModelName.compare("Quad3D") == 0) {
        vec3<F32> scale = data.scale;
        vec3<F32> position = data.position;
        P32 quadMask;
        quadMask.i = 0;
        quadMask.b[0] = 1;
        item.setBoolMask(quadMask);
        thisObj = CreateResource<Quad3D>(_resCache, item);
        static_cast<Quad3D*>(thisObj.get())->setCorner(Quad3D::CornerLocation::TOP_LEFT, vec3<F32>(0, 1, 0));
        static_cast<Quad3D*>(thisObj.get())->setCorner(Quad3D::CornerLocation::TOP_RIGHT, vec3<F32>(1, 1, 0));
        static_cast<Quad3D*>(thisObj.get())->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT, vec3<F32>(0, 0, 0));
        static_cast<Quad3D*>(thisObj.get())->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>(1, 0, 0));
    } else if (data.ModelName.compare("Text3D") == 0) {
        /// set font file
        item.setResourceLocation(data.data3);
        item.setPropertyList(data.data2);
        thisObj = CreateResource<Text3D>(_resCache, item);
        static_cast<Text3D*>(thisObj.get())->setWidth(data.data);
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_UNSUPPORTED_GEOM")),
                         data.ModelName.c_str());
        return false;
    }
    STUBBED("Load material from XML disabled for primitives! - Ionut")
    Material_ptr tempMaterial = nullptr; /* = XML::loadMaterial(data.ItemName + "_material")*/;
    if (!tempMaterial) {
        ResourceDescriptor materialDescriptor(data.ItemName + "_material");
        tempMaterial = CreateResource<Material>(_resCache, materialDescriptor);
        tempMaterial->setDiffuse(data.colour);
        tempMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    }

    thisObj->setMaterialTpl(tempMaterial);

    if (addToSceneGraph) {
        SceneGraphNode* thisObjSGN = _sceneGraph->getRoot().addNode(thisObj,
                                                                       normalMask,
                                                                       data.physicsUsage ? data.physicsStatic ? PhysicsGroup::GROUP_STATIC
                                                                                                              : PhysicsGroup::GROUP_DYNAMIC
                                                                                         : PhysicsGroup::GROUP_IGNORE);
        thisObjSGN->get<TransformComponent>()->setScale(data.scale);
        thisObjSGN->get<TransformComponent>()->setRotation(data.orientation);
        thisObjSGN->get<TransformComponent>()->setPosition(data.position);
        thisObjSGN->get<RenderingComponent>()->toggleRenderOption(RenderingComponent::RenderOptions::CAST_SHADOWS, data.castsShadows);
        thisObjSGN->get<RenderingComponent>()->toggleRenderOption(RenderingComponent::RenderOptions::RECEIVE_SHADOWS, data.receivesShadows);
        if (data.staticUsage) {
            thisObjSGN->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
        }
        if (data.navigationUsage) {
            thisObjSGN->get<NavigationComponent>()->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);
        }

        if (data.useHighDetailNavMesh) {
            thisObjSGN->get<NavigationComponent>()->navigationDetailOverride(true);
        }
    }

    return thisObj;
}

SceneGraphNode* Scene::addParticleEmitter(const stringImpl& name,
                                             std::shared_ptr<ParticleData> data,
                                             SceneGraphNode& parentNode) {
    static const U32 particleMask = to_base(ComponentType::TRANSFORM) |
                                    to_base(ComponentType::BOUNDS) |
                                    to_base(ComponentType::RENDERING) |
                                    to_base(ComponentType::NETWORKING);
    DIVIDE_ASSERT(!name.empty(),
                  "Scene::addParticleEmitter error: invalid name specified!");

    ResourceDescriptor particleEmitter(name);
    std::shared_ptr<ParticleEmitter> emitter = CreateResource<ParticleEmitter>(_resCache, particleEmitter);

    DIVIDE_ASSERT(emitter != nullptr,
                  "Scene::addParticleEmitter error: Could not instantiate emitter!");

    if (Runtime::isMainThread()) {
        emitter->initData(data);
    } else {
        _context.app().mainThreadTask([&emitter, &data] { emitter->initData(data); });
    }

    return parentNode.addNode(emitter, particleMask, PhysicsGroup::GROUP_IGNORE);
}


SceneGraphNode* Scene::addLight(LightType type,
                                   SceneGraphNode& parentNode) {
    static const U32 lightMask = to_base(ComponentType::TRANSFORM) |
                                 to_base(ComponentType::BOUNDS) |
                                 to_base(ComponentType::RENDERING) |
                                 to_base(ComponentType::NETWORKING);

    const char* lightType = "";
    switch (type) {
        case LightType::DIRECTIONAL:
            lightType = "_directional_light ";
            break;
        case LightType::POINT:
            lightType = "_point_light_";
            break;
        case LightType::SPOT:
            lightType = "_spot_light_";
            break;
    }

    ResourceDescriptor defaultLight(
        getName() +
        lightType +
        to_stringImpl(_lightPool->getLights(type).size()));

    defaultLight.setEnumValue(to_U32(type));
    defaultLight.setUserPtr(_lightPool);
    std::shared_ptr<Light> light = CreateResource<Light>(_resCache, defaultLight);
    if (type == LightType::DIRECTIONAL) {
        light->setCastShadows(true);
    }
    return parentNode.addNode(light, lightMask, PhysicsGroup::GROUP_IGNORE);
}

void Scene::toggleFlashlight(PlayerIndex idx) {
    static const U32 lightMask = to_base(ComponentType::TRANSFORM) |
                                 to_base(ComponentType::BOUNDS) |
                                 to_base(ComponentType::RENDERING) |
                                 to_base(ComponentType::NETWORKING);

    SceneGraphNode*& flashLight = _flashLight[idx];
    if (!flashLight) {
        ResourceDescriptor tempLightDesc(Util::StringFormat("Flashlight_%d", idx));
        tempLightDesc.setEnumValue(to_base(LightType::SPOT));
        tempLightDesc.setUserPtr(_lightPool);
        std::shared_ptr<Light> tempLight = CreateResource<Light>(_resCache, tempLightDesc);
        tempLight->setDrawImpostor(false);
        tempLight->setRange(30.0f);
        tempLight->setCastShadows(true);
        tempLight->setDiffuseColour(DefaultColours::WHITE);
        flashLight = _sceneGraph->getRoot().addNode(tempLight,
                                                    lightMask,
                                                    PhysicsGroup::GROUP_IGNORE);
        hashAlg::insert(_flashLight,
                        idx,
                        flashLight);
                     
    }

    flashLight->getNode<Light>()->toggleEnabled();
}

SceneGraphNode* Scene::addSky(const stringImpl& nodeName) {
    ResourceDescriptor skyDescriptor("Default Sky");
    skyDescriptor.setID(to_U32(std::floor(Camera::utilityCamera(Camera::UtilityCamera::DEFAULT)->getZPlanes().y * 2)));

    std::shared_ptr<Sky> skyItem = CreateResource<Sky>(_resCache, skyDescriptor);
    DIVIDE_ASSERT(skyItem != nullptr, "Scene::addSky error: Could not create sky resource!");

    static const U32 normalMask = 
        to_base(ComponentType::NAVIGATION) |
        to_base(ComponentType::TRANSFORM) |
        to_base(ComponentType::BOUNDS) |
        to_base(ComponentType::RENDERING) |
        to_base(ComponentType::NETWORKING);

    SceneGraphNode* skyNode = _sceneGraph->getRoot().addNode(skyItem,
                                                                normalMask,
                                                                PhysicsGroup::GROUP_IGNORE,
                                                                nodeName);
    skyNode->lockVisibility(true);

    return skyNode;
}

U16 Scene::registerInputActions() {
    _input->flushCache();

    auto none = [](InputParams param) {};
    auto deleteSelection = [this](InputParams param) { _sceneGraph->removeNode(_currentSelection[0]); };
    auto increaseCameraSpeed = [this](InputParams param){
        Camera& cam = _scenePlayers[getPlayerIndexForDevice(param._deviceIndex)]->getCamera();

        F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
        if (currentCamMoveSpeedFactor < 50) {
            cam.setMoveSpeedFactor(currentCamMoveSpeedFactor + 1.0f);
            cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() + 1.0f);
        }
    };
    auto decreaseCameraSpeed = [this](InputParams param) {
        Camera& cam = _scenePlayers[getPlayerIndexForDevice(param._deviceIndex)]->getCamera();

        F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
        if (currentCamMoveSpeedFactor > 1.0f) {
            cam.setMoveSpeedFactor(currentCamMoveSpeedFactor - 1.0f);
            cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() - 1.0f);
        }
    };
    auto increaseResolution = [this](InputParams param) {_context.gfx().increaseResolution();};
    auto decreaseResolution = [this](InputParams param) {_context.gfx().decreaseResolution();};
    auto moveForward = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::POSITIVE);};
    auto moveBackwards = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NEGATIVE);};
    auto stopMoveFWDBCK = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NONE);};
    auto strafeLeft = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NEGATIVE);};
    auto strafeRight = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::POSITIVE);};
    auto stopStrafeLeftRight = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NONE);};
    auto rollCCW = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).roll(MoveDirection::POSITIVE);};
    auto rollCW = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).roll(MoveDirection::NEGATIVE);};
    auto stopRollCCWCW = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).roll(MoveDirection::NONE);};
    auto turnLeft = [this](InputParams param) { state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::NEGATIVE);};
    auto turnRight = [this](InputParams param) { state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::POSITIVE);};
    auto stopTurnLeftRight = [this](InputParams param) { state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::NONE);};
    auto turnUp = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::NEGATIVE);};
    auto turnDown = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::POSITIVE);};
    auto stopTurnUpDown = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::NONE);};
    auto togglePauseState = [](InputParams param){
        ParamHandler& par = ParamHandler::instance();
        par.setParam(_ID("freezeLoopTime"), !par.getParam(_ID("freezeLoopTime"), false));
    };
    auto toggleDepthOfField = [](InputParams param) {
        PostFX& postFX = PostFX::instance();
        if (postFX.getFilterState(FilterType::FILTER_DEPTH_OF_FIELD)) {
            postFX.popFilter(FilterType::FILTER_DEPTH_OF_FIELD);
        } else {
            postFX.pushFilter(FilterType::FILTER_DEPTH_OF_FIELD);
        }
    };
    auto toggleBloom = [](InputParams param) {
        PostFX& postFX = PostFX::instance();
        if (postFX.getFilterState(FilterType::FILTER_BLOOM)) {
            postFX.popFilter(FilterType::FILTER_BLOOM);
        } else {
            postFX.pushFilter(FilterType::FILTER_BLOOM);
        }
    };
    auto toggleSkeletonRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_SKELETONS);};
    auto toggleAxisLineRendering = [this](InputParams param) {renderState().toggleAxisLines();};
    auto toggleWireframeRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME);};
    auto toggleGeometryRendering = [this](InputParams param) { renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY);};
    auto toggleDebugLines = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_DEBUG_LINES);};
    auto toggleBoundingBoxRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_AABB);};
    auto toggleShadowMapDepthBufferPreview = [this](InputParams param) {
        LightPool::togglePreviewShadowMaps(_context.gfx());

        ParamHandler& par = ParamHandler::instance();
        par.setParam<bool>(_ID("rendering.previewDebugViews"),
                          !par.getParam<bool>(_ID("rendering.previewDebugViews"), false));
    };
    auto takeScreenshot = [this](InputParams param) { _context.gfx().Screenshot("screenshot_"); };
    auto toggleFullScreen = [this](InputParams param) { _context.gfx().toggleFullScreen(); };
    auto toggleFlashLight = [this](InputParams param) { toggleFlashlight(getPlayerIndexForDevice(param._deviceIndex)); };
    auto toggleOctreeRegionRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_OCTREE_REGIONS);};
    auto select = [this](InputParams  param) {findSelection(getPlayerIndexForDevice(param._deviceIndex)); };
    auto lockCameraToMouse = [this](InputParams  param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).cameraLockedToMouse(true); };
    auto releaseCameraFromMouse = [this](InputParams  param) {
        state().playerState(getPlayerIndexForDevice(param._deviceIndex)).cameraLockedToMouse(false);
        state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::NONE);
        state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::NONE);
    };
    auto rendererDebugView = [this](InputParams param) {_context.gfx().getRenderer().toggleDebugView();};
    auto shutdown = [this](InputParams param) { _context.app().RequestShutdown();};
    auto povNavigation = [this](InputParams param) {
        if (param._var[0] & OIS::Pov::North) {  // Going up
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::POSITIVE);
        }
        if (param._var[0] & OIS::Pov::South) {  // Going down
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NEGATIVE);
        }
        if (param._var[0] & OIS::Pov::East) {  // Going right
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::POSITIVE);
        }
        if (param._var[0] & OIS::Pov::West) {  // Going left
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NEGATIVE);
        }
        if (param._var[0] == OIS::Pov::Centered) {  // stopped/centered out
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NONE);
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NONE);
        }
    };

    auto axisNavigation = [this](InputParams param) {
        I32 axis = param._var[2];
        Input::Joystick joystick = static_cast<Input::Joystick>(param._var[3]);

        Input::JoystickInterface* joyInterface = _context.input().getJoystickInterface();
        
        const Input::JoystickData& joyData = joyInterface->getJoystickData(joystick);
        I32 deadZone = joyData._deadZone;
        I32 axisABS = std::min(param._var[0], joyData._max);

        switch (axis) {
            case 0: {
                if (axisABS > deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::POSITIVE);
                } else if (axisABS < -deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::NEGATIVE);
                } else {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::NONE);
                }
            } break;
            case 1: {
                if (axisABS > deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::POSITIVE);
                } else if (axisABS < -deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::NEGATIVE);
                } else {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::NONE);
                }
            } break;

            case 2: {
                if (axisABS < -deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::POSITIVE);
                } else if (axisABS > deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NEGATIVE);
                } else {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NONE);
                }
            } break;
            case 3: {
                if (axisABS < -deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NEGATIVE);
                } else if (axisABS > deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::POSITIVE);
                } else {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NONE);
                }
            } break;
        }
    };

    auto toggleAntTweakBar = [this](InputParams param) {
        _context.config().gui.enableDebugVariableControls =
            !_context.config().gui.enableDebugVariableControls;
    };

    auto toggleEditor = [this](InputParams param) {
        if (Config::Build::IS_DEBUG_BUILD) {
            _context.editor().toggle(!_context.editor().running());
        }
    };

    auto toggleConsole = [this](InputParams param) {
        if (Config::Build::IS_DEBUG_BUILD) {
            _context.gui().getConsole().setVisible(!_context.gui().getConsole().isVisible());
        }
    };

    U16 actionID = 0;
    InputActionList& actions = _input->actionList();
    actions.registerInputAction(actionID++, none);
    actions.registerInputAction(actionID++, deleteSelection);
    actions.registerInputAction(actionID++, increaseCameraSpeed);
    actions.registerInputAction(actionID++, decreaseCameraSpeed);
    actions.registerInputAction(actionID++, increaseResolution);
    actions.registerInputAction(actionID++, decreaseResolution);
    actions.registerInputAction(actionID++, moveForward);
    actions.registerInputAction(actionID++, moveBackwards);
    actions.registerInputAction(actionID++, stopMoveFWDBCK);
    actions.registerInputAction(actionID++, strafeLeft);
    actions.registerInputAction(actionID++, strafeRight);
    actions.registerInputAction(actionID++, stopStrafeLeftRight);
    actions.registerInputAction(actionID++, rollCCW);
    actions.registerInputAction(actionID++, rollCW);
    actions.registerInputAction(actionID++, stopRollCCWCW);
    actions.registerInputAction(actionID++, turnLeft);
    actions.registerInputAction(actionID++, turnRight);
    actions.registerInputAction(actionID++, stopTurnLeftRight);
    actions.registerInputAction(actionID++, turnUp);
    actions.registerInputAction(actionID++, turnDown);
    actions.registerInputAction(actionID++, stopTurnUpDown);
    actions.registerInputAction(actionID++, togglePauseState);
    actions.registerInputAction(actionID++, toggleDepthOfField);
    actions.registerInputAction(actionID++, toggleBloom);
    actions.registerInputAction(actionID++, toggleSkeletonRendering);
    actions.registerInputAction(actionID++, toggleAxisLineRendering);
    actions.registerInputAction(actionID++, toggleWireframeRendering);
    actions.registerInputAction(actionID++, toggleGeometryRendering);
    actions.registerInputAction(actionID++, toggleDebugLines);
    actions.registerInputAction(actionID++, toggleBoundingBoxRendering);
    actions.registerInputAction(actionID++, toggleShadowMapDepthBufferPreview);
    actions.registerInputAction(actionID++, takeScreenshot);
    actions.registerInputAction(actionID++, toggleFullScreen);
    actions.registerInputAction(actionID++, toggleFlashLight);
    actions.registerInputAction(actionID++, toggleOctreeRegionRendering);
    actions.registerInputAction(actionID++, select);
    actions.registerInputAction(actionID++, lockCameraToMouse);
    actions.registerInputAction(actionID++, releaseCameraFromMouse);
    actions.registerInputAction(actionID++, rendererDebugView);
    actions.registerInputAction(actionID++, shutdown);
    actions.registerInputAction(actionID++, povNavigation);
    actions.registerInputAction(actionID++, axisNavigation);
    actions.registerInputAction(actionID++, toggleAntTweakBar);
    actions.registerInputAction(actionID++, toggleEditor);
    actions.registerInputAction(actionID++, toggleConsole);
    
    return actionID;
}

void Scene::loadKeyBindings() {
    XML::loadDefaultKeybindings(Paths::g_xmlDataLocation + "keyBindings.xml", this);
}

void Scene::loadDefaultCamera() {
    Camera* baseCamera = Camera::utilityCamera(Camera::UtilityCamera::DEFAULT);
    

    // Camera position is overridden in the scene's XML configuration file
    if (ParamHandler::instance().getParam<bool>(_ID_RT((getName() + ".options.cameraStartPositionOverride").c_str()))) {
        baseCamera->setEye(vec3<F32>(
            _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartPosition.x").c_str())),
            _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartPosition.y").c_str())),
            _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartPosition.z").c_str()))));
        vec2<F32> camOrientation(_paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartOrientation.xOffsetDegrees").c_str())),
            _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartOrientation.yOffsetDegrees").c_str())));
        baseCamera->setGlobalRotation(camOrientation.y /*yaw*/, camOrientation.x /*pitch*/);
    } else {
        baseCamera->setEye(vec3<F32>(0, 50, 0));
    }

    baseCamera->setMoveSpeedFactor(_paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraSpeed.move").c_str()), 1.0f));
    baseCamera->setTurnSpeedFactor(_paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraSpeed.turn").c_str()), 1.0f));
    baseCamera->setProjection(_context.gfx().renderingData().aspectRatio(),
                              _context.config().runtime.verticalFOV,
                              vec2<F32>(_context.config().runtime.zNear, _context.config().runtime.zFar));

}

bool Scene::load(const stringImpl& name) {
    setState(ResourceState::RES_LOADING);

    STUBBED("ToDo: load skyboxes from XML")
    _name = name;

    loadDefaultCamera();
    loadXMLAssets();
    addSelectionCallback([this](U8 pIndex){_context.gui().selectionChangeCallback(this, pIndex);});

    U32 totalLoadingTasks = _loadingTasks;
    Console::d_printfn(Locale::get(_ID("SCENE_LOAD_TASKS")), totalLoadingTasks);
    while (totalLoadingTasks > 0) {
        if (totalLoadingTasks != _loadingTasks) {
            totalLoadingTasks = _loadingTasks;
            Console::d_printfn(Locale::get(_ID("SCENE_LOAD_TASKS")), totalLoadingTasks);
        }
        idle();
    }

    _loadComplete = true;
    return _loadComplete;
}

bool Scene::unload() {
    // prevent double unload calls%
    if (!checkLoadFlag()) {
        return false;
    }

    U32 totalLoadingTasks = _loadingTasks;
    while (totalLoadingTasks > 0) {
        if (totalLoadingTasks != _loadingTasks) {
            totalLoadingTasks = _loadingTasks;
            Console::d_printfn(Locale::get(_ID("SCENE_LOAD_TASKS")), totalLoadingTasks);
        }
        std::this_thread::yield();
    }

    clearTasks();
    /// Destroy physics (:D)
    _pxScene->release();
    MemoryManager::DELETE(_pxScene);
    _context.pfx().setPhysicsScene(nullptr);
    clearObjects();
    _loadComplete = false;
    assert(_scenePlayers.empty());

    return true;
}

bool Scene::loadResources(bool continueOnErrors) {
    return true;
}

void Scene::postLoad() {
    _sceneGraph->postLoad();
    Console::printfn(Locale::get(_ID("CREATE_AI_ENTITIES_START")));
    initializeAI(true);
    Console::printfn(Locale::get(_ID("CREATE_AI_ENTITIES_END")));
}

void Scene::postLoadMainThread() {
    assert(Runtime::isMainThread());
    setState(ResourceState::RES_LOADED);
}

void Scene::rebuildShaders() {
    SceneGraphNode* selection(_currentSelection[0]);
    if (selection != nullptr) {
        selection->get<RenderingComponent>()->rebuildMaterial();
    } else {
        ShaderProgram::rebuildAllShaders();
    }
}

stringImpl Scene::getPlayerSGNName(PlayerIndex idx) {
    return Util::StringFormat(g_defaultPlayerName, idx + 1);
}

void Scene::currentPlayerPass(PlayerIndex idx) {
    renderState().playerPass(idx);

    if (state().playerState(idx).cameraUnderwater()) {
        PostFX::instance().pushFilter(FilterType::FILTER_UNDERWATER);
    } else {
        PostFX::instance().popFilter(FilterType::FILTER_UNDERWATER);
    }
}

void Scene::onSetActive() {
    _context.pfx().setPhysicsScene(_pxScene);
    _aiManager->pauseUpdate(false);

    input().onSetActive();
    _context.sfx().stopMusic();
    _context.sfx().dumpPlaylists();

    for (U32 i = 0; i < to_base(MusicType::COUNT); ++i) {
        const SceneState::MusicPlaylist& playlist = state().music(static_cast<MusicType>(i));
        if (!playlist.empty()) {
            for (const SceneState::MusicPlaylist::value_type& song : playlist) {
                _context.sfx().addMusic(i, song.second);
            }
        }
    }
    _context.sfx().playMusic(0);

    assert(_parent.getPlayers().empty());
    addPlayerInternal(false);
}

void Scene::onRemoveActive() {
    _aiManager->pauseUpdate(true);

    while(!_scenePlayers.empty()) {
        _parent.removePlayer(*this, _scenePlayers.back(), false);
    }

    input().onRemoveActive();
}

void Scene::addPlayerInternal(bool queue) {
    stringImpl playerName = getPlayerSGNName(to_U8(_parent.getPlayers().size()));
    
    SceneGraphNode* playerSGN(_sceneGraph->findNode(playerName));
    if (!playerSGN) {
        SceneGraphNode& root = _sceneGraph->getRoot();
        playerSGN = root.addNode(SceneNode_ptr(MemoryManager_NEW SceneTransform(_resCache, 12345678 + _parent.getPlayers().size(), g_PlayerExtents)),
                                to_base(ComponentType::NAVIGATION) |
                                to_base(ComponentType::TRANSFORM) |
                                to_base(ComponentType::BOUNDS) |
                                to_base(ComponentType::UNIT) |
                                to_base(ComponentType::NETWORKING),
                                PhysicsGroup::GROUP_KINEMATIC,
                                playerName);
        _parent.addPlayer(*this, playerSGN, queue);
    } else {
        assert(playerSGN->get<UnitComponent>()->getUnit() != nullptr);
    }
}

void Scene::removePlayerInternal(PlayerIndex idx) {
    assert(idx < _scenePlayers.size());
    
    _parent.removePlayer(*this, _scenePlayers[getSceneIndexForPlayer(idx)], true);
}

void Scene::onPlayerAdd(const Player_ptr& player) {
    _scenePlayers.push_back(player);
    state().onPlayerAdd(player->index());
    input().onPlayerAdd(player->index());
}

void Scene::onPlayerRemove(const Player_ptr& player) {
    PlayerIndex idx = player->index();

    input().onPlayerRemove(idx);
    state().onPlayerRemove(idx);

    _sceneGraph->getRoot().removeNode(*player->getBoundNode());

    _scenePlayers.erase(std::cbegin(_scenePlayers) + getSceneIndexForPlayer(idx));
}

U8 Scene::getSceneIndexForPlayer(PlayerIndex idx) const {
    for (U8 i = 0; i < to_U8(_scenePlayers.size()); ++i) {
        if (_scenePlayers[i]->index() == idx) {
            return i;
        }
    }

    DIVIDE_UNEXPECTED_CALL("Player not found!");
    return 0;
}

const Player_ptr& Scene::getPlayerForIndex(PlayerIndex idx) const {
    return _scenePlayers[getSceneIndexForPlayer(idx)];
}

U8 Scene::getPlayerIndexForDevice(U8 deviceIndex) const {
    return input().getPlayerIndexForDevice(deviceIndex);
}

bool Scene::loadPhysics(bool continueOnErrors) {
    if (_pxScene == nullptr) {
        _pxScene = _context.pfx().NewSceneInterface(*this);
        _pxScene->init();
    }

    // Cook geometry
    if (_paramHandler.getParam<bool>(_ID((getName() + ".options.autoCookPhysicsAssets").c_str()), true)) {
        _cookCollisionMeshesScheduled = true;
    }
    return true;
}

bool Scene::initializeAI(bool continueOnErrors) {
    _aiManager->initialize();
    return true;
}

 /// Shut down AIManager thread
bool Scene::deinitializeAI(bool continueOnErrors) { 
    _aiManager->stop();
    WAIT_FOR_CONDITION(!_aiManager->running());

    return true;
}

void Scene::clearObjects() {
    while (!_modelDataArray.empty()) {
        _modelDataArray.pop();
    }
    _vegetationDataArray.clear();
    _flashLight.clear();
    _sceneGraph->unload();
}

bool Scene::mouseMoved(const Input::MouseEvent& arg) {
    // ToDo: Use mapping between device ID an player index -Ionut
    PlayerIndex idx = getPlayerIndexForDevice(arg._deviceIndex);

    Camera& cam = _scenePlayers[idx]->getCamera();
    if (cam.moveRelative(arg.relativePos()))
    {
        if (cam.getType() == Camera::CameraType::THIRD_PERSON) {
            _context.app().windowManager().snapCursorToCenter();
        }
        return true;
    }

    return false;
}

bool Scene::updateCameraControls(PlayerIndex idx) {
    Camera& cam = getPlayerForIndex(idx)->getCamera();
    
    SceneStatePerPlayer& playerState = state().playerState(idx);

    playerState.cameraUpdated(false);
    switch (cam.getType()) {
        default:
        case Camera::CameraType::FREE_FLY: {
            if (playerState.angleLR() != MoveDirection::NONE) {
                cam.rotateYaw(Angle::DEGREES<F32>(playerState.angleLR()));
                playerState.cameraUpdated(true);
            }
            if (playerState.angleUD() != MoveDirection::NONE) {
                cam.rotatePitch(Angle::DEGREES<F32>(playerState.angleUD()));
                playerState.cameraUpdated(true);
            }
            if (playerState.roll() != MoveDirection::NONE) {
                cam.rotateRoll(Angle::DEGREES<F32>(playerState.roll()));
                playerState.cameraUpdated(true);
            }
            if (playerState.moveFB() != MoveDirection::NONE) {
                cam.moveForward(to_F32(playerState.moveFB()));
                playerState.cameraUpdated(true);
            }
            if (playerState.moveLR() != MoveDirection::NONE) {
                cam.moveStrafe(to_F32(playerState.moveLR()));
                playerState.cameraUpdated(true);
            }
        } break;
    }

    playerState.cameraUnderwater(checkCameraUnderwater(idx));

    return playerState.cameraUpdated();
}

void Scene::updateSceneState(const U64 deltaTimeUS) {
    _sceneTimerUS += deltaTimeUS;
    updateSceneStateInternal(deltaTimeUS);
    _sceneGraph->sceneUpdate(deltaTimeUS, *_sceneState);
    _aiManager->update(deltaTimeUS);

    for (U8 i = 0; i < to_U8(_scenePlayers.size()); ++i) {
        PlayerIndex idx = _scenePlayers[i]->index();
        findHoverTarget(idx);
        if (_flashLight[idx]) {
            TransformComponent* tComp = _flashLight[idx]->get<TransformComponent>();
            const Camera& cam = _scenePlayers[idx]->getCamera();
            tComp->setPosition(cam.getEye());
            tComp->setRotation(cam.getEuler());
        }
    }
}

void Scene::onLostFocus() {
    for (U8 i = 0; i < to_U8(_scenePlayers.size()); ++i) {
        state().playerState(_scenePlayers[i]->index()).resetMovement();
    }

    //_paramHandler.setParam(_ID("freezeLoopTime"), true);
}

I64 Scene::registerTask(const TaskHandle& taskItem, bool start, U32 flags, Task::TaskPriority priority) {
    WriteLock w_lock(_tasksMutex);
    _tasks.push_back(taskItem);
    if (start) {
        _tasks.back().startTask(priority, flags);
    }
    return taskItem._jobIdentifier;
}

void Scene::clearTasks() {
    Console::printfn(Locale::get(_ID("STOP_SCENE_TASKS")));
    // Performance shouldn't be an issue here
    WriteLock w_lock(_tasksMutex);
    for (TaskHandle& task : _tasks) {
        if (task._task->jobIdentifier() == task._jobIdentifier) {
            task._task->stopTask();
            task.wait();
        }
    }

    _tasks.clear();
}

void Scene::removeTask(I64 jobIdentifier) {
    WriteLock w_lock(_tasksMutex);
    vectorImpl<TaskHandle>::iterator it;
    for (it = std::begin(_tasks); it != std::end(_tasks); ++it) {
        if ((*it)._task->jobIdentifier() == jobIdentifier) {
            (*it)._task->stopTask();
            _tasks.erase(it);
            (*it).wait();
            return;
        }
    }

}

void Scene::processInput(PlayerIndex idx, const U64 deltaTimeUS) {
}

void Scene::processGUI(const U64 deltaTimeUS) {
    for (U16 i = 0; i < _guiTimersMS.size(); ++i) {
        _guiTimersMS[i] += Time::MicrosecondsToMilliseconds<D64>(deltaTimeUS);
    }
}

void Scene::processTasks(const U64 deltaTimeUS) {
    for (U16 i = 0; i < _taskTimers.size(); ++i) {
        _taskTimers[i] += Time::MicrosecondsToMilliseconds<D64>(deltaTimeUS);
    }
}

void Scene::debugDraw(const Camera& activeCamera, const RenderStagePass& stagePass, GFX::CommandBuffer& bufferInOut) {
    if (Config::Build::IS_DEBUG_BUILD) {
        const SceneRenderState::GizmoState& currentGizmoState = renderState().gizmoState();

        if (currentGizmoState == SceneRenderState::GizmoState::SELECTED_GIZMO) {
            SceneGraphNode* selection(_currentSelection[0]);
            if (selection != nullptr) {
                selection->get<RenderingComponent>()->drawDebugAxis();
            }
        }

        if (renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_OCTREE_REGIONS)) {
            for (IMPrimitive* prim : _octreePrimitives) {
                prim->paused(true);
            }

            _octreeBoundingBoxes.resize(0);
            sceneGraph().getOctree().getAllRegions(_octreeBoundingBoxes);

            size_t primitiveCount = _octreePrimitives.size();
            size_t regionCount = _octreeBoundingBoxes.size();
            if (regionCount > primitiveCount) {
                size_t diff = regionCount - primitiveCount;
                for (size_t i = 0; i < diff; ++i) {
                    _octreePrimitives.push_back(_context.gfx().newIMP());
                }
            }

            assert(_octreePrimitives.size() >= _octreeBoundingBoxes.size());

            for (size_t i = 0; i < regionCount; ++i) {
                const BoundingBox& box = _octreeBoundingBoxes[i];
                _octreePrimitives[i]->fromBox(box.getMin(), box.getMax(), UColour(255, 0, 255, 255));
                bufferInOut.add(_octreePrimitives[i]->toCommandBuffer());
            }
        }
    }
    if (Config::Build::IS_DEBUG_BUILD) {
        bufferInOut.add(_linesPrimitive->toCommandBuffer());
    }
    // Show NavMeshes
    _aiManager->debugDraw(bufferInOut, false);
    _lightPool->drawLightImpostors(bufferInOut);
    _envProbePool->debugDraw(bufferInOut);
}

bool Scene::checkCameraUnderwater(PlayerIndex idx) const {
    const vectorImpl<SceneGraphNode*>& waterBodies = _sceneGraph->getNodesByType(SceneNodeType::TYPE_WATER);

    if (!waterBodies.empty()) {
        const Camera& crtCamera = getPlayerForIndex(idx)->getCamera();
        const vec3<F32>& eyePos = crtCamera.getEye();

        for (SceneGraphNode* node : waterBodies) {
            if (node->getNode<WaterPlane>()->pointUnderwater(*node, eyePos)) {
                return true;
            }
        }
    }

    return false;
}

void Scene::findHoverTarget(PlayerIndex idx) {
    const Camera& crtCamera = getPlayerForIndex(idx)->getCamera();

    const vec2<U16>& displaySize = _context.app().windowManager().getActiveWindow().getDimensions();
    const vec2<F32>& zPlanes = crtCamera.getZPlanes();
    const vec2<I32>& aimPos = state().playerState(idx).aimPos();

    F32 aimX = to_F32(aimPos.x);
    F32 aimY = displaySize.height - to_F32(aimPos.y) - 1;

    const Rect<I32>& viewport = _context.gfx().getCurrentViewport();
    vec3<F32> startRay = crtCamera.unProject(aimX, aimY, 0.0f, viewport);
    vec3<F32> endRay = crtCamera.unProject(aimX, aimY, 1.0f, viewport);
    // see if we select another one
    _sceneSelectionCandidates.clear();
    // get the list of visible nodes (use DEPTH_PASS because the nodes are sorted by depth, front to back)
    RenderPassCuller::VisibleNodeList& nodes = _parent.getVisibleNodesCache(RenderStage::DISPLAY);

    // Cast the picking ray and find items between the nearPlane and far Plane
    Ray mouseRay(startRay, startRay.direction(endRay));
    for (RenderPassCuller::VisibleNode& node : nodes) {
        const SceneGraphNode* nodePtr = node.second;
        if (nodePtr) {
            nodePtr->intersect(mouseRay, zPlanes.x, zPlanes.y, _sceneSelectionCandidates, false);                   
        }
    }

    if (!_sceneSelectionCandidates.empty()) {
        _currentHoverTarget[idx] = _sceneSelectionCandidates.front();

        SceneGraphNode* target = _sceneGraph->findNode(_currentHoverTarget[idx]);

        const std::shared_ptr<SceneNode>& node = target->getNode();
        if (node->getType() == SceneNodeType::TYPE_OBJECT3D) {
            if (static_cast<Object3D*>(node.get())->getObjectType() == Object3D::ObjectType::SUBMESH) {
                _currentHoverTarget[idx] = target->getParent()->getGUID();
            }
        }

         SceneGraphNode* sgn = _sceneGraph->findNode(_currentHoverTarget[idx]);
        if (sgn->getSelectionFlag() != SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
            sgn->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_HOVER);
        }
    } else {
        SceneGraphNode* target(_sceneGraph->findNode(_currentHoverTarget[idx]));
        if (target) {
            if (target->getSelectionFlag() != SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
                target->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_NONE);
            }
            _currentHoverTarget[idx] = 0;
        }
    }
}

void Scene::resetSelection() {
    if (_currentSelection[0]) {
        _currentSelection[0]->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_NONE);
    }

    _currentSelection[0] = nullptr;
}

void Scene::findSelection(PlayerIndex idx) {
    bool hadTarget = _currentSelection[idx] != nullptr;
    bool haveTarget = _currentHoverTarget[idx] != 0;

    I64 crtGUID = hadTarget ? _currentSelection[idx]->getGUID() : -1;
    I64 GUID = haveTarget ? _currentHoverTarget[idx] : -1;

    if (crtGUID != GUID) {
        resetSelection();

        if (haveTarget) {
            _currentSelection[idx] = _sceneGraph->findNode(_currentHoverTarget[idx]);
            _currentSelection[idx]->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_SELECTED);
        }

        for (DELEGATE_CBK<void, U8>& cbk : _selectionChangeCallbacks) {
            cbk(idx);
        }
    }
}

bool Scene::save(ByteBuffer& outputBuffer) const {
    U8 playerCount = to_U8(_scenePlayers.size());
    outputBuffer << playerCount;
    for (U8 i = 0; i < playerCount; ++i) {
        const Camera& cam = _scenePlayers[i]->getCamera();
        outputBuffer << _scenePlayers[i]->index() << cam.getEye() << cam.getEuler();
    }

    return true;
}

bool Scene::load(ByteBuffer& inputBuffer) {
    if (!inputBuffer.empty()) {
        vec3<F32> camPos;
        vec3<F32> camEuler;
        U8 currentPlayerIndex = 0;
        U8 currentPlayerCount = to_U8(_scenePlayers.size());

        U8 previousPlayerCount = 0;
        inputBuffer >> previousPlayerCount;
        for (U8 i = 0; i < previousPlayerCount; ++i) {
            inputBuffer >> currentPlayerIndex >> camPos >> camEuler;
            if (currentPlayerIndex < currentPlayerCount) {
                Camera& cam = _scenePlayers[currentPlayerIndex]->getCamera();
                cam.setEye(camPos);
                cam.setGlobalRotation(-camEuler);
            }
        }
    }

    return true;
}

Camera* Scene::playerCamera() const {
    return Attorney::SceneManagerCameraAccessor::playerCamera(_parent);
}

Camera* Scene::playerCamera(U8 index) const {
    return Attorney::SceneManagerCameraAccessor::playerCamera(_parent, index);
}

};