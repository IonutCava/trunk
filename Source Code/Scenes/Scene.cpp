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

Scene::Scene()
    : Resource("temp_scene"),
      _GFX(GFX_DEVICE),
      _LRSpeedFactor(5.0f),
      _loadComplete(false),
      _cookCollisionMeshesScheduled(false),
      _paramHandler(ParamHandler::getInstance()),
      _currentSelection(nullptr),
      _currentSky(nullptr) {
    _mousePressed[OIS::MB_Left] = false;
    _mousePressed[OIS::MB_Right] = false;
    _mousePressed[OIS::MB_Middle] = false;
    _mousePressed[OIS::MB_Button3] = false;
    _mousePressed[OIS::MB_Button4] = false;
    _mousePressed[OIS::MB_Button5] = false;
    _mousePressed[OIS::MB_Button6] = false;
    _mousePressed[OIS::MB_Button7] = false;
}

Scene::~Scene() {}

bool Scene::frameStarted() { return true; }

bool Scene::frameEnded() { return true; }

bool Scene::idle() {  // Called when application is idle
    if (!_modelDataArray.empty()) {
        loadXMLAssets(true);
    }

    if (!_sceneGraph.getRoot() ||
        _sceneGraph.getRoot()->getChildren().empty()) {
        return false;
    }

    _sceneGraph.idle();

    if (_cookCollisionMeshesScheduled && checkLoadFlag()) {
        if (GFX_DEVICE.getFrameCount() > 1) {
            _sceneGraph.getRoot()
                ->getComponent<PhysicsComponent>()
                ->cookCollisionMesh(_name);
            _cookCollisionMeshesScheduled = false;
        }
    }

    return true;
}

void Scene::onCameraChange() {
    if (_sceneGraph.getRoot()) {
        _sceneGraph.getRoot()->onCameraChange();
    }
}

void Scene::postRender() {
#ifdef _DEBUG
    if (renderState().drawDebugLines()) {
        if (!_lines[DEBUG_LINE_RAY_PICK].empty()) {
            GFX_DEVICE.drawLines(_lines[DEBUG_LINE_OBJECT_TO_TARGET],
                                 mat4<F32>(), vec4<I32>(), false, false);
        }
    }
    if (!_lines[DEBUG_LINE_OBJECT_TO_TARGET].empty() &&
        renderState().drawDebugTargetLines()) {
        GFX_DEVICE.drawLines(_lines[DEBUG_LINE_OBJECT_TO_TARGET], mat4<F32>(),
                             vec4<I32>(), false, false);
    }
#endif
}

void Scene::addPatch(vectorImpl<FileData>& data) {}

void Scene::loadXMLAssets(bool singleStep) {
    while (!_modelDataArray.empty()) {
        const FileData& it = _modelDataArray.top();
        // vegetation is loaded elsewhere
        if (it.type == VEGETATION) {
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
    if (data.type == PRIMITIVE) {
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

    SceneGraphNode* meshNode =
        _sceneGraph.getRoot()->createNode(thisObj, data.ItemName);
    meshNode->getComponent<RenderingComponent>()->castsShadows(
        data.castsShadows);
    meshNode->getComponent<RenderingComponent>()->receivesShadows(
        data.receivesShadows);
    meshNode->getComponent<PhysicsComponent>()->setScale(data.scale);
    meshNode->getComponent<PhysicsComponent>()->setRotation(data.orientation);
    meshNode->getComponent<PhysicsComponent>()->setPosition(data.position);
    if (data.staticUsage) {
        meshNode->usageContext(SceneGraphNode::NODE_STATIC);
    }
    if (data.navigationUsage) {
        meshNode->getComponent<NavigationComponent>()->navigationContext(
            NavigationComponent::NODE_OBSTACLE);
    }
    if (data.physicsUsage) {
        meshNode->getComponent<PhysicsComponent>()->physicsGroup(
            data.physicsPushable ? PhysicsComponent::NODE_COLLIDE
                                 : PhysicsComponent::NODE_COLLIDE_NO_PUSH);
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
        quadMask.b.b0 = 1;
        item.setBoolMask(quadMask);
        thisObj = CreateResource<Quad3D>(item);
        static_cast<Quad3D*>(thisObj)
            ->setCorner(Quad3D::TOP_LEFT, vec3<F32>(0, 1, 0));
        static_cast<Quad3D*>(thisObj)
            ->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(1, 1, 0));
        static_cast<Quad3D*>(thisObj)
            ->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(0, 0, 0));
        static_cast<Quad3D*>(thisObj)
            ->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(1, 0, 0));
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
        tempMaterial->setShadingMode(Material::SHADING_BLINN_PHONG);
    }

    thisObj->setMaterialTpl(tempMaterial);
    SceneGraphNode* thisObjSGN = _sceneGraph.getRoot()->createNode(thisObj);
    thisObjSGN->getComponent<PhysicsComponent>()->setScale(data.scale);
    thisObjSGN->getComponent<PhysicsComponent>()->setRotation(data.orientation);
    thisObjSGN->getComponent<PhysicsComponent>()->setPosition(data.position);
    thisObjSGN->getComponent<RenderingComponent>()->castsShadows(
        data.castsShadows);
    thisObjSGN->getComponent<RenderingComponent>()->receivesShadows(
        data.receivesShadows);
    if (data.staticUsage) {
        thisObjSGN->usageContext(SceneGraphNode::NODE_STATIC);
    }
    if (data.navigationUsage) {
        thisObjSGN->getComponent<NavigationComponent>()->navigationContext(
            NavigationComponent::NODE_OBSTACLE);
    }
    if (data.physicsUsage) {
        thisObjSGN->getComponent<PhysicsComponent>()->physicsGroup(
            data.physicsPushable ? PhysicsComponent::NODE_COLLIDE
                                 : PhysicsComponent::NODE_COLLIDE_NO_PUSH);
    }
    if (data.useHighDetailNavMesh) {
        thisObjSGN->getComponent<NavigationComponent>()
            ->navigationDetailOverride(true);
    }
    return true;
}

SceneGraphNode* const Scene::addParticleEmitter(const stringImpl& name,
                                                const ParticleData& data,
                                                SceneGraphNode* parentNode) {
    assert(parentNode != nullptr && !name.empty());

    ResourceDescriptor particleEmitter(name);
    ParticleEmitter* emitter = CreateResource<ParticleEmitter>(particleEmitter);

    DIVIDE_ASSERT(
        emitter != nullptr,
        "Scene::addParticleEmitter error: Could not instantiate emitter!");

    emitter->initData(std::make_shared<ParticleData>(data));

    return parentNode->addNode(emitter);
}

SceneGraphNode* Scene::addLight(Light* const lightItem,
                                SceneGraphNode* const parentNode) {
    assert(lightItem != nullptr);

    lightItem->setCastShadows(lightItem->getType() != LIGHT_TYPE_POINT);

    SceneGraphNode* returnNode = nullptr;
    if (parentNode) {
        returnNode = parentNode->addNode(lightItem);
    } else {
        returnNode = _sceneGraph.getRoot()->addNode(lightItem);
    }
    return returnNode;
}

SceneGraphNode* Scene::addLight(LightType type,
                                SceneGraphNode* const parentNode) {
    const char* lightType = "";
    switch (type) {
        case LIGHT_TYPE_DIRECTIONAL:
            lightType = "Default_directional_light ";
            break;
        case LIGHT_TYPE_POINT:
            lightType = "Default_point_light_";
            break;
        case LIGHT_TYPE_SPOT:
            lightType = "Default_spot_light_";
            break;
    }

    ResourceDescriptor defaultLight(
        lightType +
        Util::toString(LightManager::getInstance().getLights().size()));
    defaultLight.setEnumValue(type);
    return addLight(CreateResource<Light>(defaultLight), parentNode);
}

SceneGraphNode* Scene::addSky(Sky* const skyItem) {
    assert(skyItem != nullptr);
    return _sceneGraph.getRoot()->createNode(skyItem);
}

bool Scene::load(const stringImpl& name, GUI* const guiInterface) {
    STUBBED("ToDo: load skyboxes from XML")
    _GUI = guiInterface;
    _name = name;

    _GFX.enableFog(_sceneState.getFogDesc()._fogDensity,
                   _sceneState.getFogDesc()._fogColor);

    loadXMLAssets();
    SceneGraphNode* root = _sceneGraph.getRoot();
    // Add terrain from XML
    if (!_terrainInfoArray.empty()) {
        for (TerrainDescriptor* terrainInfo : _terrainInfoArray) {
            ResourceDescriptor terrain(terrainInfo->getVariable("terrainName"));
            Terrain* temp = CreateResource<Terrain>(terrain);
            SceneGraphNode* terrainTemp = root->createNode(temp);
            terrainTemp->setActive(terrainInfo->getActive());
            terrainTemp->usageContext(SceneGraphNode::NODE_STATIC);

            NavigationComponent* nComp =
                terrainTemp->getComponent<NavigationComponent>();
            nComp->navigationContext(NavigationComponent::NODE_OBSTACLE);

            PhysicsComponent* pComp =
                terrainTemp->getComponent<PhysicsComponent>();
            pComp->physicsGroup(terrainInfo->getCreatePXActor()
                                    ? PhysicsComponent::NODE_COLLIDE_NO_PUSH
                                    : PhysicsComponent::NODE_COLLIDE_IGNORE);
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
    _aiTask.reset(kernel.AddTask(
        Time::MillisecondsToMicroseconds(1000.0 /
                                         Config::AI_THREAD_UPDATE_FREQUENCY),
        0,
        DELEGATE_BIND(&AI::AIManager::update, &AI::AIManager::getInstance())));

    addSelectionCallback(DELEGATE_BIND(&GUI::selectionChangeCallback,
                                       &GUI::getInstance(), this));
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
    _aiTask->startTask();
    return true;
}

bool Scene::deinitializeAI(
    bool continueOnErrors) {  /// Shut down AIManager thread
    if (_aiTask.get()) {
        _aiTask->stopTask();
        _aiTask.reset();
    }
    while (_aiTask.get()) {
    }

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
}

void Scene::clearLights() { LightManager::getInstance().clear(); }

bool Scene::updateCameraControls() {
    Camera& cam = renderState().getCamera();
    switch (cam.getType()) {
        default:
        case Camera::FREE_FLY: {
            if (state()._angleLR) {
                cam.rotateYaw(CLAMPED<I32>(state()._angleLR, -1, 1));
            }
            if (state()._angleUD) {
                cam.rotatePitch(CLAMPED<I32>(state()._angleUD, -1, 1));
            }
            if (state()._roll) {
                cam.rotateRoll(CLAMPED<I32>(state()._roll, -1, 1));
            }
            if (state()._moveFB) {
                cam.moveForward(CLAMPED<I32>(state()._moveFB, -1, 1));
            }
            if (state()._moveLR) {
                cam.moveStrafe(CLAMPED<I32>(state()._moveLR, -1, 1));
            }
        } break;
    }

    state()._cameraUpdated =
        (state()._moveFB || state()._moveLR || state()._angleLR ||
         state()._angleUD || state()._roll);
    return state()._cameraUpdated;
}

void Scene::updateSceneState(const U64 deltaTime) {
    updateSceneStateInternal(deltaTime);
    state()._cameraUnderwater =
        renderState().getCamera().getEye().y < state()._waterHeight;
    _sceneGraph.sceneUpdate(deltaTime, _sceneState);
}

void Scene::deleteSelection() {
    if (_currentSelection != nullptr) {
        _currentSelection->scheduleDeletion();
    }
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

void Scene::debugDraw(const RenderStage& stage) {
#ifdef _DEBUG
    const SceneRenderState::GizmoState& currentGizmoState =
        renderState().gizmoState();

    GFX_DEVICE.drawDebugAxis(currentGizmoState != SceneRenderState::NO_GIZMO);

    if (currentGizmoState == SceneRenderState::SELECTED_GIZMO) {
        if (_currentSelection != nullptr &&
            _currentSelection->getComponent<RenderingComponent>()) {
            _currentSelection->getComponent<RenderingComponent>()
                ->drawDebugAxis();
        }
    }
#endif
    if (GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)) {
        // Draw bounding boxes, skeletons, axis gizmo, etc.
        GFX_DEVICE.debugDraw(renderState());
        // Show NavMeshes
        AI::AIManager::getInstance().debugDraw(false);
    }
}
};