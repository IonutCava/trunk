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

#include "Physics/Headers/PhysicsSceneInterface.h"

namespace Divide {

namespace {
struct selectionQueueDistanceFrontToBack {
    selectionQueueDistanceFrontToBack(const vec3<F32>& eyePos)
        : _eyePos(eyePos) {}

    bool operator()(SceneGraphNode* const a, SceneGraphNode* const b) const {
        F32 dist_a =
            a->getBoundingBoxConst().nearestDistanceFromPointSquared(_eyePos);
        F32 dist_b =
            b->getBoundingBoxConst().nearestDistanceFromPointSquared(_eyePos);
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
      _paramHandler(ParamHandler::getInstance()),
      _currentSelection(nullptr),
      _currentSky(nullptr)
{
    _sceneTimer = 0UL;
    _sceneGraph.load();
    _input.reset(MemoryManager_NEW SceneInput(*this));
#ifdef _DEBUG
    _linesPrimitive[to_uint(DebugLines::DEBUG_LINE_OBJECT_TO_TARGET)] =
        GFX_DEVICE.getOrCreatePrimitive(false);
    _linesPrimitive[to_uint(DebugLines::DEBUG_LINE_OBJECT_TO_TARGET)]->name("LinesObjectToTarget");
    _linesPrimitive[to_uint(DebugLines::DEBUG_LINE_RAY_PICK)] =
        GFX_DEVICE.getOrCreatePrimitive(false);
    _linesPrimitive[to_uint(DebugLines::DEBUG_LINE_RAY_PICK)]->name("LinesRayPick");
#endif
}

Scene::~Scene()
{
#ifdef _DEBUG
    _linesPrimitive[to_uint(DebugLines::DEBUG_LINE_OBJECT_TO_TARGET)]
        ->_canZombify = true;
    _linesPrimitive[to_uint(DebugLines::DEBUG_LINE_RAY_PICK)]->_canZombify =
        true;
#endif
}

bool Scene::frameStarted() { return true; }

bool Scene::frameEnded() { return true; }

bool Scene::idle() {  // Called when application is idle
    if (!_modelDataArray.empty()) {
        loadXMLAssets(true);
    }

    if (_sceneGraph.getRoot().getChildren().empty()) {
        return false;
    }

    _sceneGraph.idle();

    Attorney::SceneRenderStateScene::playAnimations(
        renderState(),
        ParamHandler::getInstance().getParam<bool>("mesh.playAnimations"));

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

void Scene::preRender() {
}

void Scene::postRender() {
#ifdef _DEBUG
    if (renderState().drawDebugLines()) {
        U32 linePickIdx = to_uint(DebugLines::DEBUG_LINE_RAY_PICK);
        if (!_lines[linePickIdx].empty()) {
            GFX_DEVICE.drawLines(*_linesPrimitive[linePickIdx],
                                 _lines[linePickIdx], 3.0f, mat4<F32>(),
                                 vec4<I32>(), false, false);
        }
    }
    U32 lineTargetIdx = to_uint(DebugLines::DEBUG_LINE_OBJECT_TO_TARGET);
    if (!_lines[lineTargetIdx].empty() &&
        renderState().drawDebugTargetLines()) {
        GFX_DEVICE.drawLines(*_linesPrimitive[lineTargetIdx],
                             _lines[lineTargetIdx], 3.0f, mat4<F32>(),
                             vec4<I32>(), false, false);
    }
#endif
}

void Scene::addPatch(vectorImpl<FileData>& data) {}

void Scene::loadXMLAssets(bool singleStep) {
    while (!_modelDataArray.empty()) {
        const FileData& it = _modelDataArray.top();
        // vegetation is loaded elsewhere
        if (it.type == GeometryType::VEGETATION) {
            _vegetationDataArray.push_back(it);
        } else {
            loadModel(it);
        }
        _modelDataArray.pop();

        if (singleStep) {
            return;
        }
    }
}

bool Scene::loadModel(const FileData& data) {
    if (data.type == GeometryType::PRIMITIVE) {
        return loadGeometry(data);
    }

    ResourceDescriptor model(data.ModelName);
    model.setResourceLocation(data.ModelName);
    model.setFlag(true);
    Mesh* thisObj = CreateResource<Mesh>(model);
    if (!thisObj) {
        Console::errorfn(Locale::get("ERROR_SCENE_LOAD_MODEL"),
                         data.ModelName.c_str());
        return false;
    }

    SceneGraphNode& meshNode =
        _sceneGraph.getRoot().createNode(*thisObj, data.ItemName);
    meshNode.getComponent<RenderingComponent>()->castsShadows(
        data.castsShadows);
    meshNode.getComponent<RenderingComponent>()->receivesShadows(
        data.receivesShadows);
    meshNode.getComponent<PhysicsComponent>()->setScale(data.scale);
    meshNode.getComponent<PhysicsComponent>()->setRotation(data.orientation);
    meshNode.getComponent<PhysicsComponent>()->setPosition(data.position);
    if (data.staticUsage) {
        meshNode.usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
    }
    if (data.navigationUsage) {
        meshNode.getComponent<NavigationComponent>()->navigationContext(
            NavigationComponent::NavigationContext::NODE_OBSTACLE);
    }
    if (data.physicsUsage) {
        meshNode.getComponent<PhysicsComponent>()->physicsGroup(
            data.physicsPushable ? PhysicsComponent::PhysicsGroup::NODE_COLLIDE
                                 : PhysicsComponent::PhysicsGroup::NODE_COLLIDE_NO_PUSH);
    }
    if (data.useHighDetailNavMesh) {
        meshNode.getComponent<NavigationComponent>()->navigationDetailOverride(
            true);
    }
    return true;
}

bool Scene::loadGeometry(const FileData& data) {
    Object3D* thisObj;
    ResourceDescriptor item(data.ItemName);
    item.setResourceLocation(data.ModelName);
    if (data.ModelName.compare("Box3D") == 0) {
        thisObj = CreateResource<Box3D>(item);
        static_cast<Box3D*>(thisObj)->setSize(data.data);
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
        Console::errorfn(Locale::get("ERROR_SCENE_UNSUPPORTED_GEOM"),
                         data.ModelName.c_str());
        return false;
    }
    STUBBED("Load material from XML disabled for primitives! - Ionut")
    Material* tempMaterial =
        nullptr /*XML::loadMaterial( stringAlg::fromBase( data.ItemName + "_material" ) )*/;
    if (!tempMaterial) {
        ResourceDescriptor materialDescriptor(data.ItemName + "_material");
        tempMaterial = CreateResource<Material>(materialDescriptor);
        tempMaterial->setDiffuse(data.color);
        tempMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    }

    thisObj->setMaterialTpl(tempMaterial);
    SceneGraphNode& thisObjSGN = _sceneGraph.getRoot().createNode(*thisObj);
    thisObjSGN.getComponent<PhysicsComponent>()->setScale(data.scale);
    thisObjSGN.getComponent<PhysicsComponent>()->setRotation(data.orientation);
    thisObjSGN.getComponent<PhysicsComponent>()->setPosition(data.position);
    thisObjSGN.getComponent<RenderingComponent>()->castsShadows(
        data.castsShadows);
    thisObjSGN.getComponent<RenderingComponent>()->receivesShadows(
        data.receivesShadows);
    if (data.staticUsage) {
        thisObjSGN.usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
    }
    if (data.navigationUsage) {
        thisObjSGN.getComponent<NavigationComponent>()->navigationContext(
            NavigationComponent::NavigationContext::NODE_OBSTACLE);
    }
    if (data.physicsUsage) {
        thisObjSGN.getComponent<PhysicsComponent>()->physicsGroup(
            data.physicsPushable ? PhysicsComponent::PhysicsGroup::NODE_COLLIDE
                                 : PhysicsComponent::PhysicsGroup::NODE_COLLIDE_NO_PUSH);
    }
    if (data.useHighDetailNavMesh) {
        thisObjSGN.getComponent<NavigationComponent>()
            ->navigationDetailOverride(true);
    }
    return true;
}

SceneGraphNode& Scene::addParticleEmitter(const stringImpl& name,
                                          const ParticleData& data,
                                          SceneGraphNode& parentNode) {
    assert(!name.empty());

    ResourceDescriptor particleEmitter(name);
    ParticleEmitter* emitter = CreateResource<ParticleEmitter>(particleEmitter);

    DIVIDE_ASSERT(
        emitter != nullptr,
        "Scene::addParticleEmitter error: Could not instantiate emitter!");

    emitter->initData(std::make_shared<ParticleData>(data));

    return parentNode.addNode(*emitter);
}

SceneGraphNode& Scene::addLight(Light& lightItem,
                                SceneGraphNode& parentNode) {
    lightItem.setCastShadows(lightItem.getLightType() != LightType::POINT);

    return parentNode.addNode(lightItem);
}

SceneGraphNode& Scene::addLight(LightType type,
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
        std::to_string(LightManager::getInstance().getLights().size()));
    defaultLight.setEnumValue(to_uint(type));
    return addLight(*CreateResource<Light>(defaultLight), parentNode);
}

SceneGraphNode& Scene::addSky(Sky* const skyItem) {
    assert(skyItem != nullptr);
    return _sceneGraph.getRoot().createNode(*skyItem);
}

bool Scene::load(const stringImpl& name, GUI* const guiInterface) {
    STUBBED("ToDo: load skyboxes from XML")
    _GUI = guiInterface;
    _name = name;

    _GFX.enableFog(_sceneState.fogDescriptor()._fogDensity,
                   _sceneState.fogDescriptor()._fogColor);

    loadXMLAssets();
    SceneGraphNode& root = _sceneGraph.getRoot();
    // Add terrain from XML
    if (!_terrainInfoArray.empty()) {
        for (TerrainDescriptor* terrainInfo : _terrainInfoArray) {
            ResourceDescriptor terrain(terrainInfo->getVariable("terrainName"));
            Terrain* temp = CreateResource<Terrain>(terrain);
            SceneGraphNode& terrainTemp = root.createNode(*temp);
            terrainTemp.setActive(terrainInfo->getActive());
            terrainTemp.usageContext(SceneGraphNode::UsageContext::NODE_STATIC);

            NavigationComponent* nComp =
                terrainTemp.getComponent<NavigationComponent>();
            nComp->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);

            PhysicsComponent* pComp =
                terrainTemp.getComponent<PhysicsComponent>();
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

    vec3<F32> ambientColor(0.1f, 0.1f, 0.1f);
    LightManager::getInstance().setAmbientLight(ambientColor);

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
    cbks.second = [this](){ state().angleLR(0); state().angleUD(0); };
    _input->addMouseMapping(Input::MouseButton::MB_Right, cbks);
    
    cbks.first = DELEGATE_BIND(&Scene::deleteSelection, this);
    cbks.second = [](){};
    _input->addKeyMapping(Input::KeyCode::KC_END, cbks);

    cbks.first = [this]() {
        Camera& cam = renderState().getCamera();
        F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
        if (currentCamMoveSpeedFactor < 50) {
            cam.setMoveSpeedFactor(currentCamMoveSpeedFactor + 1.0f);
            cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() + 1.0f);
        }
    };
    _input->addKeyMapping(Input::KeyCode::KC_ADD, cbks);

    cbks.first = [this]() {
        Camera& cam = renderState().getCamera();
        F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
        if (currentCamMoveSpeedFactor > 1.0f) {
            cam.setMoveSpeedFactor(currentCamMoveSpeedFactor - 1.0f);
            cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() - 1.0f);
        }
    };
    _input->addKeyMapping(Input::KeyCode::KC_SUBTRACT, cbks);

    cbks.first = [this]() {state().moveFB(1); };
    cbks.second = [this]() {state().moveFB(0); };
    _input->addKeyMapping(Input::KeyCode::KC_W, cbks);
    cbks.first = [this]() {state().moveFB(-1); };
    cbks.second = [this]() {state().moveFB(0); };
    _input->addKeyMapping(Input::KeyCode::KC_S, cbks);

    cbks.first = [this]() {state().moveLR(-1); };
    cbks.second = [this]() {state().moveLR(0); };
    _input->addKeyMapping(Input::KeyCode::KC_A, cbks);
    cbks.first = [this]() {state().moveLR(1); };
    cbks.second = [this]() {state().moveLR(0); };
    _input->addKeyMapping(Input::KeyCode::KC_D, cbks);

    cbks.first = [this]() { state().roll(1); };
    cbks.second = [this]() {state().roll(0); };
    _input->addKeyMapping(Input::KeyCode::KC_Q, cbks);
    cbks.first = [this]() { state().roll(-1); };
    cbks.second = [this]() {state().roll(0); };
    _input->addKeyMapping(Input::KeyCode::KC_E, cbks);

    cbks.first = [this]() { state().angleLR(1); };
    cbks.second = [this]() {state().angleLR(0); };
    _input->addKeyMapping(Input::KeyCode::KC_RIGHT, cbks);
    cbks.first = [this]() { state().angleLR(-1); };
    cbks.second = [this]() {state().angleLR(0); };
    _input->addKeyMapping(Input::KeyCode::KC_LEFT, cbks);

     cbks.first = [this]() { state().angleUD(-1); };
     cbks.second = [this]() {state().angleUD(0); };
    _input->addKeyMapping(Input::KeyCode::KC_UP, cbks);
    cbks.first = [this]() { state().angleUD(1); };
    cbks.second = [this]() {state().angleUD(0); };
    _input->addKeyMapping(Input::KeyCode::KC_DOWN, cbks);

    cbks.first = [](){};
    cbks.second = []() {
        ParamHandler::getInstance().setParam(
            "freezeLoopTime",
            !ParamHandler::getInstance().getParam("freezeLoopTime", false));
    };
    _input->addKeyMapping(Input::KeyCode::KC_P, cbks);

    cbks.second = []() {
        ParamHandler::getInstance().setParam(
            "postProcessing.enableDepthOfField",
            !ParamHandler::getInstance().getParam(
                "postProcessing.enableDepthOfField", false));
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

#ifdef _DEBUG
    cbks.second =
        [this]() {
            for (U32 i = 0; i < to_uint(DebugLines::COUNT); ++i) {
                _lines[i].clear();
            }
    };
    _input->addKeyMapping(Input::KeyCode::KC_F9, cbks);
#endif

    cbks.second = []() {
        ParamHandler& param = ParamHandler::getInstance();
        LightManager::getInstance().togglePreviewShadowMaps();
        param.setParam<bool>(
            "rendering.previewDepthBuffer",
            !param.getParam<bool>("rendering.previewDepthBuffer", false));
    };
    _input->addKeyMapping(Input::KeyCode::KC_F10, cbks);

    cbks.second =
        DELEGATE_BIND(&GFXDevice::Screenshot, &GFX_DEVICE, "screenshot_");
    _input->addKeyMapping(Input::KeyCode::KC_SYSRQ, cbks);
    _loadComplete = true;
    return _loadComplete;
}

bool Scene::unload() {
    // prevent double unload calls
    if (!checkLoadFlag()) {
        return false;
    }
    clearTasks();
    clearPhysics();
    clearObjects();
    clearLights();
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

void Scene::clearPhysics() { PHYSICS_DEVICE.setPhysicsScene(nullptr); }

bool Scene::initializeAI(bool continueOnErrors) {
    _aiTask->startTask(Task::TaskPriority::MAX);
    return true;
}

bool Scene::deinitializeAI(
    bool continueOnErrors) {  /// Shut down AIManager thread
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

void Scene::clearLights() { LightManager::getInstance().clear(); }

bool Scene::updateCameraControls() {
    Camera& cam = renderState().getCamera();
    switch (cam.getType()) {
        default:
        case Camera::CameraType::FREE_FLY: {
            if (state().angleLR()) {
                cam.rotateYaw(CLAMPED<I32>(state().angleLR(), -1, 1));
            }
            if (state().angleUD()) {
                cam.rotatePitch(CLAMPED<I32>(state().angleUD(), -1, 1));
            }
            if (state().roll()) {
                cam.rotateRoll(CLAMPED<I32>(state().roll(), -1, 1));
            }
            if (state().moveFB()) {
                cam.moveForward(CLAMPED<I32>(state().moveFB(), -1, 1));
            }
            if (state().moveLR()) {
                cam.moveStrafe(CLAMPED<I32>(state().moveLR(), -1, 1));
            }
        } break;
    }

    state().cameraUpdated((state().moveFB() || state().moveLR() ||
                           state().angleLR() || state().angleUD() ||
                           state().roll()));
    return state().cameraUpdated();
}

void Scene::updateSceneState(const U64 deltaTime) {
    _sceneTimer += deltaTime;
    updateSceneStateInternal(deltaTime);
    state().cameraUnderwater(renderState().getCamera().getEye().y <
                             state().waterLevel());
    _sceneGraph.sceneUpdate(deltaTime, _sceneState);
}

void Scene::deleteSelection() {
    if (_currentSelection != nullptr) {
        _currentSelection->scheduleDeletion();
    }
}

void Scene::onLostFocus() {
    state().resetMovement();
#ifndef _DEBUG
    _paramHandler.setParam("freezeLoopTime", true);
#endif
}

void Scene::registerTask(Task_ptr taskItem) { _tasks.push_back(taskItem); }

void Scene::clearTasks() {
    Console::printfn(Locale::get("STOP_SCENE_TASKS"));
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
    const SceneRenderState::GizmoState& currentGizmoState =
        renderState().gizmoState();

    GFX_DEVICE.drawDebugAxis(currentGizmoState != SceneRenderState::GizmoState::NO_GIZMO);

    if (currentGizmoState == SceneRenderState::GizmoState::SELECTED_GIZMO) {
        if (_currentSelection != nullptr) {
            _currentSelection->getComponent<RenderingComponent>()
                ->drawDebugAxis();
        }
    }
#endif
    if (stage == RenderStage::DISPLAY) {
        // Draw bounding boxes, skeletons, axis gizmo, etc.
        GFX_DEVICE.debugDraw(renderState());
        // Show NavMeshes
        AI::AIManager::getInstance().debugDraw(false);
    }
}

void Scene::findSelection() {
    vec2<I32> mousePos = _input->getMousePosition();
    F32 mouseX = static_cast<F32>(mousePos.x);
    F32 mouseY = static_cast<F32>(mousePos.y);

    mouseY = renderState().cachedResolution().height - mouseY - 1;
    vec3<F32> startRay = renderState().getCameraConst().unProject(
        vec3<F32>(mouseX, mouseY, 0.0f));
    vec3<F32> endRay = renderState().getCameraConst().unProject(
        vec3<F32>(mouseX, mouseY, 1.0f));
    const vec2<F32>& zPlanes = renderState().getCameraConst().getZPlanes();

    // deselect old node
    if (_currentSelection) {
        _currentSelection->setSelected(false);
    }

    _currentSelection = nullptr;

    // see if we select another one
    _sceneSelectionCandidates.clear();
    // Cast the picking ray and find items between the nearPlane (with an
    // offset) and limit the range to half of the far plane
    _sceneGraph.intersect(Ray(startRay, startRay.direction(endRay)),
                          zPlanes.x + 0.5f, zPlanes.y * 0.5f,
                          _sceneSelectionCandidates);

    if (!_sceneSelectionCandidates.empty()) {
        std::sort(std::begin(_sceneSelectionCandidates),
                  std::end(_sceneSelectionCandidates),
                  selectionQueueDistanceFrontToBack(
                      renderState().getCameraConst().getEye()));
        _currentSelection = _sceneSelectionCandidates[0];
        // set it's state to selected
        _currentSelection->setSelected(true);
#ifdef _DEBUG
        _lines[to_uint(DebugLines::DEBUG_LINE_RAY_PICK)].push_back(
            Line(startRay, endRay, vec4<U8>(0, 255, 0, 255)));
#endif
    }

    for (DELEGATE_CBK<>& cbk : _selectionChangeCallbacks) {
        cbk();
    }
}
};