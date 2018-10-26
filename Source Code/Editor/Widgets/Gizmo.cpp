#include "stdafx.h"

#include "Headers/Gizmo.h"
#include "Editor/Headers/Editor.h"

#include "Managers/Headers/SceneManager.h"

#include <imgui_internal.h>

namespace Divide {
    Gizmo::Gizmo(Editor& parent, ImGuiContext* mainContext, DisplayWindow* mainWindow)
        : _parent(parent),
          _visible(false),
          _enabled(false)
    {
        _imguiContext = ImGui::CreateContext(mainContext->IO.Fonts);

        ImGuiIO& io = _imguiContext->IO;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.KeyMap[ImGuiKey_Tab] = Input::KeyCode::KC_TAB;
        io.KeyMap[ImGuiKey_LeftArrow] = Input::KeyCode::KC_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = Input::KeyCode::KC_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = Input::KeyCode::KC_UP;
        io.KeyMap[ImGuiKey_DownArrow] = Input::KeyCode::KC_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = Input::KeyCode::KC_PGUP;
        io.KeyMap[ImGuiKey_PageDown] = Input::KeyCode::KC_PGDOWN;
        io.KeyMap[ImGuiKey_Home] = Input::KeyCode::KC_HOME;
        io.KeyMap[ImGuiKey_End] = Input::KeyCode::KC_END;
        io.KeyMap[ImGuiKey_Delete] = Input::KeyCode::KC_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = Input::KeyCode::KC_BACK;
        io.KeyMap[ImGuiKey_Enter] = Input::KeyCode::KC_RETURN;
        io.KeyMap[ImGuiKey_Escape] = Input::KeyCode::KC_ESCAPE;
        io.KeyMap[ImGuiKey_Space] = Input::KeyCode::KC_SPACE;
        io.KeyMap[ImGuiKey_A] = Input::KeyCode::KC_A;
        io.KeyMap[ImGuiKey_C] = Input::KeyCode::KC_C;
        io.KeyMap[ImGuiKey_V] = Input::KeyCode::KC_V;
        io.KeyMap[ImGuiKey_X] = Input::KeyCode::KC_X;
        io.KeyMap[ImGuiKey_Y] = Input::KeyCode::KC_Y;
        io.KeyMap[ImGuiKey_Z] = Input::KeyCode::KC_Z;

        io.SetClipboardTextFn = SetClipboardText;
        io.GetClipboardTextFn = GetClipboardText;
        io.ClipboardUserData = nullptr;
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

    void Gizmo::update(const U64 deltaTimeUS) {
        ImGuiIO& io = _imguiContext->IO;
        io.DeltaTime = Time::MicrosecondsToSeconds<F32>(deltaTimeUS);
    }

    void Gizmo::render(const Camera& camera) {
        if (!_enabled || _selectedNodes.empty()) {
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

        mat4<F32> matrix(transform->getLocalMatrix());

        ImGui::SetCurrentContext(_imguiContext);
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        const ImGuiIO& io = _imguiContext->IO;
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        ImGuizmo::Manipulate(cameraView,
                             cameraProjection,
                             _transformSettings.currentGizmoOperation,
                             _transformSettings.currentGizmoMode,
                             matrix,
                             NULL,
                             _transformSettings.useSnap ? &_transformSettings.snap[0] : NULL);

        //ToDo: This seems slow as hell, but it works. Should I bother? -Ionut
        TransformValues values;  vec3<F32> euler;
        ImGuizmo::DecomposeMatrixToComponents(matrix, values._translation, euler._v, values._scale);
        matrix.orthoNormalize();
        values._orientation.fromMatrix(mat3<F32>(matrix));
        transform->setTransform(values);

        ImGui::Render();
        Attorney::EditorGizmo::renderDrawList(_parent, ImGui::GetDrawData(), true, -1);
    }


    void Gizmo::updateSelections(SceneGraphNode* node) {
        if (node == nullptr) {
            _selectedNodes.resize(0);
        } else {
            _selectedNodes.push_back(node);
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
        ImGuiIO& io = _imguiContext->IO;

        io.KeysDown[key._key] = true;
        if (key._text > 0) {
            io.AddInputCharacter(to_U16(key._text));
        }
        io.KeyCtrl = key._key == Input::KeyCode::KC_LCONTROL || key._key == Input::KeyCode::KC_RCONTROL;
        io.KeyShift = key._key == Input::KeyCode::KC_LSHIFT || key._key == Input::KeyCode::KC_RSHIFT;
        io.KeyAlt = key._key == Input::KeyCode::KC_LMENU || key._key == Input::KeyCode::KC_RMENU;
        io.KeySuper = false;

        return io.WantCaptureKeyboard;
    }

    /// Key released: return true if input was consumed
    bool Gizmo::onKeyUp(const Input::KeyEvent& key) {
        ImGuiIO& io = _imguiContext->IO;
        io.KeysDown[key._key] = false;

        if (key._key == Input::KeyCode::KC_LCONTROL || key._key == Input::KeyCode::KC_RCONTROL) {
            io.KeyCtrl = false;
        }

        if (key._key == Input::KeyCode::KC_LSHIFT || key._key == Input::KeyCode::KC_RSHIFT) {
            io.KeyShift = false;
        }

        if (key._key == Input::KeyCode::KC_LMENU || key._key == Input::KeyCode::KC_RMENU) {
            io.KeyAlt = false;
        }

        io.KeySuper = false;

        return io.WantCaptureKeyboard;
    }

    /// Mouse moved: return true if input was consumed
    bool Gizmo::mouseMoved(const Input::MouseEvent& arg) {
        ImGuiIO& io = _imguiContext->IO;

        io.MousePos.x = (F32)arg.X(true).abs;
        io.MousePos.y = (F32)arg.Y(true).abs;
        io.MouseWheel += (F32)arg.Z(true).rel / 60.0f;

        return io.WantCaptureMouse || isUsing();
    }

    /// Mouse button pressed: return true if input was consumed
    bool Gizmo::mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button) {
        ACKNOWLEDGE_UNUSED(arg);

        ImGuiIO& io = _imguiContext->IO;
        if (button < 5) {
            io.MouseDown[button] = true;
        }

        return io.WantCaptureMouse || isOver();
    }

    /// Mouse button released: return true if input was consumed
    bool Gizmo::mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button) {
        ACKNOWLEDGE_UNUSED(arg);

        ImGuiIO& io = _imguiContext->IO;
        if (button < 5) {
            io.MouseDown[button] = false;
        }

        return io.WantCaptureMouse  || isOver();
    }

    bool Gizmo::joystickButtonPressed(const Input::JoystickEvent &arg, Input::JoystickButton button) {
        ACKNOWLEDGE_UNUSED(arg);
        ACKNOWLEDGE_UNUSED(button);

        return false;
    }

    bool Gizmo::joystickButtonReleased(const Input::JoystickEvent &arg, Input::JoystickButton button) {
        ACKNOWLEDGE_UNUSED(arg);
        ACKNOWLEDGE_UNUSED(button);

        return false;
    }

    bool Gizmo::joystickAxisMoved(const Input::JoystickEvent &arg, I8 axis) {
        ACKNOWLEDGE_UNUSED(arg);
        ACKNOWLEDGE_UNUSED(axis);

        return false;
    }

    bool Gizmo::joystickPovMoved(const Input::JoystickEvent &arg, I8 pov) {
        ACKNOWLEDGE_UNUSED(arg);
        ACKNOWLEDGE_UNUSED(pov);

        return false;
    }

    bool Gizmo::joystickSliderMoved(const Input::JoystickEvent &arg, I8 index) {
        ACKNOWLEDGE_UNUSED(arg);
        ACKNOWLEDGE_UNUSED(index);

        return false;
    }

    bool Gizmo::joystickvector3Moved(const Input::JoystickEvent &arg, I8 index) {
        ACKNOWLEDGE_UNUSED(arg);
        ACKNOWLEDGE_UNUSED(index);

        return false;
    }

    bool Gizmo::onSDLInputEvent(SDL_Event event) {
        ACKNOWLEDGE_UNUSED(event);

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