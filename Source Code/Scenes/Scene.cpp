#include "Headers/Scene.h"

#include "GUI/Headers/GUI.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"

#include "Utility/Headers/XMLParser.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
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
            a.lock()->getBoundingBoxConst().nearestDistanceFromPointSquared(_eyePos);
        F32 dist_b =
            b.lock()->getBoundingBoxConst().nearestDistanceFromPointSquared(_eyePos);
        return dist_a > dist_b;
    }

   private:
    vec3<F32> _eyePos;
};
};

Scene::Scene()
    : Resource("temp_scene"),
      _GFX(GFX_DEVICE),
      _LRSpeedFactor(5.0f),
      _loadComplete(false),
      _cookCollisionMeshesScheduled(false),
      _paramHandler(ParamHandler::getInstance())
{
    _sceneTimer = 0UL;
    _input.reset(new SceneInput(*this));

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

bool Scene::frameStarted() { return true; }

bool Scene::frameEnded() { return true; }

bool Scene::idle() {  // Called when application is idle
    if (!_modelDataArray.empty()) {
        loadXMLAssets(true);
    }

    if (!_sceneGraph.getRoot().hasChildren()) {
        return false;
    }

    _sceneGraph.idle();

    Attorney::SceneRenderStateScene::playAnimations(
        renderState(),
        ParamHandler::getInstance().getParam<bool>(_ID("mesh.playAnimations"), true));

    if (_cookCollisionMeshesScheduled && checkLoadFlag()) {
        if (GFX_DEVICE.getFrameCount() > 1) {
            _sceneGraph.getRoot()
                .getComponent<PhysicsComponent>()
                ->cookCollisionMesh(_name);
            _cookCollisionMeshesScheduled = false;
        }
    }

    return true;
}

void Scene::onCameraUpdate(Camera& camera) {
    _sceneGraph.getRoot().onCameraUpdate(camera);
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
    ResourceDescriptor model(data.ModelName);
    model.setResourceLocation(data.ModelName);
    model.setFlag(true);
    Mesh* thisObj = CreateResource<Mesh>(model);
    if (!thisObj) {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD_MODEL")),
                         data.ModelName.c_str());
        return false;
    }

    SceneGraphNode_ptr meshNode =
        _sceneGraph.getRoot().addNode(*thisObj, data.ItemName);
    meshNode->getComponent<RenderingComponent>()->castsShadows(
        data.castsShadows);
    meshNode->getComponent<RenderingComponent>()->receivesShadows(
        data.receivesShadows);
    meshNode->getComponent<PhysicsComponent>()->setScale(data.scale);
    meshNode->getComponent<PhysicsComponent>()->setRotation(data.orientation);
    meshNode->getComponent<PhysicsComponent>()->setPosition(data.position);
    if (data.staticUsage) {
        meshNode->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
    }
    if (data.navigationUsage) {
        meshNode->getComponent<NavigationComponent>()->navigationContext(
            NavigationComponent::NavigationContext::NODE_OBSTACLE);
    }
    if (data.physicsUsage) {
        meshNode->getComponent<PhysicsComponent>()->physicsGroup(
            data.physicsPushable ? PhysicsComponent::PhysicsGroup::NODE_COLLIDE
                                 : PhysicsComponent::PhysicsGroup::NODE_COLLIDE_NO_PUSH);
    }
    if (data.useHighDetailNavMesh) {
        meshNode->getComponent<NavigationComponent>()->navigationDetailOverride(
            true);
    }
    return true;
}

bool Scene::loadGeometry(const FileData& data) {
    Object3D* thisObj;
    ResourceDescriptor item(data.ItemName);
    item.setResourceLocation(data.ModelName);
    if (data.ModelName.compare("Box3D") == 0) {
        item.setPropertyList(Util::StringFormat("%2.2f", data.data));
        thisObj = CreateResource<Box3D>(item);
    } else if (data.ModelName.compare("Sphere3D") == 0) {
        thisObj = CreateResource<Sphere3D>(item);
        static_cast<Sphere3D*>(thisObj)->setRadius(data.data);
    } else if (data.ModelName.compare("Quad3D") == 0) {
        vec3<F32> scale = data.scale;
        vec3<F32> position = data.position;
        P32 quadMask;
        quadMask.i = 0;
        quadMask.b[0] = 1;
        item.setBoolMask(quadMask);
        thisObj = CreateResource<Quad3D>(item);
        static_cast<Quad3D*>(thisObj)
            ->setCorner(Quad3D::CornerLocation::TOP_LEFT, vec3<F32>(0, 1, 0));
        static_cast<Quad3D*>(thisObj)
            ->setCorner(Quad3D::CornerLocation::TOP_RIGHT, vec3<F32>(1, 1, 0));
        static_cast<Quad3D*>(thisObj)
            ->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT, vec3<F32>(0, 0, 0));
        static_cast<Quad3D*>(thisObj)
            ->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>(1, 0, 0));
    } else if (data.ModelName.compare("Text3D") == 0) {
        /// set font file
        item.setResourceLocation(data.data3);
        item.setPropertyList(data.data2);
        thisObj = CreateResource<Text3D>(item);
        static_cast<Text3D*>(thisObj)->getWidth() = data.data;
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_UNSUPPORTED_GEOM")),
                         data.ModelName.c_str());
        return false;
    }
    STUBBED("Load material from XML disabled for primitives! - Ionut")
    Material* tempMaterial =
        nullptr /*XML::loadMaterial(data.ItemName + "_material")*/;
    if (!tempMaterial) {
        ResourceDescriptor materialDescriptor(data.ItemName + "_material");
        tempMaterial = CreateResource<Material>(materialDescriptor);
        tempMaterial->setDiffuse(data.color);
        tempMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    }

    thisObj->setMaterialTpl(tempMaterial);
    SceneGraphNode_ptr thisObjSGN = _sceneGraph.getRoot().addNode(*thisObj);
    thisObjSGN->getComponent<PhysicsComponent>()->setScale(data.scale);
    thisObjSGN->getComponent<PhysicsComponent>()->setRotation(data.orientation);
    thisObjSGN->getComponent<PhysicsComponent>()->setPosition(data.position);
    thisObjSGN->getComponent<RenderingComponent>()->castsShadows(
        data.castsShadows);
    thisObjSGN->getComponent<RenderingComponent>()->receivesShadows(
        data.receivesShadows);
    if (data.staticUsage) {
        thisObjSGN->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
    }
    if (data.navigationUsage) {
        thisObjSGN->getComponent<NavigationComponent>()->navigationContext(
            NavigationComponent::NavigationContext::NODE_OBSTACLE);
    }
    if (data.physicsUsage) {
        thisObjSGN->getComponent<PhysicsComponent>()->physicsGroup(
            data.physicsPushable ? PhysicsComponent::PhysicsGroup::NODE_COLLIDE
                                 : PhysicsComponent::PhysicsGroup::NODE_COLLIDE_NO_PUSH);
    }
    if (data.useHighDetailNavMesh) {
        thisObjSGN->getComponent<NavigationComponent>()
            ->navigationDetailOverride(true);
    }
    return true;
}

SceneGraphNode_ptr Scene::addParticleEmitter(const stringImpl& name,
                                             std::shared_ptr<ParticleData> data,
                                             SceneGraphNode& parentNode) {
    DIVIDE_ASSERT(!name.empty(),
                  "Scene::addParticleEmitter error: invalid name specified!");

    ResourceDescriptor particleEmitter(name);
    ParticleEmitter* emitter = CreateResource<ParticleEmitter>(particleEmitter);

    DIVIDE_ASSERT(emitter != nullptr,
                  "Scene::addParticleEmitter error: Could not instantiate emitter!");

    emitter->initData(data);

    return parentNode.addNode(*emitter);
}


SceneGraphNode_ptr Scene::addLight(LightType type,
                                SceneGraphNode& parentNode) {
    const char* lightType = "";
    switch (type) {
        case LightType::DIRECTIONAL:
            lightType = "Default_directional_light ";
            break;
        case LightType::POINT:
            lightType = "Default_point_light_";
            break;
        case LightType::SPOT:
            lightType = "Default_spot_light_";
            break;
    }

    ResourceDescriptor defaultLight(
        lightType +
        std::to_string(LightManager::getInstance().getLights(type).size()));

    defaultLight.setEnumValue(to_uint(type));
    Light* light = CreateResource<Light>(defaultLight);
    if (type == LightType::DIRECTIONAL) {
        light->setCastShadows(true);
    }
    return parentNode.addNode(*light);
}

void Scene::toggleFlashlight() {
    if (_flashLight.lock() == nullptr) {
        ResourceDescriptor tempLightDesc("MainFlashlight");
        tempLightDesc.setEnumValue(to_uint(LightType::SPOT));
        Light* tempLight = CreateResource<Light>(tempLightDesc);
        tempLight->setDrawImpostor(false);
        tempLight->setRange(30.0f);
        tempLight->setCastShadows(false);
        tempLight->setDiffuseColor(DefaultColors::WHITE());
        _flashLight = _sceneGraph.getRoot().addNode(*tempLight);
    }

    _flashLight.lock()->getNode<Light>()->setEnabled(!_flashLight.lock()->getNode<Light>()->getEnabled());
}

SceneGraphNode_ptr Scene::addSky() {
    Sky* skyItem = CreateResource<Sky>(ResourceDescriptor("Default Sky"));
    DIVIDE_ASSERT(skyItem != nullptr, "Scene::addSky error: Could not create sky resource!");
    return addSky(*skyItem);
}

SceneGraphNode_ptr Scene::addSky(Sky& skyItem) {
    return _sceneGraph.getRoot().addNode(skyItem);
}

bool Scene::load(const stringImpl& name, GUI* const guiInterface) {
    STUBBED("ToDo: load skyboxes from XML")
    _GUI = guiInterface;
    _name = name;

    SceneManager::getInstance().enableFog(_sceneState.fogDescriptor()._fogDensity,
                                          _sceneState.fogDescriptor()._fogColor);

    loadXMLAssets();
    SceneGraphNode& root = _sceneGraph.getRoot();
    // Add terrain from XML
    if (!_terrainInfoArray.empty()) {
        for (TerrainDescriptor* terrainInfo : _terrainInfoArray) {
            ResourceDescriptor terrain(terrainInfo->getVariable("terrainName"));
            Terrain* temp = CreateResource<Terrain>(terrain);
            SceneGraphNode_ptr terrainTemp = root.addNode(*temp);
            terrainTemp->setActive(terrainInfo->getActive());
            terrainTemp->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);

            NavigationComponent* nComp =
                terrainTemp->getComponent<NavigationComponent>();
            nComp->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);

            PhysicsComponent* pComp =
                terrainTemp->getComponent<PhysicsComponent>();
            pComp->physicsGroup(terrainInfo->getCreatePXActor()
                                    ? PhysicsComponent::PhysicsGroup::NODE_COLLIDE_NO_PUSH
                                    : PhysicsComponent::PhysicsGroup::NODE_COLLIDE_IGNORE);
        }
    }
    // Camera position is overridden in the scene's XML configuration file
    if (ParamHandler::getInstance().getParam<bool>(
            "options.cameraStartPositionOverride")) {
        renderState().getCamera().setEye(vec3<F32>(
            _paramHandler.getParam<F32>("options.cameraStartPosition.x"),
            _paramHandler.getParam<F32>("options.cameraStartPosition.y"),
            _paramHandler.getParam<F32>("options.cameraStartPosition.z")));
        vec2<F32> camOrientation(
            _paramHandler.getParam<F32>(
                "options.cameraStartOrientation.xOffsetDegrees"),
            _paramHandler.getParam<F32>(
                "options.cameraStartOrientation.yOffsetDegrees"));
        renderState().getCamera().setGlobalRotation(camOrientation.y /*yaw*/,
                                                    camOrientation.x /*pitch*/);
    } else {
        renderState().getCamera().setEye(vec3<F32>(0, 50, 0));
    }

    // Create an AI thread, but start it only if needed
    Kernel& kernel = Application::getInstance().getKernel();
    _aiTask = kernel.AddTask(
        Time::MillisecondsToMicroseconds(Config::AI_THREAD_UPDATE_FREQUENCY), 0,
        DELEGATE_BIND(&AI::AIManager::update, &AI::AIManager::getInstance()));

    addSelectionCallback(DELEGATE_BIND(&GUI::selectionChangeCallback,
                                       &GUI::getInstance(), this));

    SceneInput::PressReleaseActions cbks;
    cbks.second = DELEGATE_BIND(&Scene::findSelection, this);
    /// Input
    _input->addMouseMapping(Input::MouseButton::MB_Left, cbks);
    cbks.second = [this](){
        state().angleLR(SceneState::MoveDirection::NONE);
        state().angleUD(SceneState::MoveDirection::NONE);
    };
    _input->addMouseMapping(Input::MouseButton::MB_Right, cbks);
    
    cbks.first = [this](){
        _sceneGraph.deleteNode(_currentSelection, false);
    };
    cbks.second = [](){};
    _input->addKeyMapping(Input::KeyCode::KC_END, cbks);

    cbks.first = [this]() {
        if (_input->getKeyState(Input::KeyCode::KC_LCONTROL) == SceneInput::InputState::PRESSED) {
            GFX_DEVICE.increaseResolution();
        } else {
            Camera& cam = renderState().getCamera();
            F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
            if (currentCamMoveSpeedFactor < 50) {
                cam.setMoveSpeedFactor(currentCamMoveSpeedFactor + 1.0f);
                cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() + 1.0f);
            }
        }
    };
    _input->addKeyMapping(Input::KeyCode::KC_ADD, cbks);

    cbks.first = [this]() {
        if (_input->getKeyState(Input::KeyCode::KC_LCONTROL) == SceneInput::InputState::PRESSED) {
            GFX_DEVICE.decreaseResolution();
        } else {
            Camera& cam = renderState().getCamera();
            F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
            if (currentCamMoveSpeedFactor > 1.0f) {
                cam.setMoveSpeedFactor(currentCamMoveSpeedFactor - 1.0f);
                cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() - 1.0f);
            }
        }
    };
    _input->addKeyMapping(Input::KeyCode::KC_SUBTRACT, cbks);

    cbks.first = [this]() {state().moveFB(SceneState::MoveDirection::POSITIVE);};
    cbks.second = [this]() {state().moveFB(SceneState::MoveDirection::NONE);};
    _input->addKeyMapping(Input::KeyCode::KC_W, cbks);

    cbks.first = [this]() {state().moveFB(SceneState::MoveDirection::NEGATIVE); };
    _input->addKeyMapping(Input::KeyCode::KC_S, cbks);

    cbks.first = [this]() {state().moveLR(SceneState::MoveDirection::NEGATIVE);};
    cbks.second = [this]() {state().moveLR(SceneState::MoveDirection::NONE);};
    _input->addKeyMapping(Input::KeyCode::KC_A, cbks);
    cbks.first = [this]() {state().moveLR(SceneState::MoveDirection::POSITIVE);};
    _input->addKeyMapping(Input::KeyCode::KC_D, cbks);

    cbks.first = [this]()  {
        if (_input->getKeyState(Input::KeyCode::KC_LCONTROL) == SceneInput::InputState::PRESSED) {
            Application::getInstance().RequestShutdown();
        } else {
            state().roll(SceneState::MoveDirection::POSITIVE);
        }
    };
    cbks.second = [this]() { 
        state().roll(SceneState::MoveDirection::NONE); 
    };
    _input->addKeyMapping(Input::KeyCode::KC_Q, cbks);
    cbks.first = [this]() { state().roll(SceneState::MoveDirection::NEGATIVE); };
    _input->addKeyMapping(Input::KeyCode::KC_E, cbks);

    cbks.first = [this]() { state().angleLR(SceneState::MoveDirection::POSITIVE); };
    cbks.second = [this]() {state().angleLR(SceneState::MoveDirection::NONE); };
    _input->addKeyMapping(Input::KeyCode::KC_RIGHT, cbks);
    cbks.first = [this]() { state().angleLR(SceneState::MoveDirection::NEGATIVE); };
    _input->addKeyMapping(Input::KeyCode::KC_LEFT, cbks);

     cbks.first = [this]() { state().angleUD(SceneState::MoveDirection::NEGATIVE); };
     cbks.second = [this]() {state().angleUD(SceneState::MoveDirection::NONE); };
    _input->addKeyMapping(Input::KeyCode::KC_UP, cbks);
    cbks.first = [this]() { state().angleUD(SceneState::MoveDirection::POSITIVE); };
    _input->addKeyMapping(Input::KeyCode::KC_DOWN, cbks);

    cbks.first = [](){};
    cbks.second = []() {
        ParamHandler::getInstance().setParam(
            _ID("freezeLoopTime"),
            !ParamHandler::getInstance().getParam(_ID("freezeLoopTime"), false));
    };
    _input->addKeyMapping(Input::KeyCode::KC_P, cbks);

    cbks.second = []() {
        ParamHandler::getInstance().setParam(
            _ID("postProcessing.enableDepthOfField"),
            !ParamHandler::getInstance().getParam(
                _ID("postProcessing.enableDepthOfField"), false));
    };
    _input->addKeyMapping(Input::KeyCode::KC_F2, cbks);

    cbks.second = []() {
        ParamHandler::getInstance().setParam(
            "postProcessing.enableBloom",
            !ParamHandler::getInstance().getParam(
                "postProcessing.enableBloom", false));
    };
    _input->addKeyMapping(Input::KeyCode::KC_F3, cbks);

    cbks.second = [this]() { renderState().toggleSkeletons(); };
    _input->addKeyMapping(Input::KeyCode::KC_F4, cbks);

    cbks.second = [this]() { renderState().toggleAxisLines(); };
    _input->addKeyMapping(Input::KeyCode::KC_F5, cbks);

    cbks.second = [this]() { renderState().toggleWireframe(); };
    _input->addKeyMapping(Input::KeyCode::KC_F6, cbks);

    cbks.second = [this]() { renderState().toggleGeometry(); };
    _input->addKeyMapping(Input::KeyCode::KC_F7, cbks);

    cbks.second = [this]() { renderState().toggleDebugLines(); };
    _input->addKeyMapping(Input::KeyCode::KC_F8, cbks);

    cbks.second = [this]() { renderState().toggleBoundingBoxes(); };
    _input->addKeyMapping(Input::KeyCode::KC_B, cbks);

    cbks.second = []() {
        ParamHandler& param = ParamHandler::getInstance();
        LightManager::getInstance().togglePreviewShadowMaps();
        param.setParam<bool>(
            _ID("rendering.previewDepthBuffer"),
            !param.getParam<bool>(_ID("rendering.previewDepthBuffer"), false));
    };
    _input->addKeyMapping(Input::KeyCode::KC_F10, cbks);

    cbks.second =
        DELEGATE_BIND(&GFXDevice::Screenshot, &GFX_DEVICE, "screenshot_");
    _input->addKeyMapping(Input::KeyCode::KC_SYSRQ, cbks);


    cbks.second = [this]() {
        if (_input->getKeyState(Input::KeyCode::KC_LMENU) == SceneInput::InputState::PRESSED) {
            GFX_DEVICE.toggleFullScreen();
        }
    };

    cbks.second = [this]() {
        toggleFlashlight();
    };
    _input->addKeyMapping(Input::KeyCode::KC_F, cbks);

    _input->addKeyMapping(Input::KeyCode::KC_RETURN, cbks);
    _loadComplete = true;
    return _loadComplete;
}

bool Scene::unload() {
    // prevent double unload calls
    if (!checkLoadFlag()) {
        return false;
    }
    clearTasks();
    /// Destroy physics (:D)
    PHYSICS_DEVICE.setPhysicsScene(nullptr);
    LightManager::getInstance().clear();
    clearObjects();
    _loadComplete = false;
    return true;
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
    if (_paramHandler.getParam<bool>("options.autoCookPhysicsAssets")) {
        _cookCollisionMeshesScheduled = true;
    }
    return true;
}

bool Scene::initializeAI(bool continueOnErrors) {
    _aiTask->startTask(Task::TaskPriority::MAX);
    return true;
}

 /// Shut down AIManager thread
bool Scene::deinitializeAI(bool continueOnErrors) { 
    if (_aiTask.get()) {
        _aiTask->stopTask();
        _aiTask.reset();
    }
    WAIT_FOR_CONDITION(!_aiTask.get());
    
    return true;
}

void Scene::clearObjects() {
    for (U8 i = 0; i < _terrainInfoArray.size(); ++i) {
        RemoveResource(_terrainInfoArray[i]);
    }
    _terrainInfoArray.clear();

    while (!_modelDataArray.empty()) {
        _modelDataArray.pop();
    }
    _vegetationDataArray.clear();

    _sceneGraph.unload();
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
    _sceneGraph.sceneUpdate(deltaTime, _sceneState);
    findHoverTarget();
    SceneGraphNode_ptr flashLight = _flashLight.lock();

    if (flashLight) {
        const Camera& cam = renderState().getCameraConst();
        flashLight->getComponent<PhysicsComponent>()->setPosition(cam.getEye());
        flashLight->getComponent<PhysicsComponent>()->setRotation(cam.getEuler());
    }
}

void Scene::onLostFocus() {
    state().resetMovement();
#ifndef _DEBUG
    //_paramHandler.setParam(_ID("freezeLoopTime"), true);
#endif
}

void Scene::registerTask(Task_ptr taskItem) { 
    _tasks.push_back(taskItem);
}

void Scene::clearTasks() {
    Console::printfn(Locale::get(_ID("STOP_SCENE_TASKS")));
    // Calls the destructor for each task killing it's associated thread
    _tasks.clear();
}

void Scene::removeTask(I64 taskGUID) {
    for (vectorImpl<Task_ptr>::iterator it = std::begin(_tasks);
         it != std::end(_tasks); ++it) {
        if ((*it)->getGUID() == taskGUID) {
            (*it)->stopTask();
            _tasks.erase(it);
            return;
        }
    }
}

void Scene::processInput(const U64 deltaTime) {
}

void Scene::processGUI(const U64 deltaTime) {
    for (U16 i = 0; i < _guiTimers.size(); ++i) {
        _guiTimers[i] += Time::MicrosecondsToMilliseconds<D32>(deltaTime);
    }
}

void Scene::processTasks(const U64 deltaTime) {
    for (U16 i = 0; i < _taskTimers.size(); ++i) {
        _taskTimers[i] += Time::MicrosecondsToMilliseconds<D32>(deltaTime);
    }
}

TerrainDescriptor* Scene::getTerrainInfo(const stringImpl& terrainName) {
    for (U8 i = 0; i < _terrainInfoArray.size(); i++) {
        if (terrainName.compare(
                _terrainInfoArray[i]->getVariable("terrainName")) == 0) {
            return _terrainInfoArray[i];
        }
    }

    DIVIDE_ASSERT(
        false,
        "Scene error: INVALID TERRAIN NAME FOR INFO LOOKUP");  // not found;
    return _terrainInfoArray[0];
}

void Scene::debugDraw(RenderStage stage) {
#ifdef _DEBUG
    const SceneRenderState::GizmoState& currentGizmoState = renderState().gizmoState();

    GFX_DEVICE.drawDebugAxis(currentGizmoState != SceneRenderState::GizmoState::NO_GIZMO);

    if (currentGizmoState == SceneRenderState::GizmoState::SELECTED_GIZMO) {
        SceneGraphNode_ptr selection(_currentSelection.lock());
        if (selection != nullptr) {
            selection->getComponent<RenderingComponent>()->drawDebugAxis();
        }
    }
#endif
    if (stage == RenderStage::DISPLAY) {
        
        LightManager::getInstance().drawLightImpostors();

        // Draw bounding boxes, skeletons, axis gizmo, etc.
        GFX_DEVICE.debugDraw(renderState());

        // Show NavMeshes
        AI::AIManager::getInstance().debugDraw(false);
    }
}

void Scene::findHoverTarget() {
    const vec2<U16>& displaySize = Application::getInstance().getWindowManager().getWindowDimensions();
    const vec2<F32>& zPlanes = renderState().getCameraConst().getZPlanes();
    const vec2<I32>& mousePos = _input->getMousePosition();

    F32 mouseX = to_float(mousePos.x);
    F32 mouseY = displaySize.height - to_float(mousePos.y) - 1;

    vec3<F32> startRay = renderState().getCameraConst().unProject(mouseX, mouseY, 0.0f);
    vec3<F32> endRay = renderState().getCameraConst().unProject(mouseX, mouseY, 1.0f);
    // see if we select another one
    _sceneSelectionCandidates.clear();
    // get the list of visible nodes (use Z_PRE_PASS because the nodes are sorted by depth, front to back)
    RenderPassCuller::VisibleNodeList& nodes = SceneManager::getInstance().getVisibleNodesCache(RenderStage::Z_PRE_PASS);

    // Cast the picking ray and find items between the nearPlane and far Plane
    Ray mouseRay(startRay, startRay.direction(endRay));
    for (SceneGraphNode_wptr node : nodes) {
        SceneGraphNode_ptr nodePtr = node.lock();
        if (nodePtr) {
            nodePtr->intersect(mouseRay, zPlanes.x, zPlanes.y, _sceneSelectionCandidates, false);                   
        }
    }

    if (!_sceneSelectionCandidates.empty()) {
        _currentHoverTarget = _sceneSelectionCandidates.front();
        SceneNode* node = _currentHoverTarget.lock()->getNode();
        if (node->getType() == SceneNodeType::TYPE_OBJECT3D) {
            if (static_cast<Object3D*>(node)->getObjectType() == Object3D::ObjectType::SUBMESH) {
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
};