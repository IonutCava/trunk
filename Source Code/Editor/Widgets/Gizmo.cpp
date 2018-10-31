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
        io.KeyMap[ImGuiKey_Tab] = to_I32(Input::KeyCode::KC_TAB);
        io.KeyMap[ImGuiKey_LeftArrow] = to_I32(Input::KeyCode::KC_LEFT);
        io.KeyMap[ImGuiKey_RightArrow] = to_I32(Input::KeyCode::KC_RIGHT);
        io.KeyMap[ImGuiKey_UpArrow] = to_I32(Input::KeyCode::KC_UP);
        io.KeyMap[ImGuiKey_DownArrow] = to_I32(Input::KeyCode::KC_DOWN);
        io.KeyMap[ImGuiKey_PageUp] = to_I32(Input::KeyCode::KC_PGUP);
        io.KeyMap[ImGuiKey_PageDown] = to_I32(Input::KeyCode::KC_PGDOWN);
        io.KeyMap[ImGuiKey_Home] = to_I32(Input::KeyCode::KC_HOME);
        io.KeyMap[ImGuiKey_End] = to_I32(Input::KeyCode::KC_END);
        io.KeyMap[ImGuiKey_Delete] = to_I32(Input::KeyCode::KC_DELETE);
        io.KeyMap[ImGuiKey_Backspace] = to_I32(Input::KeyCode::KC_BACK);
        io.KeyMap[ImGuiKey_Enter] = to_I32(Input::KeyCode::KC_RETURN);
        io.KeyMap[ImGuiKey_NumericEnter] = to_I32(Input::KeyCode::KC_NUMPADENTER);
        io.KeyMap[ImGuiKey_Escape] = to_I32(Input::KeyCode::KC_ESCAPE);
        io.KeyMap[ImGuiKey_Space] = to_I32(Input::KeyCode::KC_SPACE);
        io.KeyMap[ImGuiKey_A] = to_I32(Input::KeyCode::KC_A);
        io.KeyMap[ImGuiKey_C] = to_I32(Input::KeyCode::KC_C);
        io.KeyMap[ImGuiKey_V] = to_I32(Input::KeyCode::KC_V);
        io.KeyMap[ImGuiKey_X] = to_I32(Input::KeyCode::KC_X);
        io.KeyMap[ImGuiKey_Y] = to_I32(Input::KeyCode::KC_Y);
        io.KeyMap[ImGuiKey_Z] = to_I32(Input::KeyCode::KC_Z);

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

    bool Gizmo::active() const {
        return enabled() && !_selectedNodes.empty();
    }

    void Gizmo::update(const U64 deltaTimeUS) {
        ImGuiIO& io = _imguiContext->IO;
        io.DeltaTime = Time::MicrosecondsToSeconds<F32>(deltaTimeUS);
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
        if (!active()) {
            return false;
        }

        ImGuiIO& io = _imguiContext->IO;

        io.KeysDown[to_I32(key._key)] = true;
        if (key._text != nullptr) {
            io.AddInputCharactersUTF8(key._text);
        }
        io.KeyCtrl = key._key == Input::KeyCode::KC_LCONTROL || key._key == Input::KeyCode::KC_RCONTROL;
        io.KeyShift = key._key == Input::KeyCode::KC_LSHIFT || key._key == Input::KeyCode::KC_RSHIFT;
        io.KeyAlt = key._key == Input::KeyCode::KC_LMENU || key._key == Input::KeyCode::KC_RMENU;
        io.KeySuper = key._key == Input::KeyCode::KC_LWIN || key._key == Input::KeyCode::KC_RWIN;

        return io.WantCaptureKeyboard;
    }

    /// Key released: return true if input was consumed
    bool Gizmo::onKeyUp(const Input::KeyEvent& key) {
        if (!active()) {
            return false;
        }

        ImGuiIO& io = _imguiContext->IO;
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

        io.MousePos.x = (F32)arg.X(true).abs;
        io.MousePos.y = (F32)arg.Y(true).abs;
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
        return io.WantCaptureMouse || isUsing();
    }

    /// Mouse button pressed: return true if input was consumed
    bool Gizmo::mouseButtonPressed(const Input::MouseButtonEvent& arg) {
        ACKNOWLEDGE_UNUSED(arg);

        if (!active()) {
            return false;
        }

        ImGuiIO& io = _imguiContext->IO;
        if (to_base(arg.button) < 5) {
            io.MouseDown[to_base(arg.button)] = true;
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
        if (to_base(arg.button) < 5) {
            io.MouseDown[to_base(arg.button)] = false;
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