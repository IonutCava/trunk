#include "Headers/Scene.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

void Scene::onLostFocus() {
    state().resetMovement();
#ifndef _DEBUG
    _paramHandler.setParam("freezeLoopTime", true);
#endif
}

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
static vectorImpl<SceneGraphNode*> _sceneSelectionCandidates;
}

void Scene::findSelection(F32 mouseX, F32 mouseY) {
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

bool Scene::mouseButtonPressed(const Input::MouseEvent& key,
                               Input::MouseButton button) {
    _mousePressed[button] = true;
    switch (button) {
        default:
            return false;

        case Input::MouseButton::MB_Left:
            break;
        case Input::MouseButton::MB_Right:
            break;
        case Input::MouseButton::MB_Middle:
            break;
        case Input::MouseButton::MB_Button3:
            break;
        case Input::MouseButton::MB_Button4:
            break;
        case Input::MouseButton::MB_Button5:
            break;
        case Input::MouseButton::MB_Button6:
            break;
        case Input::MouseButton::MB_Button7:
            break;
    }
    return true;
}

bool Scene::mouseButtonReleased(const Input::MouseEvent& key,
                                Input::MouseButton button) {
    _mousePressed[button] = false;
    switch (button) {
        default:
            return false;

        case Input::MouseButton::MB_Left: {
            findSelection(key.state.X.abs, key.state.Y.abs);
        } break;
        case Input::MouseButton::MB_Right: {
            state().angleLR(0);
            state().angleUD(0);
        } break;
        case Input::MouseButton::MB_Middle:
            break;
        case Input::MouseButton::MB_Button3:
            break;
        case Input::MouseButton::MB_Button4:
            break;
        case Input::MouseButton::MB_Button5:
            break;
        case Input::MouseButton::MB_Button6:
            break;
        case Input::MouseButton::MB_Button7:
            break;
    }
    return true;
}

bool Scene::mouseMoved(const Input::MouseEvent& key) {
    state().mouseXDelta(_previousMousePos.x - key.state.X.abs);
    state().mouseYDelta(_previousMousePos.y - key.state.Y.abs);
    _previousMousePos.set(key.state.X.abs, key.state.Y.abs);
    if (_mousePressed[Input::MouseButton::MB_Right]) {
        state().angleLR(-state().mouseXDelta());
        state().angleUD(-state().mouseYDelta());
    }
    return true;
}

using namespace Input;
bool Scene::onKeyDown(const Input::KeyEvent& key) {
    switch (key._key) {
        default:
            return false;
        case KeyCode::KC_END: {
            deleteSelection();
        } break;
        case KeyCode::KC_ADD: {
            Camera& cam = renderState().getCamera();
            F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
            if (currentCamMoveSpeedFactor < 50) {
                cam.setMoveSpeedFactor(currentCamMoveSpeedFactor + 1.0f);
                cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() + 1.0f);
            }
        } break;
        case KeyCode::KC_SUBTRACT: {
            Camera& cam = renderState().getCamera();
            F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
            if (currentCamMoveSpeedFactor > 1.0f) {
                cam.setMoveSpeedFactor(currentCamMoveSpeedFactor - 1.0f);
                cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() - 1.0f);
            }
        } break;
        case KeyCode::KC_W: {
            state().moveFB(1);
        } break;
        case KeyCode::KC_S: {
            state().moveFB(-1);
        } break;
        case KeyCode::KC_A: {
            state().moveLR(-1);
        } break;
        case KeyCode::KC_D: {
            state().moveLR(1);
        } break;
        case KeyCode::KC_Q: {
            state().roll(1);
        } break;
        case KeyCode::KC_E: {
            state().roll(-1);
        } break;
        case KeyCode::KC_RIGHT: {
            state().angleLR(1);
        } break;
        case KeyCode::KC_LEFT: {
            state().angleLR(-1);
        } break;
        case KeyCode::KC_UP: {
            state().angleUD(-1);
        } break;
        case KeyCode::KC_DOWN: {
            state().angleUD(1);
        } break;
    }
    return true;
}

bool Scene::onKeyUp(const Input::KeyEvent& key) {
    switch (key._key) {
        case KeyCode::KC_P: {
            _paramHandler.setParam(
                "freezeLoopTime",
                !_paramHandler.getParam("freezeLoopTime", false));
        } break;
        case KeyCode::KC_F2: {
            renderState().toggleSkeletons();
        } break;
        case KeyCode::KC_F3: {
            _paramHandler.setParam("postProcessing.enableDepthOfField",
                                   !_paramHandler.getParam<bool>(
                                       "postProcessing.enableDepthOfField"));
        } break;
        case KeyCode::KC_F4: {
            _paramHandler.setParam(
                "postProcessing.enableBloom",
                !_paramHandler.getParam<bool>("postProcessing.enableBloom"));
        } break;
        case KeyCode::KC_F5: {
            renderState().toggleAxisLines();
        } break;
        case KeyCode::KC_B: {
            renderState().toggleBoundingBoxes();
        } break;
        case KeyCode::KC_F8: {
            renderState().drawDebugLines(!renderState().drawDebugLines());
        } break;
#ifdef _DEBUG
        case KeyCode::KC_F9: {
            for (U32 i = 0;
                 i < to_uint(DebugLines::COUNT); ++i) {
                _lines[i].clear();
            }
        } break;
#endif
        case KeyCode::KC_F10: {
            ParamHandler& param = ParamHandler::getInstance();
            LightManager::getInstance().togglePreviewShadowMaps();
            param.setParam<bool>(
                "rendering.previewDepthBuffer",
                !param.getParam<bool>("rendering.previewDepthBuffer", false));
        } break;
        case KeyCode::KC_F7: {
            GFX_DEVICE.Screenshot("screenshot_");
        } break;
        case KeyCode::KC_W:
        case KeyCode::KC_S: {
            state().moveFB(0);
        } break;
        case KeyCode::KC_A:
        case KeyCode::KC_D: {
            state().moveLR(0);
        } break;
        case KeyCode::KC_Q:
        case KeyCode::KC_E: {
            state().roll(0);
        } break;
        case KeyCode::KC_RIGHT:
        case KeyCode::KC_LEFT: {
            state().angleLR(0);
        } break;
        case KeyCode::KC_UP:
        case KeyCode::KC_DOWN: {
            state().angleUD(0);
        } break;
        default:
            return false;
    }
    return true;
}

static I32 axisDeadZone = 256;

bool Scene::joystickAxisMoved(const Input::JoystickEvent& key, I8 axis) {
    STUBBED(
        "ToDo: Store input from multiple joysticks in scene state! - Ionut");
    if (key.device->getID() != to_int(Input::Joystick::JOYSTICK_1)) {
        return false;
    }
    I32 axisABS = key.state.mAxes[axis].abs;

    switch (axis) {
        case 0: {
            if (axisABS > axisDeadZone) {
                state().angleUD(1);
            } else if (axisABS < -axisDeadZone) {
                state().angleUD(-1);
            } else {
                state().angleUD(0);
            }
        } break;
        case 1: {
            if (axisABS > axisDeadZone) {
                state().angleLR(1);
            } else if (axisABS < -axisDeadZone) {
                state().angleLR(-1);
            } else {
                state().angleLR(0);
            }
        } break;

        case 2: {
            if (axisABS < -axisDeadZone) {
                state().moveFB(1);
            } else if (axisABS > axisDeadZone) {
                state().moveFB(-1);
            } else {
                state().moveFB(0);
            }
        } break;
        case 3: {
            if (axisABS < -axisDeadZone) {
                state().moveLR(-1);
            } else if (axisABS > axisDeadZone) {
                state().moveLR(1);
            } else {
                state().moveLR(0);
            }
        } break;
    }
    return true;
}

bool Scene::joystickPovMoved(const Input::JoystickEvent& key, I8 pov) {
    if (key.state.mPOV[pov].direction & OIS::Pov::North) {  // Going up
        state().moveFB(1);
    } else if (key.state.mPOV[pov].direction & OIS::Pov::South) {  // Going down
        state().moveFB(-1);
    } else if (key.state.mPOV[pov].direction & OIS::Pov::East) {  // Going right
        state().moveLR(1);
    } else if (key.state.mPOV[pov].direction & OIS::Pov::West) {  // Going left
        state().moveLR(-1);
    } else /*if (key.state.mPOV[pov].direction == OIS::Pov::Centered)*/ {  // stopped/centered
                                                                           // out
        state().moveLR(0);
        state().moveFB(0);
    }
    return true;
}
};