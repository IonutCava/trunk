#include "stdafx.h"

#include "Headers/Gizmo.h"
#include "Editor/Headers/Editor.h"
#include "Core/Headers/PlatformContext.h"
#include "Managers/Headers/SceneManager.h"

#include <imgui_internal.h>

namespace Divide {
    Gizmo::Gizmo(Editor& parent, ImGuiContext* mainContext, DisplayWindow* mainWindow)
        : _parent(parent),
          _visible(false),
          _enabled(false),
          _wasUsed(false),
          _shouldRegisterUndo(false)
    {
        _imguiContext = ImGui::CreateContext(mainContext->IO.Fonts);
        _undoEntry._name = "Gizmo Manipulation";
        InitBasicImGUIState(_imguiContext->IO);
        ImGui::SetCurrentContext(_imguiContext);
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        main_viewport->PlatformHandle = mainWindow;
    }

    Gizmo::~Gizmo()
    {
        ImGui::SetCurrentContext(_imguiContext);
        ImGui::DestroyPlatformWindows();
        ImGui::DestroyContext(_imguiContext);
        _imguiContext= nullptr;
    }

    ImGuiContext& Gizmo::getContext() {
        assert(_imguiContext != nullptr);
        return *_imguiContext;
    }

    const ImGuiContext& Gizmo::getContext() const {
        assert(_imguiContext != nullptr);
        return *_imguiContext;
    }

    void Gizmo::enable(bool state) {
        _enabled = state;
    }

    bool Gizmo::enabled() const {
        return _enabled;
    }

    bool Gizmo::active() const {
        return enabled() && !_selectedNodes.empty();
    }

    void Gizmo::update(const U64 deltaTimeUS) {
        ImGui::SetCurrentContext(_imguiContext);

        ImGuiIO& io = _imguiContext->IO;
        io.DeltaTime = Time::MicrosecondsToSeconds<F32>(deltaTimeUS);
        if (io.WantSetMousePos) {
            WindowManager& winMgr = _parent.context().app().windowManager();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                winMgr.setGlobalCursorPosition((I32)io.MousePos.x, (I32)io.MousePos.y);
            } else {
                winMgr.setCursorPosition((I32)io.MousePos.x, (I32)io.MousePos.y);
            }
        }

        if (_shouldRegisterUndo) {
            _parent.registerUndoEntry(_undoEntry);
            _shouldRegisterUndo = false;
        }
    }

    void Gizmo::render(const Camera& camera) {
        if (!active()) {
            return;
        }

        SceneGraphNode* sgn = _selectedNodes.front();
        if (sgn == nullptr) {
            return;
        }

        TransformComponent* const transform = sgn->get<TransformComponent>();
        if (transform == nullptr) {
            return;
        }

        const mat4<F32>& cameraView = camera.getViewMatrix();
        const mat4<F32>& cameraProjection = camera.getProjectionMatrix();

        ImGui::SetCurrentContext(_imguiContext);
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        DisplayWindow* mainWindow = static_cast<DisplayWindow*>(main_viewport->PlatformHandle);
        vec2<U16> size = mainWindow->getDrawableSize();
        ImGuizmo::SetRect(0.0f, 0.0f, size.width, size.height);

        mat4<F32> matrix(transform->getLocalMatrix());
        ImGuizmo::Manipulate(cameraView,
                             cameraProjection,
                             _transformSettings.currentGizmoOperation,
                             _transformSettings.currentGizmoMode,
                             matrix,
                             NULL,
                             _transformSettings.useSnap ? &_transformSettings.snap[0] : NULL);

        if (ImGuizmo::IsUsing()) {
            if (!_wasUsed) {
                transform->getValues(_undoEntry.oldVal);
            }
            _wasUsed = true;
            //ToDo: This seems slow as hell, but it works. Should I bother? -Ionut
            TransformValues values;  vec3<F32> euler;
            ImGuizmo::DecomposeMatrixToComponents(matrix, values._translation, euler._v, values._scale);
            matrix.orthoNormalize();
            values._orientation.fromMatrix(mat3<F32>(matrix));
            transform->setTransform(values);

            _undoEntry.newVal = values;
            _undoEntry._dataSetter = [transform](const void* data) {
                if (transform != nullptr) {
                    transform->setTransform(*(TransformValues*)data);
                }
            };
        }

        ImGui::Render();
        Attorney::EditorGizmo::renderDrawList(_parent, ImGui::GetDrawData(), true, -1);
    }

    void Gizmo::updateSelections(SceneGraphNode* node) {
        if (node == nullptr) {
            _selectedNodes.resize(0);
        } else {
            if (node->get<TransformComponent>() != nullptr) {
                _selectedNodes.push_back(node);
            }
        }
    }

    void Gizmo::setTransformSettings(const TransformSettings& settings) {
        _transformSettings = settings;
    }

    const TransformSettings& Gizmo::getTransformSettings() const {
        return _transformSettings;
    }

    /// Key pressed: return true if input was consumed
    bool Gizmo::onKeyDown(const Input::KeyEvent& key) {
        if (!active()) {
            return false;
        }

        ImGuiIO& io = _imguiContext->IO;

        io.KeysDown[to_I32(key._key)] = true;
        if (key._text != nullptr) {
            io.AddInputCharactersUTF8(key._text);
        }

        if (key._key == Input::KeyCode::KC_LCONTROL || key._key == Input::KeyCode::KC_RCONTROL) {
            io.KeyCtrl = true;
        }
        if (key._key == Input::KeyCode::KC_LSHIFT || key._key == Input::KeyCode::KC_RSHIFT) {
            io.KeyShift = true;
        }
        if (key._key == Input::KeyCode::KC_LMENU || key._key == Input::KeyCode::KC_RMENU) {
            io.KeyAlt = true;
        }
        if (key._key == Input::KeyCode::KC_LWIN || key._key == Input::KeyCode::KC_RWIN) {
            io.KeySuper = true;
        }

        return io.WantCaptureKeyboard;
    }

    /// Key released: return true if input was consumed
    bool Gizmo::onKeyUp(const Input::KeyEvent& key) {
        if (_wasUsed) {
            _shouldRegisterUndo = true;
            _wasUsed = false;
        }

        if (!active()) {
            return false;
        }

        ImGuiIO& io = _imguiContext->IO;


        if (io.KeyCtrl) {
            if (key._key == Input::KeyCode::KC_Z) {
                _parent.Undo();
            }
            else if (key._key == Input::KeyCode::KC_R) {
                _parent.Redo();
            }
        } else {
            if (_parent.scenePreviewFocused()) {
                TransformSettings settings = _parent.getTransformSettings();
                if (key._key == Input::KeyCode::KC_T) {
                    settings.currentGizmoOperation = ImGuizmo::TRANSLATE;
                } else if (key._key == Input::KeyCode::KC_R) {
                    settings.currentGizmoOperation = ImGuizmo::ROTATE;
                } else if (key._key == Input::KeyCode::KC_S) {
                    settings.currentGizmoOperation = ImGuizmo::SCALE;
                }
                _parent.setTransformSettings(settings);
            }
        }

        io.KeysDown[to_I32(key._key)] = false;

        if (key._key == Input::KeyCode::KC_LCONTROL || key._key == Input::KeyCode::KC_RCONTROL) {
            io.KeyCtrl = false;
        }

        if (key._key == Input::KeyCode::KC_LSHIFT || key._key == Input::KeyCode::KC_RSHIFT) {
            io.KeyShift = false;
        }

        if (key._key == Input::KeyCode::KC_LMENU || key._key == Input::KeyCode::KC_RMENU) {
            io.KeyAlt = false;
        }

        if (key._key == Input::KeyCode::KC_LWIN || key._key == Input::KeyCode::KC_RWIN) {
            io.KeySuper = false;
        }

        return io.WantCaptureKeyboard;
    }

    /// Mouse moved: return true if input was consumed
    bool Gizmo::mouseMoved(const Input::MouseMoveEvent& arg) {
        if (!active()) {
            return false;
        }

        ImGuiIO& io = _imguiContext->IO;

        if (!arg.wheelEvent()) {
            ImGuiContext& parentContext = Attorney::EditorGeneralWidget::imguiContext(_parent);
            WindowManager& winMgr = _parent.context().app().windowManager();
            if (ImGuiViewport* viewport = Attorney::EditorGizmo::findViewportByPlatformHandle(_parent, &parentContext, winMgr.getFocusedWindow())) {
                io.MousePos = ImVec2(viewport->Pos.x + (F32)arg.X().abs, viewport->Pos.y + (F32)arg.Y().abs);
                if (_parent.scenePreviewFocused()) {
                    const Rect<I32>& sceneRect = _parent.scenePreviewRect(true);
                    vec2<I32> mousePos(io.MousePos.x, io.MousePos.y);
                    if (sceneRect.contains(mousePos)) {
                        mousePos = COORD_REMAP(mousePos, sceneRect, Rect<I32>(0, 0, to_I32(viewport->Size.x), to_I32(viewport->Size.y)));
                        io.MousePos = ImVec2(to_F32(mousePos.x), to_F32(mousePos.y));
                    }
                }
            }
        } else {
            if (arg.WheelH() > 0) {
                io.MouseWheelH += 1;
            }
            if (arg.WheelH() < 0) {
                io.MouseWheelH -= 1;
            }
            if (arg.WheelV() > 0) {
                io.MouseWheel += 1;
            }
            if (arg.WheelV() < 0) {
                io.MouseWheel -= 1;
            }
        }

        return io.WantCaptureMouse || isUsing();
    }

    /// Mouse button pressed: return true if input was consumed
    bool Gizmo::mouseButtonPressed(const Input::MouseButtonEvent& arg) {
        ACKNOWLEDGE_UNUSED(arg);
        _wasUsed = false;

        if (!active()) {
            return false;
        }

        ImGuiIO& io = _imguiContext->IO;
        if (to_base(arg.button) < 5) {
            for (U8 i = 0; i < 5; ++i) {
                if (arg.button == Editor::g_oisButtons[i]) {
                    io.MouseDown[i] = true;
                    break;
                }
            }
        }

        return io.WantCaptureMouse || isOver();
    }

    /// Mouse button released: return true if input was consumed
    bool Gizmo::mouseButtonReleased(const Input::MouseButtonEvent& arg) {
        if (!active()) {
            return false;
        }

        ACKNOWLEDGE_UNUSED(arg);

        ImGuiIO& io = _imguiContext->IO;
        for (U8 i = 0; i < 5; ++i) {
            if (arg.button == Editor::g_oisButtons[i]) {
                io.MouseDown[i] = false;
                break;
            }
        }

        return io.WantCaptureMouse  || isOver();
    }

    bool Gizmo::joystickButtonPressed(const Input::JoystickEvent &arg) {
        ACKNOWLEDGE_UNUSED(arg);

        return false;
    }

    bool Gizmo::joystickButtonReleased(const Input::JoystickEvent &arg) {
        ACKNOWLEDGE_UNUSED(arg);

        return false;
    }

    bool Gizmo::joystickAxisMoved(const Input::JoystickEvent &arg) {
        ACKNOWLEDGE_UNUSED(arg);

        return false;
    }

    bool Gizmo::joystickPovMoved(const Input::JoystickEvent &arg) {
        ACKNOWLEDGE_UNUSED(arg);

        return false;
    }

    bool Gizmo::joystickBallMoved(const Input::JoystickEvent &arg) {
        ACKNOWLEDGE_UNUSED(arg);

        return false;
    }

    bool Gizmo::joystickAddRemove(const Input::JoystickEvent &arg) {
        ACKNOWLEDGE_UNUSED(arg);

        return false;
    }

    bool Gizmo::joystickRemap(const Input::JoystickEvent &arg) {
        ACKNOWLEDGE_UNUSED(arg);

        return false;
    }

    bool Gizmo::onUTF8(const Input::UTF8Event& arg) {
        ACKNOWLEDGE_UNUSED(arg);

        return false;
    }

    bool Gizmo::isUsing() const {
        ImGuiContext* crtContext = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(_imguiContext);
        bool imguizmoState = ImGuizmo::IsUsing();
        ImGui::SetCurrentContext(crtContext);
        return imguizmoState;
    }

    bool Gizmo::isOver() const {
        ImGuiContext* crtContext = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(_imguiContext);
        bool imguizmoState = ImGuizmo::IsOver();
        ImGui::SetCurrentContext(crtContext);
        return imguizmoState;
    }

    void Gizmo::onSizeChange(const SizeChangeParams& params, const vec2<U16>& displaySize) {
        ImGuiIO& io = _imguiContext->IO;
        io.DisplaySize = ImVec2((F32)params.width, (F32)params.height);
        io.DisplayFramebufferScale = ImVec2(params.width > 0 ? ((F32)displaySize.width / params.width) : 0.f,
                                            params.height > 0 ? ((F32)displaySize.height / params.height) : 0.f);
    }
}; //namespace Divide