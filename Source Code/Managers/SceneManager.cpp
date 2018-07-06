#include "Headers/SceneManager.h"
#include "Headers/AIManager.h"

#include "SceneList.h"
#include "Core/Headers/ParamHandler.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

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
    _renderPassCuller->refresh();
    return Attorney::SceneManager::frameEnded(*_activeScene);
}

void SceneManager::preRender() {
    _activeScene->preRender();
}

void SceneManager::sortVisibleNodes(
    RenderPassCuller::VisibleNodeCache& nodes) const {
    if (nodes._sorted) {
        return;
    }

    RenderPassCuller::VisibleNodeList& visibleNodes = nodes._visibleNodes;
    std::sort(std::begin(visibleNodes), std::end(visibleNodes),
              [](const RenderPassCuller::RenderableNode& nodeA,
                 const RenderPassCuller::RenderableNode& nodeB) {
                  RenderingComponent* renderableA =
                      nodeA._visibleNode->getComponent<RenderingComponent>();
                  RenderingComponent* renderableB =
                      nodeB._visibleNode->getComponent<RenderingComponent>();
                  return renderableA->drawOrder() < renderableB->drawOrder();
              });

    nodes._sorted = true;
}

void SceneManager::updateVisibleNodes(bool flushCache) {
    auto cullingFunction = [](const SceneGraphNode& node) -> bool {
        if (node.getNode()->getType() == SceneNodeType::TYPE_OBJECT3D) {
            Object3D::ObjectType type =
                node.getNode<Object3D>()->getObjectType();
            return (type == Object3D::ObjectType::FLYWEIGHT);
        }
        return false;
    };

    auto meshCullingFunction = [](const RenderPassCuller::RenderableNode& node) -> bool {
        const SceneGraphNode* sgnNode = node._visibleNode;
        if (sgnNode->getNode()->getType() == SceneNodeType::TYPE_OBJECT3D) {
            Object3D::ObjectType type =
                sgnNode->getNode<Object3D>()->getObjectType();
            return (type == Object3D::ObjectType::MESH);
        }
        return false;
    };

    if (flushCache) {
        _renderPassCuller->refresh();
    }

    RenderQueue& queue = RenderQueue::getInstance();
    RenderPassCuller::VisibleNodeCache& nodes =
        _renderPassCuller->frustumCull(
            *GET_ACTIVE_SCENEGRAPH().getRoot(), _activeScene->state(),
            cullingFunction);

    RenderStage stage = GFX_DEVICE.getRenderStage();
    bool refreshNodeData = !nodes._locked;
    if (refreshNodeData) {
        queue.refresh();
        vec3<F32> eyePos(_activeScene->renderState().getCameraConst().getEye());
        for (RenderPassCuller::RenderableNode& node : nodes._visibleNodes) {
            queue.addNodeToQueue(*node._visibleNode, eyePos);
        }
        queue.sort(stage);
        sortVisibleNodes(nodes);

        // Generate and upload all lighting data
        if (stage != RenderStage::SHADOW &&
            stage != RenderStage::DISPLAY) {
            LightManager::getInstance().updateAndUploadLightData(GFX_DEVICE.getMatrix(MATRIX_MODE::VIEW));
        }
    }

    _renderPassCuller->cullSpecial(nodes, meshCullingFunction);

    GFX_DEVICE.buildDrawCommands(nodes._visibleNodes,
                                 _activeScene->renderState(),
                                refreshNodeData);
}

void SceneManager::renderVisibleNodes(bool flushCache) {
    updateVisibleNodes(flushCache);
    renderScene();
}

void SceneManager::renderScene() {
    _renderPassManager->render(
        _activeScene->renderState(),
        _activeScene->getSceneGraph());
}

void SceneManager::render(RenderStage stage, const Kernel& kernel) {
    assert(_activeScene != nullptr);

    _activeScene->renderState().getCamera().renderLookAt();

    Attorney::KernelScene::submitRenderCall(
        kernel, stage, _activeScene->renderState(),
        [&]() {
            renderVisibleNodes(stage != RenderStage::DISPLAY);
        });

    Attorney::SceneManager::debugDraw(*_activeScene, stage);
}

void SceneManager::postRender() {
    _activeScene->postRender();
}

void SceneManager::onCameraUpdate(Camera& camera) {
    Attorney::SceneManager::onCameraUpdate(*_activeScene, camera);
}

///--------------------------Input
/// Management-------------------------------------///

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