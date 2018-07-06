#include "stdafx.h"

#include "Headers/Scene.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Math/Headers/Transform.h"

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
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIConsole.h"
#include "GUI/GUIEditor/Headers/GUIEditor.h"

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

    bool operator()(const SceneGraphNode_wptr& a, const SceneGraphNode_wptr& b) const {
        F32 dist_a =
            a.lock()->get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(_eyePos);
        F32 dist_b =
            b.lock()->get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(_eyePos);
        return dist_a > dist_b;
    }

   private:
    vec3<F32> _eyePos;
};

constexpr char* g_defaultPlayerName = "Player_%d";
};

Scene::Scene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
    : Resource(ResourceType::DEFAULT, name),
      _context(context),
      _parent(parent),
      _resCache(cache),
      _LRSpeedFactor(5.0f),
      _loadComplete(false),
      _cookCollisionMeshesScheduled(false),
      _pxScene(nullptr),
      _baseCamera(nullptr),
      _paramHandler(ParamHandler::instance())
{
    _sceneTimer = 0UL;
    _sceneState = MemoryManager_NEW SceneState(*this);
    _input = MemoryManager_NEW SceneInput(*this, _context.input());
    _sceneGraph = MemoryManager_NEW SceneGraph(*this);
    _aiManager = MemoryManager_NEW AI::AIManager(*this, _context.input().parent().taskPool());
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
    std::unique_lock<std::mutex> lk(_perFrameArenaMutex);
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
            _sceneGraph->getRoot().get<PhysicsComponent>()->cookCollisionMesh(_name);
            _cookCollisionMeshesScheduled = false;
        }
    }

    if (Config::Build::IS_DEBUG_BUILD) {
        _linesPrimitive->paused(!renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_DEBUG_LINES));
    }

    _lightPool->idle();

    WriteLock w_lock(_tasksMutex);
    _tasks.erase(std::remove_if(std::begin(_tasks), std::end(_tasks),
                 [](const TaskHandle& handle) -> bool { return handle._task->finished(); }),
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
    hashAlg::emplace(state().music(type),
                     _ID_RT(name),
                     CreateResource<AudioDescriptor>(_resCache, music));
}

void Scene::addPatch(vectorImpl<FileData>& data) {
}

void Scene::loadXMLAssets(bool singleStep) {
    constexpr bool terrainThreadedLoading = true;

    static const U32 normalMask = to_base(SGNComponent::ComponentType::NAVIGATION) |
                                  to_base(SGNComponent::ComponentType::PHYSICS) |
                                  to_base(SGNComponent::ComponentType::BOUNDS) |
                                  to_base(SGNComponent::ComponentType::RENDERING) |
                                  to_base(SGNComponent::ComponentType::NETWORKING);

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
        SceneGraphNode_ptr terrainTemp = root.addNode(std::dynamic_pointer_cast<Terrain>(res.lock()), normalMask, PhysicsGroup::GROUP_STATIC);
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

    static const U32 normalMask = to_base(SGNComponent::ComponentType::NAVIGATION) |
                                  to_base(SGNComponent::ComponentType::PHYSICS) |
                                  to_base(SGNComponent::ComponentType::BOUNDS) |
                                  to_base(SGNComponent::ComponentType::RENDERING) |
                                  to_base(SGNComponent::ComponentType::NETWORKING);

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
            SceneGraphNode_ptr meshNode =
                _sceneGraph->getRoot().addNode(thisObj,
                                               data.isUnit ? normalMask | to_base(SGNComponent::ComponentType::UNIT) : normalMask,
                                               data.physicsUsage ? data.physicsStatic ? PhysicsGroup::GROUP_STATIC
                                                                                      : PhysicsGroup::GROUP_DYNAMIC
                                                                 : PhysicsGroup::GROUP_IGNORE,
                                               data.ItemName);
            meshNode->get<RenderingComponent>()->castsShadows(data.castsShadows);
            meshNode->get<RenderingComponent>()->receivesShadows(data.receivesShadows);
            meshNode->get<PhysicsComponent>()->setScale(data.scale);
            meshNode->get<PhysicsComponent>()->setRotation(data.orientation);
            meshNode->get<PhysicsComponent>()->setPosition(data.position);

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
    static const U32 normalMask = to_base(SGNComponent::ComponentType::NAVIGATION) |
                                  to_base(SGNComponent::ComponentType::PHYSICS) |
                                  to_base(SGNComponent::ComponentType::BOUNDS) |
                                  to_base(SGNComponent::ComponentType::RENDERING) |
                                  to_base(SGNComponent::ComponentType::NETWORKING);

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
        SceneGraphNode_ptr thisObjSGN = _sceneGraph->getRoot().addNode(thisObj,
                                                                       normalMask,
                                                                       data.physicsUsage ? data.physicsStatic ? PhysicsGroup::GROUP_STATIC
                                                                                                              : PhysicsGroup::GROUP_DYNAMIC
                                                                                         : PhysicsGroup::GROUP_IGNORE);
        thisObjSGN->get<PhysicsComponent>()->setScale(data.scale);
        thisObjSGN->get<PhysicsComponent>()->setRotation(data.orientation);
        thisObjSGN->get<PhysicsComponent>()->setPosition(data.position);
        thisObjSGN->get<RenderingComponent>()->castsShadows(data.castsShadows);
        thisObjSGN->get<RenderingComponent>()->receivesShadows(data.receivesShadows);
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

SceneGraphNode_ptr Scene::addParticleEmitter(const stringImpl& name,
                                             std::shared_ptr<ParticleData> data,
                                             SceneGraphNode& parentNode) {
    static const U32 particleMask = to_base(SGNComponent::ComponentType::PHYSICS) |
                                    to_base(SGNComponent::ComponentType::BOUNDS) |
                                    to_base(SGNComponent::ComponentType::RENDERING) |
                                    to_base(SGNComponent::ComponentType::NETWORKING);
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


SceneGraphNode_ptr Scene::addLight(LightType type,
                                   SceneGraphNode& parentNode) {
    static const U32 lightMask = to_base(SGNComponent::ComponentType::PHYSICS) |
                                 to_base(SGNComponent::ComponentType::BOUNDS) |
                                 to_base(SGNComponent::ComponentType::RENDERING) |
                                 to_base(SGNComponent::ComponentType::NETWORKING);

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

void Scene::toggleFlashlight(U8 playerIndex) {
    static const U32 lightMask = to_base(SGNComponent::ComponentType::PHYSICS) |
                                 to_base(SGNComponent::ComponentType::BOUNDS) |
                                 to_base(SGNComponent::ComponentType::RENDERING) |
                                 to_base(SGNComponent::ComponentType::NETWORKING);

    SceneGraphNode_ptr& flashLight = _flashLight[playerIndex];
    if (!flashLight) {
        ResourceDescriptor tempLightDesc(Util::StringFormat("Flashlight_%d", playerIndex));
        tempLightDesc.setEnumValue(to_base(LightType::SPOT));
        tempLightDesc.setUserPtr(_lightPool);
        std::shared_ptr<Light> tempLight = CreateResource<Light>(_resCache, tempLightDesc);
        tempLight->setDrawImpostor(false);
        tempLight->setRange(30.0f);
        tempLight->setCastShadows(true);
        tempLight->setDiffuseColour(DefaultColours::WHITE());
        flashLight = _sceneGraph->getRoot().addNode(tempLight,
                                                    lightMask,
                                                    PhysicsGroup::GROUP_IGNORE);
        hashAlg::emplace(_flashLight,
                         playerIndex,
                         flashLight);
                     
    }

    flashLight->getNode<Light>()->toggleEnabled();
}

SceneGraphNode_ptr Scene::addSky(const stringImpl& nodeName) {
    ResourceDescriptor skyDescriptor("Default Sky");
    skyDescriptor.setID(to_U32(std::floor(_baseCamera->getZPlanes().y * 2)));

    std::shared_ptr<Sky> skyItem = CreateResource<Sky>(_resCache, skyDescriptor);
    DIVIDE_ASSERT(skyItem != nullptr, "Scene::addSky error: Could not create sky resource!");

    static const U32 normalMask = 
        to_base(SGNComponent::ComponentType::NAVIGATION) |
        to_base(SGNComponent::ComponentType::PHYSICS) |
        to_base(SGNComponent::ComponentType::BOUNDS) |
        to_base(SGNComponent::ComponentType::RENDERING) |
        to_base(SGNComponent::ComponentType::NETWORKING);

    SceneGraphNode_ptr skyNode = _sceneGraph->getRoot().addNode(skyItem,
                                                                normalMask,
                                                                PhysicsGroup::GROUP_IGNORE,
                                                                nodeName);
    skyNode->lockVisibility(true);

    return skyNode;
}

U16 Scene::registerInputActions() {
    _input->flushCache();

    auto none = [](InputParams param) {};
    auto deleteSelection = [this](InputParams param) { _sceneGraph->deleteNode(_currentSelection[0], false); };
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
            _context.gui().getEditor().setVisible(!_context.gui().getEditor().isVisible());
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

void Scene::loadBaseCamera() {
    _baseCamera = Camera::createCamera(Util::StringFormat("BaseCamera_%s", _name.c_str()), Camera::CameraType::FREE_FLY);
    

    // Camera position is overridden in the scene's XML configuration file
    if (ParamHandler::instance().getParam<bool>(_ID_RT((getName() + ".options.cameraStartPositionOverride").c_str()))) {
        _baseCamera->setEye(vec3<F32>(
            _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartPosition.x").c_str())),
            _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartPosition.y").c_str())),
            _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartPosition.z").c_str()))));
        vec2<F32> camOrientation(_paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartOrientation.xOffsetDegrees").c_str())),
            _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartOrientation.yOffsetDegrees").c_str())));
        _baseCamera->setGlobalRotation(camOrientation.y /*yaw*/, camOrientation.x /*pitch*/);
    } else {
        _baseCamera->setEye(vec3<F32>(0, 50, 0));
    }

    _baseCamera->setMoveSpeedFactor(_paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraSpeed.move").c_str()), 1.0f));
    _baseCamera->setTurnSpeedFactor(_paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraSpeed.turn").c_str()), 1.0f));
    _baseCamera->setProjection(_context.gfx().renderingData().aspectRatio(),
                               _context.config().runtime.verticalFOV,
                               vec2<F32>(_context.config().runtime.zNear, _context.config().runtime.zFar));
}

bool Scene::load(const stringImpl& name) {
    setState(ResourceState::RES_LOADING);

    STUBBED("ToDo: load skyboxes from XML")
    _name = name;

    loadBaseCamera();
    loadXMLAssets();
    addSelectionCallback(DELEGATE_BIND(&GUI::selectionChangeCallback, &_context.gui(), this, std::placeholders::_1));

    U32 totalLoadingTasks = _loadingTasks;
    Console::d_printfn(Locale::get(_ID("SCENE_LOAD_TASKS")), totalLoadingTasks);
    while (totalLoadingTasks > 0) {
        if (totalLoadingTasks != _loadingTasks) {
            totalLoadingTasks = _loadingTasks;
            Console::d_printfn(Locale::get(_ID("SCENE_LOAD_TASKS")), totalLoadingTasks);
        }
        std::this_thread::yield();
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
    _lightPool->clear();
    /// Destroy physics (:D)
    _pxScene->release();
    MemoryManager::DELETE(_pxScene);
    _context.pfx().setPhysicsScene(nullptr);
    clearObjects();
    Camera::destroyCamera(_baseCamera);
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
    SceneGraphNode_ptr selection(_currentSelection[0].lock());
    if (selection != nullptr) {
        selection->get<RenderingComponent>()->rebuildMaterial();
    } else {
        ShaderProgram::rebuildAllShaders();
    }
}

stringImpl Scene::getPlayerSGNName(U8 playerIndex) {
    return Util::StringFormat(g_defaultPlayerName, playerIndex + 1);
}

void Scene::currentPlayerPass(U8 playerIndex) {
    renderState().playerPass(playerIndex);

    if (state().playerState(playerIndex).cameraUnderwater()) {
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
    
    SceneGraphNode_ptr playerSGN(_sceneGraph->findNode(playerName).lock());
    if (!playerSGN) {
        SceneGraphNode& root = _sceneGraph->getRoot();
        playerSGN = root.addNode(SceneNode_ptr(MemoryManager_NEW SceneTransform(_resCache, 12345678 + _parent.getPlayers().size(), g_PlayerExtents)),
                                to_base(SGNComponent::ComponentType::NAVIGATION) |
                                to_base(SGNComponent::ComponentType::PHYSICS) |
                                to_base(SGNComponent::ComponentType::BOUNDS) |
                                to_base(SGNComponent::ComponentType::UNIT) |
                                to_base(SGNComponent::ComponentType::NETWORKING),
                                PhysicsGroup::GROUP_KINEMATIC,
                                playerName);
        _parent.addPlayer(*this, playerSGN, queue);
    } else {
        assert(playerSGN->get<UnitComponent>()->getUnit() != nullptr);
    }
}

void Scene::removePlayerInternal(U8 playerIndex) {
    assert(playerIndex < _scenePlayers.size());
    
    _parent.removePlayer(*this, _scenePlayers[getSceneIndexForPlayer(playerIndex)], true);
}

void Scene::onPlayerAdd(const Player_ptr& player) {
    _scenePlayers.push_back(player);
    state().onPlayerAdd(player->index());
    input().onPlayerAdd(player->index());
}

void Scene::onPlayerRemove(const Player_ptr& player) {
    U8 playerIndex = player->index();

    input().onPlayerRemove(playerIndex);
    state().onPlayerRemove(playerIndex);

    _sceneGraph->getRoot().removeNode(*player->getBoundNode().lock());

    _scenePlayers.erase(std::cbegin(_scenePlayers) + getSceneIndexForPlayer(playerIndex));
}

U8 Scene::getSceneIndexForPlayer(U8 playerIndex) const {
    for (U8 i = 0; i < to_U8(_scenePlayers.size()); ++i) {
        if (_scenePlayers[i]->index() == playerIndex) {
            return i;
        }
    }

    DIVIDE_UNEXPECTED_CALL("Player not found!");
    return 0;
}

const Player_ptr& Scene::getPlayerForIndex(U8 playerIndex) const {
    return _scenePlayers[getSceneIndexForPlayer(playerIndex)];
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
    U8 playerIndex = getPlayerIndexForDevice(arg._deviceIndex);

    Camera& cam = _scenePlayers[playerIndex]->getCamera();
    if (cam.moveRelative(vec3<I32>(arg._event.state.X.rel,
                                   arg._event.state.Y.rel,
                                   arg._event.state.Z.rel)))
    {
        if (cam.getType() == Camera::CameraType::THIRD_PERSON) {
            _context.app().snapCursorToCenter();
        }
        return true;
    }

    return false;
}

bool Scene::updateCameraControls(U8 playerIndex) {
    Camera& cam = getPlayerForIndex(playerIndex)->getCamera();
    
    SceneStatePerPlayer& playerState = state().playerState(playerIndex);

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

    playerState.cameraUnderwater(checkCameraUnderwater(playerIndex));

    return playerState.cameraUpdated();
}

void Scene::updateSceneState(const U64 deltaTime) {
    _sceneTimer += deltaTime;
    updateSceneStateInternal(deltaTime);
    _sceneGraph->sceneUpdate(deltaTime, *_sceneState);
    _aiManager->update(deltaTime);

    for (U8 i = 0; i < to_U8(_scenePlayers.size()); ++i) {
        U8 playerIndex = _scenePlayers[i]->index();
        findHoverTarget(playerIndex);
        if (_flashLight[playerIndex]) {
            PhysicsComponent* pComp = _flashLight[playerIndex]->get<PhysicsComponent>();
            const Camera& cam = _scenePlayers[playerIndex]->getCamera();
            pComp->setPosition(cam.getEye());
            pComp->setRotation(cam.getEuler());
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

void Scene::processInput(U8 playerIndex, const U64 deltaTime) {
}

void Scene::processGUI(const U64 deltaTime) {
    for (U16 i = 0; i < _guiTimers.size(); ++i) {
        _guiTimers[i] += Time::MicrosecondsToMilliseconds<D64>(deltaTime);
    }
}

void Scene::processTasks(const U64 deltaTime) {
    for (U16 i = 0; i < _taskTimers.size(); ++i) {
        _taskTimers[i] += Time::MicrosecondsToMilliseconds<D64>(deltaTime);
    }
}

void Scene::debugDraw(const Camera& activeCamera, const RenderStagePass& stagePass, RenderSubPassCmds& subPassesInOut) {
    if (stagePass.stage() != RenderStage::DISPLAY || stagePass.pass() == RenderPassType::DEPTH_PASS) {
        return;
    }

    RenderSubPassCmd& subPass = subPassesInOut.back();
    if (Config::Build::IS_DEBUG_BUILD) {
        const SceneRenderState::GizmoState& currentGizmoState = renderState().gizmoState();

        if (currentGizmoState == SceneRenderState::GizmoState::SELECTED_GIZMO) {
            SceneGraphNode_ptr selection(_currentSelection[0].lock());
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
                _octreePrimitives[i]->fromBox(box.getMin(), box.getMax(), vec4<U8>(255, 0, 255, 255));
                subPass._commands.push_back(_octreePrimitives[i]->toDrawCommand());
            }
        }
    }
    if (Config::Build::IS_DEBUG_BUILD) {
        subPass._commands.push_back(_linesPrimitive->toDrawCommand());
    }
    // Show NavMeshes
    _aiManager->debugDraw(subPassesInOut, false);
    _lightPool->drawLightImpostors(subPassesInOut);
    _envProbePool->debugDraw(subPassesInOut);
}

bool Scene::checkCameraUnderwater(U8 playerIndex) const {
    const vectorImpl<SceneGraphNode_wptr>& waterBodies = _sceneGraph->getNodesByType(SceneNodeType::TYPE_WATER);

    if (!waterBodies.empty()) {
        const Camera& crtCamera = getPlayerForIndex(playerIndex)->getCamera();
        const vec3<F32>& eyePos = crtCamera.getEye();

        for (const SceneGraphNode_wptr& node : waterBodies) {
            SceneGraphNode_ptr nodePtr = node.lock();
            if (nodePtr->getNode<WaterPlane>()->pointUnderwater(*nodePtr, eyePos)) {
                return true;
            }
        }
    }

    return false;
}

void Scene::findHoverTarget(U8 playerIndex) {
    const Camera& crtCamera = getPlayerForIndex(playerIndex)->getCamera();

    const vec2<U16>& displaySize = _context.app().windowManager().getActiveWindow().getDimensions();
    const vec2<F32>& zPlanes = crtCamera.getZPlanes();
    const vec2<I32>& aimPos = state().playerState(playerIndex).aimPos();

    F32 aimX = to_F32(aimPos.x);
    F32 aimY = displaySize.height - to_F32(aimPos.y) - 1;

    const vec4<I32>& viewport = _context.gfx().getCurrentViewport();
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
        _currentHoverTarget[playerIndex] = _sceneSelectionCandidates.front();
        std::shared_ptr<SceneNode> node = _currentHoverTarget[playerIndex].lock()->getNode();
        if (node->getType() == SceneNodeType::TYPE_OBJECT3D) {
            if (static_cast<Object3D*>(node.get())->getObjectType() == Object3D::ObjectType::SUBMESH) {
                _currentHoverTarget[playerIndex] = _currentHoverTarget[playerIndex].lock()->getParent();
            }
        }

        SceneGraphNode_ptr target = _currentHoverTarget[playerIndex].lock();
        if (target->getSelectionFlag() != SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
            target->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_HOVER);
        }
    } else {
        SceneGraphNode_ptr target(_currentHoverTarget[playerIndex].lock());
        if (target) {
            if (target->getSelectionFlag() != SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
                target->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_NONE);
            }
            _currentHoverTarget[playerIndex].reset();
        }

    }

}

void Scene::resetSelection() {
    if (!_currentSelection[0].expired()) {
        _currentSelection[0].lock()->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_NONE);
    }

    _currentSelection[0].reset();
}

void Scene::findSelection(U8 playerIndex) {
    bool hadTarget = !_currentSelection[playerIndex].expired();
    bool haveTarget = !_currentHoverTarget[playerIndex].expired();

    I64 crtGUID = hadTarget ? _currentSelection[playerIndex].lock()->getGUID() : -1;
    I64 GUID = haveTarget ? _currentHoverTarget[playerIndex].lock()->getGUID() : -1;

    if (crtGUID != GUID) {
        resetSelection();

        if (haveTarget) {
            _currentSelection[playerIndex] = _currentHoverTarget[playerIndex];
            _currentSelection[playerIndex].lock()->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_SELECTED);
        }

        for (DELEGATE_CBK<void, U8>& cbk : _selectionChangeCallbacks) {
            cbk(playerIndex);
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

};