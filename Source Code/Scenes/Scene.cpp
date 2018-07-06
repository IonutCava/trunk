#include "Headers/Scene.h"

#include "GUI/Headers/GUI.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"

#include "Utility/Headers/XMLParser.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

#include "Environment/Sky/Headers/Sky.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

#include "Platform/Video/Headers/IMPrimitive.h"

#include "Physics/Headers/PhysicsSceneInterface.h"


namespace Divide {

namespace {
struct selectionQueueDistanceFrontToBack {
    selectionQueueDistanceFrontToBack(const vec3<F32>& eyePos)
        : _eyePos(eyePos) {}

    bool operator()(SceneGraphNode_wptr a, SceneGraphNode_wptr b) const {
        F32 dist_a =
            a.lock()->get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(_eyePos);
        F32 dist_b =
            b.lock()->get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(_eyePos);
        return dist_a > dist_b;
    }

   private:
    vec3<F32> _eyePos;
};
};

Scene::Scene(const stringImpl& name)
    : Resource(name),
      _GFX(GFX_DEVICE),
      _LRSpeedFactor(5.0f),
      _loadComplete(false),
      _cookCollisionMeshesScheduled(false),
      _paramHandler(ParamHandler::instance())
{
    _sceneTimer = 0UL;
    _input.reset(new SceneInput(*this));

    _sceneGraph.reset(new SceneGraph(*this));

#ifdef _DEBUG

    RenderStateBlock primitiveDescriptor;
    _linesPrimitive = GFX_DEVICE.getOrCreatePrimitive(false);
    _linesPrimitive->name("LinesRayPick");
    _linesPrimitive->stateHash(primitiveDescriptor.getHash());
    _linesPrimitive->paused(true);
#endif
}

Scene::~Scene()
{
#ifdef _DEBUG
    _linesPrimitive->_canZombify = true;
#endif
}

bool Scene::initStaticData() {
    return true;
}

bool Scene::frameStarted() { return true; }

bool Scene::frameEnded() { return true; }

bool Scene::idle() {  // Called when application is idle
    if (!_modelDataArray.empty()) {
        loadXMLAssets(true);
    }

    if (!_sceneGraph->getRoot().hasChildren()) {
        return false;
    }

    _sceneGraph->idle();

    Attorney::SceneRenderStateScene::playAnimations(
        renderState(),
        ParamHandler::instance().getParam<bool>(_ID("mesh.playAnimations"), true));

    if (_cookCollisionMeshesScheduled && checkLoadFlag()) {
        if (GFX_DEVICE.getFrameCount() > 1) {
            _sceneGraph->getRoot().get<PhysicsComponent>()->cookCollisionMesh(_name);
            _cookCollisionMeshesScheduled = false;
        }
    }

    _lightPool->idle();

    return true;
}

void Scene::addPatch(vectorImpl<FileData>& data) {
}

void Scene::loadXMLAssets(bool singleStep) {
    while (!_modelDataArray.empty()) {
        const FileData& it = _modelDataArray.top();
        // vegetation is loaded elsewhere
        if (it.type == GeometryType::VEGETATION) {
            _vegetationDataArray.push_back(it);
        } else  if (it.type == GeometryType::PRIMITIVE) {
            loadGeometry(it);
        } else {
            loadModel(it);
        }
        _modelDataArray.pop();

        if (singleStep) {
            break;
        }
    }
}

bool Scene::loadModel(const FileData& data) {
    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);

    ResourceDescriptor model(data.ModelName);
    model.setResourceLocation(data.ModelName);
    model.setFlag(true);
    Mesh_ptr thisObj = CreateResource<Mesh>(model);
    if (!thisObj) {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD_MODEL")),
                         data.ModelName.c_str());
        return false;
    }

    SceneGraphNode_ptr meshNode =
        _sceneGraph->getRoot().addNode(thisObj,
                                       normalMask,
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
    return true;
}

bool Scene::loadGeometry(const FileData& data) {
    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);

    std::shared_ptr<Object3D> thisObj;
    ResourceDescriptor item(data.ItemName);
    item.setResourceLocation(data.ModelName);
    if (data.ModelName.compare("Box3D") == 0) {
        item.setPropertyList(Util::StringFormat("%2.2f", data.data));
        thisObj = CreateResource<Box3D>(item);
    } else if (data.ModelName.compare("Sphere3D") == 0) {
        thisObj = CreateResource<Sphere3D>(item);
        static_cast<Sphere3D*>(thisObj.get())->setRadius(data.data);
    } else if (data.ModelName.compare("Quad3D") == 0) {
        vec3<F32> scale = data.scale;
        vec3<F32> position = data.position;
        P32 quadMask;
        quadMask.i = 0;
        quadMask.b[0] = 1;
        item.setBoolMask(quadMask);
        thisObj = CreateResource<Quad3D>(item);
        static_cast<Quad3D*>(thisObj.get())
            ->setCorner(Quad3D::CornerLocation::TOP_LEFT, vec3<F32>(0, 1, 0));
        static_cast<Quad3D*>(thisObj.get())
            ->setCorner(Quad3D::CornerLocation::TOP_RIGHT, vec3<F32>(1, 1, 0));
        static_cast<Quad3D*>(thisObj.get())
            ->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT, vec3<F32>(0, 0, 0));
        static_cast<Quad3D*>(thisObj.get())
            ->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>(1, 0, 0));
    } else if (data.ModelName.compare("Text3D") == 0) {
        /// set font file
        item.setResourceLocation(data.data3);
        item.setPropertyList(data.data2);
        thisObj = CreateResource<Text3D>(item);
        static_cast<Text3D*>(thisObj.get())->setWidth(data.data);
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_UNSUPPORTED_GEOM")),
                         data.ModelName.c_str());
        return false;
    }
    STUBBED("Load material from XML disabled for primitives! - Ionut")
    std::shared_ptr<Material> tempMaterial; /* = XML::loadMaterial(data.ItemName + "_material")*/;
    if (!tempMaterial) {
        ResourceDescriptor materialDescriptor(data.ItemName + "_material");
        tempMaterial = CreateResource<Material>(materialDescriptor);
        tempMaterial->setDiffuse(data.color);
        tempMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    }

    thisObj->setMaterialTpl(tempMaterial);
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
    return true;
}

SceneGraphNode_ptr Scene::addParticleEmitter(const stringImpl& name,
                                             std::shared_ptr<ParticleData> data,
                                             SceneGraphNode& parentNode) {
    static const U32 particleMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                    to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                    to_const_uint(SGNComponent::ComponentType::RENDERING);
    DIVIDE_ASSERT(!name.empty(),
                  "Scene::addParticleEmitter error: invalid name specified!");

    ResourceDescriptor particleEmitter(name);
    std::shared_ptr<ParticleEmitter> emitter = CreateResource<ParticleEmitter>(particleEmitter);

    DIVIDE_ASSERT(emitter != nullptr,
                  "Scene::addParticleEmitter error: Could not instantiate emitter!");

    emitter->initData(data);

    return parentNode.addNode(emitter, particleMask, PhysicsGroup::GROUP_IGNORE);
}


SceneGraphNode_ptr Scene::addLight(LightType type,
                                SceneGraphNode& parentNode) {
    static const U32 lightMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                 to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                 to_const_uint(SGNComponent::ComponentType::RENDERING);

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

    defaultLight.setEnumValue(to_uint(type));
    defaultLight.setUserPtr(_lightPool.get());
    std::shared_ptr<Light> light = CreateResource<Light>(defaultLight);
    if (type == LightType::DIRECTIONAL) {
        light->setCastShadows(true);
    }
    return parentNode.addNode(light, lightMask, PhysicsGroup::GROUP_IGNORE);
}

void Scene::toggleFlashlight() {
    static const U32 lightMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                 to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                 to_const_uint(SGNComponent::ComponentType::RENDERING);

    if (_flashLight.lock() == nullptr) {

        ResourceDescriptor tempLightDesc("MainFlashlight");
        tempLightDesc.setEnumValue(to_const_uint(LightType::SPOT));
        tempLightDesc.setUserPtr(_lightPool.get());
        std::shared_ptr<Light> tempLight = CreateResource<Light>(tempLightDesc);
        tempLight->setDrawImpostor(false);
        tempLight->setRange(30.0f);
        tempLight->setCastShadows(true);
        tempLight->setDiffuseColor(DefaultColors::WHITE());
        _flashLight = _sceneGraph->getRoot().addNode(tempLight, lightMask, PhysicsGroup::GROUP_IGNORE);
    }

    _flashLight.lock()->getNode<Light>()->setEnabled(!_flashLight.lock()->getNode<Light>()->getEnabled());
}

SceneGraphNode_ptr Scene::addSky(const stringImpl& nodeName) {
    ResourceDescriptor skyDescriptor("Default Sky");
    skyDescriptor.setID(to_uint(std::floor(renderState().getCameraConst().getZPlanes().y * 2)));

    std::shared_ptr<Sky> skyItem = CreateResource<Sky>(skyDescriptor);
    DIVIDE_ASSERT(skyItem != nullptr, "Scene::addSky error: Could not create sky resource!");

    static const U32 normalMask = 
        to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
        to_const_uint(SGNComponent::ComponentType::PHYSICS) |
        to_const_uint(SGNComponent::ComponentType::BOUNDS) |
        to_const_uint(SGNComponent::ComponentType::RENDERING);

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
    auto deleteSelection = [this](InputParams param) { _sceneGraph->deleteNode(_currentSelection, false); };
    auto increaseCameraSpeed = [this](InputParams param){
        Camera& cam = renderState().getCamera();
        F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
        if (currentCamMoveSpeedFactor < 50) {
            cam.setMoveSpeedFactor(currentCamMoveSpeedFactor + 1.0f);
            cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() + 1.0f);
        }
    };
    auto decreaseCameraSpeed = [this](InputParams param) {
        Camera& cam = renderState().getCamera();
        F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
        if (currentCamMoveSpeedFactor > 1.0f) {
            cam.setMoveSpeedFactor(currentCamMoveSpeedFactor - 1.0f);
            cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() - 1.0f);
        }
    };
    auto increaseResolution = [](InputParams param) {GFX_DEVICE.increaseResolution();};
    auto decreaseResolution = [](InputParams param) {GFX_DEVICE.decreaseResolution();};
    auto moveForward = [this](InputParams param) {state().moveFB(SceneState::MoveDirection::POSITIVE);};
    auto moveBackwards = [this](InputParams param) {state().moveFB(SceneState::MoveDirection::NEGATIVE);};
    auto stopMoveFWDBCK = [this](InputParams param) {state().moveFB(SceneState::MoveDirection::NONE);};
    auto strafeLeft = [this](InputParams param) {state().moveLR(SceneState::MoveDirection::NEGATIVE);};
    auto strafeRight = [this](InputParams param) {state().moveLR(SceneState::MoveDirection::POSITIVE);};
    auto stopStrafeLeftRight = [this](InputParams param) {state().moveLR(SceneState::MoveDirection::NONE);};
    auto rollCCW = [this](InputParams param) {state().roll(SceneState::MoveDirection::POSITIVE);};
    auto rollCW = [this](InputParams param) {state().roll(SceneState::MoveDirection::NEGATIVE);};
    auto stopRollCCWCW = [this](InputParams param) {state().roll(SceneState::MoveDirection::NONE);};
    auto turnLeft = [this](InputParams param) { state().angleLR(SceneState::MoveDirection::POSITIVE);};
    auto turnRight = [this](InputParams param) { state().angleLR(SceneState::MoveDirection::NEGATIVE);};
    auto stopTurnLeftRight = [this](InputParams param) { state().angleLR(SceneState::MoveDirection::NONE);};
    auto turnUp = [this](InputParams param) {state().angleUD(SceneState::MoveDirection::POSITIVE);};
    auto turnDown = [this](InputParams param) {state().angleUD(SceneState::MoveDirection::NEGATIVE);};
    auto stopTurnUpDown = [this](InputParams param) {state().angleUD(SceneState::MoveDirection::NONE);};
    auto togglePauseState = [](InputParams param){
        ParamHandler& par = ParamHandler::instance();
        par.setParam(_ID("freezeLoopTime"), !par.getParam(_ID("freezeLoopTime"), false));
    };
    auto toggleDepthOfField = [](InputParams param) {
        ParamHandler& par = ParamHandler::instance();
        par.setParam(_ID("postProcessing.enableDepthOfField"), !par.getParam(_ID("postProcessing.enableDepthOfField"), false));
    };
    auto toggleBloom = [](InputParams param) {
        ParamHandler& par = ParamHandler::instance();
        par.setParam(_ID("postProcessing.enableBloom"), !par.getParam(_ID("postProcessing.enableBloom"), false));
    };
    auto toggleSkeletonRendering = [this](InputParams param) {renderState().toggleSkeletons();};
    auto toggleAxisLineRendering = [this](InputParams param) {renderState().toggleAxisLines();};
    auto toggleWireframeRendering = [this](InputParams param) {renderState().toggleWireframe();};
    auto toggleGeometryRendering = [this](InputParams param) { renderState().toggleGeometry();};
    auto toggleDebugLines = [this](InputParams param) {renderState().toggleDebugLines();};
    auto toggleBoundingBoxRendering = [this](InputParams param) {renderState().toggleBoundingBoxes();};
    auto toggleShadowMapDepthBufferPreview = [](InputParams param) {
        ParamHandler& par = ParamHandler::instance();
        LightPool::togglePreviewShadowMaps();
        par.setParam<bool>(
            _ID("rendering.previewDepthBuffer"),
            !par.getParam<bool>(_ID("rendering.previewDepthBuffer"), false));
    };
    auto takeScreenshot = [](InputParams param) { GFX_DEVICE.Screenshot("screenshot_"); };
    auto toggleFullScreen = [](InputParams param) { GFX_DEVICE.toggleFullScreen(); };
    auto toggleFlashLight = [this](InputParams param) {toggleFlashlight(); };
    auto toggleOctreeRegionRendering = [this](InputParams param) {renderState().drawOctreeRegions(!renderState().drawOctreeRegions());};
    auto select = [this](InputParams  param) {findSelection(); };
    auto lockCameraToMouse = [this](InputParams  param) {state().cameraLockedToMouse(true); };
    auto releaseCameraFromMouse = [this](InputParams  param) {
        state().cameraLockedToMouse(false);
        state().angleLR(SceneState::MoveDirection::NONE);
        state().angleUD(SceneState::MoveDirection::NONE);
    };
    auto rendererDebugView = [](InputParams param) {SceneManager::instance().getRenderer().toggleDebugView();};
    auto shutdown = [](InputParams param) {Application::instance().RequestShutdown();};
    auto povNavigation = [this](InputParams param) {
        if (param._var[0] & OIS::Pov::North) {  // Going up
            state().moveFB(SceneState::MoveDirection::POSITIVE);
        }
        if (param._var[0] & OIS::Pov::South) {  // Going down
            state().moveFB(SceneState::MoveDirection::NEGATIVE);
        }
        if (param._var[0] & OIS::Pov::East) {  // Going right
            state().moveLR(SceneState::MoveDirection::POSITIVE);
        }
        if (param._var[0] & OIS::Pov::West) {  // Going left
            state().moveLR(SceneState::MoveDirection::NEGATIVE);
        }
        if (param._var[0] == OIS::Pov::Centered) {  // stopped/centered out
            state().moveLR(SceneState::MoveDirection::NONE);
            state().moveFB(SceneState::MoveDirection::NONE);
        }
    };

    auto axisNavigation = [this](InputParams param) {
        I32 axis = param._var[2];
        Input::Joystick joystick = static_cast<Input::Joystick>(param._var[3]);

        Input::JoystickInterface* joyInterface = nullptr;
        joyInterface = Input::InputInterface::instance().getJoystickInterface();
        
        const Input::JoystickData& joyData = joyInterface->getJoystickData(joystick);
        I32 deadZone = joyData._deadZone;
        I32 axisABS = std::min(param._var[0], joyData._max);

        switch (axis) {
            case 0: {
                if (axisABS > deadZone) {
                    state().angleUD(SceneState::MoveDirection::POSITIVE);
                } else if (axisABS < -deadZone) {
                    state().angleUD(SceneState::MoveDirection::NEGATIVE);
                } else {
                    state().angleUD(SceneState::MoveDirection::NONE);
                }
            } break;
            case 1: {
                if (axisABS > deadZone) {
                    state().angleLR(SceneState::MoveDirection::POSITIVE);
                } else if (axisABS < -deadZone) {
                    state().angleLR(SceneState::MoveDirection::NEGATIVE);
                } else {
                    state().angleLR(SceneState::MoveDirection::NONE);
                }
            } break;

            case 2: {
                if (axisABS < -deadZone) {
                    state().moveFB(SceneState::MoveDirection::POSITIVE);
                } else if (axisABS > deadZone) {
                    state().moveFB(SceneState::MoveDirection::NEGATIVE);
                } else {
                    state().moveFB(SceneState::MoveDirection::NONE);
                }
            } break;
            case 3: {
                if (axisABS < -deadZone) {
                    state().moveLR(SceneState::MoveDirection::NEGATIVE);
                } else if (axisABS > deadZone) {
                    state().moveLR(SceneState::MoveDirection::POSITIVE);
                } else {
                    state().moveLR(SceneState::MoveDirection::NONE);
                }
            } break;
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

    return actionID;
}

void Scene::loadKeyBindings() {
    XML::loadDefaultKeybindings("keyBindings.xml", this);
}

bool Scene::load(const stringImpl& name, GUI* const guiInterface) {
    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);

    STUBBED("ToDo: load skyboxes from XML")
    _GUI = guiInterface;
    _name = name;

    SceneManager::instance().enableFog(_sceneState.fogDescriptor()._fogDensity,
                                        _sceneState.fogDescriptor()._fogColor);

    loadXMLAssets();
    SceneGraphNode& root = _sceneGraph->getRoot();
    // Add terrain from XML
    if (!_terrainInfoArray.empty()) {
        for (TerrainDescriptor* terrainInfo : _terrainInfoArray) {
            ResourceDescriptor terrain(terrainInfo->getVariable("terrainName"));
            terrain.setPropertyDescriptor(*terrainInfo);
            std::shared_ptr<Terrain> temp = CreateResource<Terrain>(terrain);

            SceneGraphNode_ptr terrainTemp = root.addNode(temp, normalMask, PhysicsGroup::GROUP_STATIC);
            terrainTemp->setActive(terrainInfo->getActive());
            terrainTemp->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
            _terrainList.push_back(terrainTemp->getName());

            NavigationComponent* nComp = terrainTemp->get<NavigationComponent>();
            nComp->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);
            MemoryManager::DELETE(terrainInfo);
        }
    }
    _terrainInfoArray.clear();

    // Camera position is overridden in the scene's XML configuration file
    if (ParamHandler::instance().getParam<bool>(
        _ID("options.cameraStartPositionOverride"))) {
        renderState().getCamera().setEye(vec3<F32>(
            _paramHandler.getParam<F32>(_ID("options.cameraStartPosition.x")),
            _paramHandler.getParam<F32>(_ID("options.cameraStartPosition.y")),
            _paramHandler.getParam<F32>(_ID("options.cameraStartPosition.z"))));
        vec2<F32> camOrientation(
            _paramHandler.getParam<F32>(
                _ID("options.cameraStartOrientation.xOffsetDegrees")),
            _paramHandler.getParam<F32>(
                _ID("options.cameraStartOrientation.yOffsetDegrees")));
        renderState().getCamera().setGlobalRotation(camOrientation.y /*yaw*/,
                                                    camOrientation.x /*pitch*/);
    } else {
        renderState().getCamera().setEye(vec3<F32>(0, 50, 0));
    }

    renderState().getCamera().setMoveSpeedFactor(_paramHandler.getParam<F32>(_ID("options.cameraSpeed.move"), 1.0f));
    renderState().getCamera().setTurnSpeedFactor(_paramHandler.getParam<F32>(_ID("options.cameraSpeed.turn"), 1.0f));

    addSelectionCallback(DELEGATE_BIND(&GUI::selectionChangeCallback,
                                       &GUI::instance(), this));
    _loadComplete = true;
    return _loadComplete;
}

bool Scene::unload() {
    // prevent double unload calls
    if (!checkLoadFlag()) {
        return false;
    }
    clearTasks();
    _lightPool->clear();
    /// Destroy physics (:D)
    PHYSICS_DEVICE.setPhysicsScene(nullptr);
    clearObjects();
    _loadComplete = false;
    return true;
}

void Scene::postLoad() {
    _sceneGraph->postLoad();
}

PhysicsSceneInterface* Scene::createPhysicsImplementation() {
    return PHYSICS_DEVICE.NewSceneInterface(this);
}

bool Scene::loadPhysics(bool continueOnErrors) {
    // Add a new physics scene (can be overridden in each scene for custom
    // behavior)
    PHYSICS_DEVICE.setPhysicsScene(createPhysicsImplementation());
    // Initialize the physics scene
    PHYSICS_DEVICE.initScene();
    // Cook geometry
    if (_paramHandler.getParam<bool>(_ID("options.autoCookPhysicsAssets"), true)) {
        _cookCollisionMeshesScheduled = true;
    }
    return true;
}

bool Scene::initializeAI(bool continueOnErrors) {
    _aiTask = std::thread(DELEGATE_BIND(&AI::AIManager::update, &AI::AIManager::instance()));
    return true;
}

 /// Shut down AIManager thread
bool Scene::deinitializeAI(bool continueOnErrors) { 
    AI::AIManager::instance().stop();
    WAIT_FOR_CONDITION(!AI::AIManager::instance().running());
    _aiTask.join();
        
    return true;
}

void Scene::clearObjects() {
    while (!_modelDataArray.empty()) {
        _modelDataArray.pop();
    }
    _vegetationDataArray.clear();

    _sceneGraph->unload();
}

bool Scene::updateCameraControls() {
    Camera& cam = renderState().getCamera();

    state().cameraUpdated(false);
    switch (cam.getType()) {
        default:
        case Camera::CameraType::FREE_FLY: {
            if (state().angleLR() != SceneState::MoveDirection::NONE) {
                cam.rotateYaw(to_float(state().angleLR()));
                state().cameraUpdated(true);
            }
            if (state().angleUD() != SceneState::MoveDirection::NONE) {
                cam.rotatePitch(to_float(state().angleUD()));
                state().cameraUpdated(true);
            }
            if (state().roll() != SceneState::MoveDirection::NONE) {
                cam.rotateRoll(to_float(state().roll()));
                state().cameraUpdated(true);
            }
            if (state().moveFB() != SceneState::MoveDirection::NONE) {
                cam.moveForward(to_float(state().moveFB()));
                state().cameraUpdated(true);
            }
            if (state().moveLR() != SceneState::MoveDirection::NONE) {
                cam.moveStrafe(to_float(state().moveLR()));
                state().cameraUpdated(true);
            }
        } break;
    }

    return state().cameraUpdated();
}

void Scene::updateSceneState(const U64 deltaTime) {
    _sceneTimer += deltaTime;
    updateSceneStateInternal(deltaTime);
    state().cameraUnderwater(renderState().getCamera().getEye().y <
                             state().waterLevel());
    _sceneGraph->sceneUpdate(deltaTime, _sceneState);
    findHoverTarget();
    SceneGraphNode_ptr flashLight = _flashLight.lock();

    if (flashLight) {
        const Camera& cam = renderState().getCameraConst();
        flashLight->get<PhysicsComponent>()->setPosition(cam.getEye());
        flashLight->get<PhysicsComponent>()->setRotation(cam.getEuler());
    }
}

void Scene::onLostFocus() {
    state().resetMovement();
#ifndef _DEBUG
    //_paramHandler.setParam(_ID("freezeLoopTime"), true);
#endif
}

void Scene::registerTask(const TaskHandle& taskItem) { 
    _tasks.push_back(taskItem);
}

void Scene::clearTasks() {
    Console::printfn(Locale::get(_ID("STOP_SCENE_TASKS")));
    // Performance shouldn't be an issue here
    for (TaskHandle& task : _tasks) {
        if (task._task->jobIdentifier() == task._jobIdentifier) {
            task._task->stopTask();
            task.wait();
        }
    }

    _tasks.clear();
}

void Scene::removeTask(I64 jobIdentifier) {
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

void Scene::processInput(const U64 deltaTime) {
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

void Scene::debugDraw(RenderStage stage) {
#ifdef _DEBUG
    const SceneRenderState::GizmoState& currentGizmoState = renderState().gizmoState();

    GFX_DEVICE.drawDebugAxis(currentGizmoState != SceneRenderState::GizmoState::NO_GIZMO);

    if (currentGizmoState == SceneRenderState::GizmoState::SELECTED_GIZMO) {
        SceneGraphNode_ptr selection(_currentSelection.lock());
        if (selection != nullptr) {
            selection->get<RenderingComponent>()->drawDebugAxis();
        }
    }

    if (renderState().drawOctreeRegions()) {
        for (IMPrimitive* prim : _octreePrimitives) {
            prim->paused(true);
        }

        GFXDevice& gfx = GFX_DEVICE;

        _octreeBoundingBoxes.resize(0);
        getSceneGraph().getOctree().getAllRegions(_octreeBoundingBoxes);

        size_t primitiveCount = _octreePrimitives.size();
        size_t regionCount = _octreeBoundingBoxes.size();
        if (regionCount > primitiveCount) {
            size_t diff = regionCount - primitiveCount;
            for (size_t i = 0; i < diff; ++i) {
                _octreePrimitives.push_back(gfx.getOrCreatePrimitive(false));
            }
        }

        assert(_octreePrimitives.size() >= _octreeBoundingBoxes.size());

        for (size_t i = 0; i < regionCount; ++i) {
            const BoundingBox& box = _octreeBoundingBoxes[i];
            IMPrimitive& prim = *_octreePrimitives[i];
            gfx.drawBox3D(prim, box.getMin(), box.getMax(), vec4<U8>(255, 0, 255, 255));
        }
    }

#endif
    if (stage == RenderStage::DISPLAY) {
        // Draw bounding boxes, skeletons, axis gizmo, etc.
        GFX_DEVICE.debugDraw(renderState());
        // Show NavMeshes
        AI::AIManager::instance().debugDraw(false);
        _lightPool->drawLightImpostors();
    }
}

void Scene::findHoverTarget() {
    const vec2<U16>& displaySize = Application::instance().windowManager().getActiveWindow().getDimensions();
    const vec2<F32>& zPlanes = renderState().getCameraConst().getZPlanes();
    const vec2<I32>& mousePos = _input->getMousePosition();

    F32 mouseX = to_float(mousePos.x);
    F32 mouseY = displaySize.height - to_float(mousePos.y) - 1;

    vec3<F32> startRay = renderState().getCameraConst().unProject(mouseX, mouseY, 0.0f);
    vec3<F32> endRay = renderState().getCameraConst().unProject(mouseX, mouseY, 1.0f);
    // see if we select another one
    _sceneSelectionCandidates.clear();
    // get the list of visible nodes (use Z_PRE_PASS because the nodes are sorted by depth, front to back)
    RenderPassCuller::VisibleNodeList& nodes = SceneManager::instance().getVisibleNodesCache(RenderStage::Z_PRE_PASS);

    // Cast the picking ray and find items between the nearPlane and far Plane
    Ray mouseRay(startRay, startRay.direction(endRay));
    for (RenderPassCuller::VisibleNode& node : nodes) {
        SceneGraphNode_cptr nodePtr = node.second.lock();
        if (nodePtr) {
            nodePtr->intersect(mouseRay, zPlanes.x, zPlanes.y, _sceneSelectionCandidates, false);                   
        }
    }

    if (!_sceneSelectionCandidates.empty()) {
        _currentHoverTarget = _sceneSelectionCandidates.front();
        std::shared_ptr<SceneNode> node = _currentHoverTarget.lock()->getNode();
        if (node->getType() == SceneNodeType::TYPE_OBJECT3D) {
            if (static_cast<Object3D*>(node.get())->getObjectType() == Object3D::ObjectType::SUBMESH) {
                _currentHoverTarget = _currentHoverTarget.lock()->getParent();
            }
        }

        SceneGraphNode_ptr target = _currentHoverTarget.lock();
        if (target->getSelectionFlag() != SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
            target->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_HOVER);
        }
    } else {
        SceneGraphNode_ptr target(_currentHoverTarget.lock());
        if (target) {
            if (target->getSelectionFlag() != SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
                target->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_NONE);
            }
            _currentHoverTarget.reset();
        }

    }

}

void Scene::resetSelection() {
    SceneGraphNode_ptr selection(_currentSelection.lock());
    if (selection) {
        selection->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_NONE);
    }

    _currentSelection.reset();
}

void Scene::findSelection() {
    SceneGraphNode_ptr target(_currentHoverTarget.lock());
    SceneGraphNode_ptr crtTarget(_currentSelection.lock());

    bool hadTarget = crtTarget != nullptr;
    bool haveTarget = target != nullptr;

    I64 crtGUID = hadTarget ? crtTarget->getGUID() : -1;
    I64 GUID = haveTarget ? target->getGUID() : -1;

    if (crtGUID != GUID) {
        resetSelection();

        if (haveTarget) {
            _currentSelection = target;
            target->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_SELECTED);
        }

        for (DELEGATE_CBK<>& cbk : _selectionChangeCallbacks) {
            cbk();
        }
    }
}

bool Scene::save(ByteBuffer& outputBuffer) const {
    const Camera& cam = state().renderState().getCameraConst();
    outputBuffer << cam.getEye() << cam.getEuler();
    return true;
}

bool Scene::load(ByteBuffer& inputBuffer) {
    vec3<F32> camPos;
    vec3<F32> camEuler;

    inputBuffer >> camPos >> camEuler;

    Camera& cam = state().renderState().getCamera();
    cam.setEye(camPos);
    cam.setGlobalRotation(-camEuler);

    return true;
}

};