#include "Headers/SceneManager.h"
#include "Headers/AIManager.h"

#include "SceneList.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/ProfileTimer.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

namespace Divide {

SceneManager::SceneManager()
    : FrameListener(),
      _GUI(nullptr),
      _activeScene(nullptr),
      _renderPassCuller(nullptr),
      _renderPassManager(nullptr),
      _defaultMaterial(nullptr),
      _processInput(false),
      _init(false)

{
    AI::AIManager::createInstance();
}

SceneManager::~SceneManager()
{
    AI::AIManager::destroyInstance();
    Time::REMOVE_TIMER(_sceneGraphCullTimer);
    UNREGISTER_FRAME_LISTENER(&(this->getInstance()));
    Console::printfn(Locale::get("STOP_SCENE_MANAGER"));
    // Console::printfn(Locale::get("SCENE_MANAGER_DELETE"));
    Console::printfn(Locale::get("SCENE_MANAGER_REMOVE_SCENES"));
    MemoryManager::DELETE_HASHMAP(_sceneMap);
    MemoryManager::DELETE(_renderPassCuller);
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
    return Attorney::SceneManager::frameStarted(*_activeScene);
}

bool SceneManager::frameEnded(const FrameEvent& evt) {
    return Attorney::SceneManager::frameEnded(*_activeScene);
}

void SceneManager::preRender() {
    _activeScene->preRender();
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

    _renderPassCuller->frustumCull(GET_ACTIVE_SCENEGRAPH(), _activeScene->state(), stage, cullingFunction);
    RenderPassCuller::VisibleNodeList& visibleNodes = _renderPassCuller->getNodeCache(stage);

    visibleNodes.erase(std::remove_if(std::begin(visibleNodes),
                                   std::end(visibleNodes),
                                   meshCullingFunction),
                       std::end(visibleNodes));

    return visibleNodes;
}

void SceneManager::updateVisibleNodes(RenderStage stage, bool refreshNodeData) {
    RenderQueue& queue = _renderPassManager->getQueue();

    RenderPassCuller::VisibleNodeList& visibleNodes = _renderPassCuller->getNodeCache(stage);

    if (refreshNodeData) {
        queue.refresh();
        vec3<F32> eyePos(_activeScene->renderState().getCameraConst().getEye());
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

    GFX_DEVICE.buildDrawCommands(visibleNodes, _activeScene->renderState(), refreshNodeData);
}

void SceneManager::renderScene() {
    _renderPassManager->render(_activeScene->renderState());
}

void SceneManager::renderVisibleNodes(RenderStage stage, bool frustumCull, bool refreshNodeData) {
    if (frustumCull) {
        Time::ScopedTimer timer(*_sceneGraphCullTimer);
        cullSceneGraph(stage);
    }
    updateVisibleNodes(stage, refreshNodeData);

    switch(stage) {
        case RenderStage::DISPLAY:
            GFX_DEVICE.occlusionCull();
            break;
        case RenderStage::Z_PRE_PASS:
            LightManager::getInstance().updateAndUploadLightData(GFX_DEVICE.getMatrix(MATRIX_MODE::VIEW));
            break;
    };

    renderScene();
}

void SceneManager::render(RenderStage stage, const Kernel& kernel, bool frustumCull, bool refreshNodeData) {
    assert(_activeScene != nullptr);

    _activeScene->renderState().getCamera().renderLookAt();

    Attorney::KernelScene::submitRenderCall(
        kernel, stage, _activeScene->renderState(),
        [&, stage, frustumCull, refreshNodeData]() {
            renderVisibleNodes(stage, frustumCull, refreshNodeData);
        });

    Attorney::SceneManager::debugDraw(*_activeScene, stage);
}

void SceneManager::postRender() {
    _activeScene->postRender();
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