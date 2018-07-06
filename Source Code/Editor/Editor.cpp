#include "stdafx.h"

#include "Headers/Editor.h"
#include "Headers/Sample.h"
#include "Editor/Widgets/Headers/ImWindowManagerDivide.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/FrameListenerManager.h"

#include "Platform/Input/Headers/InputInterface.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Rendering/Camera/Headers/Camera.h"


namespace Divide {

namespace {
    bool show_test_window = true;
    bool show_another_window = false;
};

Editor::Editor(PlatformContext& context)
    : PlatformContextComponent(context),
      _running(false),
      _activeWindowGUID(-1),
      _editorTimer(Time::ADD_TIMER("Main Editor Timer")),
      _editorUpdateTimer(Time::ADD_TIMER("Editor Update Timer")),
      _editorRenderTimer(Time::ADD_TIMER("Editor Render Timer"))
{
    _windowManager = std::make_unique<ImwWindowManagerDivide>(*this);
    _mainWindow = nullptr;
    _editorTimer.addChildTimer(_editorUpdateTimer);
    _editorTimer.addChildTimer(_editorRenderTimer);
    REGISTER_FRAME_LISTENER(this, 99999);
}

Editor::~Editor()
{
    UNREGISTER_FRAME_LISTENER(this);
    close();
}

void Editor::idle() {

}

bool Editor::init() {
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pPixels;
    int iWidth;
    int iHeight;
    io.Fonts->AddFontDefault();
    io.Fonts->GetTexDataAsRGBA32(&pPixels, &iWidth, &iHeight);

    SamplerDescriptor sampler;
    sampler.setFilters(TextureFilter::LINEAR);

    TextureDescriptor descriptor(TextureType::TEXTURE_2D,
                                 GFXImageFormat::RGBA8,
                                 GFXDataFormat::UNSIGNED_BYTE);
    descriptor.setSampler(sampler);

    ResourceDescriptor textureDescriptor("IMGUI_font_texture");
    textureDescriptor.setThreadedLoading(false);
    textureDescriptor.setFlag(true);
    textureDescriptor.setPropertyDescriptor(descriptor);

    ResourceCache& parentCache = _context.gfx().parent().resourceCache();
    _fontTexture = CreateResource<Texture>(parentCache, textureDescriptor);
    assert(_fontTexture);

    Texture::TextureLoadInfo info;

    _fontTexture->loadData(info, (bufferPtr)pPixels, vec2<U16>(iWidth, iHeight));

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)_fontTexture->getHandle();
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();


    ResourceDescriptor shaderDescriptor("IMGUI");
    shaderDescriptor.setThreadedLoading(false);
    _imguiProgram = CreateResource<ShaderProgram>(_context.gfx().parent().resourceCache(), shaderDescriptor);

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
    io.KeyMap[ImGuiKey_A] = Input::KeyCode::KC_A;
    io.KeyMap[ImGuiKey_C] = Input::KeyCode::KC_C;
    io.KeyMap[ImGuiKey_V] = Input::KeyCode::KC_V;
    io.KeyMap[ImGuiKey_X] = Input::KeyCode::KC_X;
    io.KeyMap[ImGuiKey_Y] = Input::KeyCode::KC_Y;
    io.KeyMap[ImGuiKey_Z] = Input::KeyCode::KC_Z;
    io.SetClipboardTextFn = SetClipboardText;
    io.GetClipboardTextFn = GetClipboardText;
    io.ClipboardUserData = nullptr;
    io.RenderDrawListsFn = nullptr;

    _mainWindow = &context().app().windowManager().getWindow(0u);
    _mainWindow->addEventListener(WindowEvent::CLOSE_REQUESTED, [this](const DisplayWindow::WindowEventArgs& args) { ACKNOWLEDGE_UNUSED(args); OnClose();});
    _mainWindow->addEventListener(WindowEvent::GAINED_FOCUS, [this](const DisplayWindow::WindowEventArgs& args) { OnFocus(args._flag);});
    _mainWindow->addEventListener(WindowEvent::RESIZED_INTERNAL, [this](const DisplayWindow::WindowEventArgs& args) { OnSize(args.x, args.y);});
    _mainWindow->addEventListener(WindowEvent::TEXT, [this](const DisplayWindow::WindowEventArgs& args) { OnUTF8(args._text);});
    _activeWindowGUID = _mainWindow->getGUID();

    vec2<I32> size(_mainWindow->getDimensions());

    OnSize(size.width, size.height);

    if (_windowManager->Init()) {
        InitSample();
        return true;
    }

    return false;
}

void Editor::close() {
    ImGui::Shutdown();
    _fontTexture.reset();
    _imguiProgram.reset();
}

void Editor::toggle(const bool state) {
    _running = state;
}

bool Editor::running() const {
    return _running;
}

void Editor::update(const U64 deltaTimeUS) {
    if (!_running) {
        return;
    }

    Time::ScopedTimer timer(_editorUpdateTimer);
    if (_windowManager->Run(false))
    {

        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = Time::MicrosecondsToSeconds<float>(deltaTimeUS);

        ToggleCursor(!io.MouseDrawCursor);
        if (io.MouseDrawCursor)
        {
            _mainWindow->setCursorStyle(CursorStyle::NONE);
        } else if (io.MousePos.x != -1.f && io.MousePos.y != -1.f)
        {
            switch (ImGui::GetCurrentContext()->MouseCursor)
            {
                case ImGuiMouseCursor_Arrow:
                    _mainWindow->setCursorStyle(CursorStyle::ARROW);
                    break;
                case ImGuiMouseCursor_TextInput:         // When hovering over InputText, etc.
                    _mainWindow->setCursorStyle(CursorStyle::TEXT_INPUT);
                    break;
                case ImGuiMouseCursor_Move:              // Unused
                    _mainWindow->setCursorStyle(CursorStyle::HAND);
                    break;
                case ImGuiMouseCursor_ResizeNS:          // Unused
                    _mainWindow->setCursorStyle(CursorStyle::RESIZE_NS);
                    break;
                case ImGuiMouseCursor_ResizeEW:          // When hovering over a column
                    _mainWindow->setCursorStyle(CursorStyle::RESIZE_EW);
                    break;
                case ImGuiMouseCursor_ResizeNESW:        // Unused
                    _mainWindow->setCursorStyle(CursorStyle::RESIZE_NESW);
                    break;
                case ImGuiMouseCursor_ResizeNWSE:        // When hovering over the bottom-right corner of a window
                    _mainWindow->setCursorStyle(CursorStyle::RESIZE_NWSE);
                    break;
            }
        }
    }
}

bool Editor::frameStarted(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    return true;
}

bool Editor::framePreRenderStarted(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    return true;
}

bool Editor::framePreRenderEnded(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    return true;
}

bool Editor::frameRenderingQueued(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    return true;
}

bool Editor::framePostRenderStarted(const FrameEvent& evt) {
    if (!_running) {
        return true;
    }

    ACKNOWLEDGE_UNUSED(evt);
    Time::ScopedTimer timer(_editorRenderTimer);

    // Render ImWindow stuff
    if (_windowManager->Run(true))
    {
        ImGui::NewFrame();
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit4("clear color", _mainWindow->clearColour()._v);
        if (ImGui::Button("Test Window")) show_test_window ^= 1;
        if (ImGui::Button("Another Window")) show_another_window ^= 1;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Time since last events %.3f ms", Time::MicrosecondsToSeconds<float>(evt._timeSinceLastFrameUS));
        if (ImGui::Button("Toggle cursor")) {
            ImGuiIO& io = ImGui::GetIO();
            io.MouseDrawCursor = !io.MouseDrawCursor;
        }
        // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name the window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello from another window!");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow().
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }

        ImGui::Render();
        renderDrawList(ImGui::GetDrawData(), _mainWindow->getGUID());
        return true;
    }

    return false;
}

bool Editor::framePostRenderEnded(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    return true;
}

bool Editor::frameEnded(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);

    return true;
}

// Needs to be rendered immediately. *IM*GUI. IMGUI::NewFrame invalidates this data
void Editor::renderDrawList(ImDrawData* pDrawData, I64 windowGUID)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0) {
        return;
    }

    I64 previousGUID = -1;
    bool switchWindow = windowGUID != _activeWindowGUID;
    if (switchWindow) {
        previousGUID = _activeWindowGUID;
        _activeWindowGUID = windowGUID;
    }

    pDrawData->ScaleClipRects(io.DisplayFramebufferScale);

    GFX::ScopedCommandBuffer sBuffer = GFX::allocateScopedCommandBuffer();
    GFX::CommandBuffer& buffer = sBuffer();

    RenderStateBlock state;
    state.setCullMode(CullMode::CCW);
    state.setZRead(false);
    state.setScissorTest(true);

    PipelineDescriptor pipelineDesc;
    pipelineDesc._stateHash = state.getHash();
    pipelineDesc._shaderProgram = _imguiProgram;

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = std::numeric_limits<U16>::max();
    beginDebugScopeCmd._scopeName = "Render IMGUI";
    GFX::BeginDebugScope(buffer, beginDebugScopeCmd);

    if (switchWindow) {
        GFX::SwitchWindowCommand switchWindowCmd;
        switchWindowCmd.windowGUID = _activeWindowGUID;
        GFX::AddSwitchWindow(buffer, switchWindowCmd);
    }

    GFX::SetBlendCommand blendCmd;
    blendCmd._enabled = true;
    blendCmd._blendProperties = BlendingProperties{
        BlendProperty::SRC_ALPHA,
        BlendProperty::INV_SRC_ALPHA
    };
    GFX::SetBlend(buffer, blendCmd);

    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = &_context.gfx().newPipeline(pipelineDesc);
    GFX::BindPipeline(buffer, pipelineCmd);

    GFX::SetViewportCommand viewportCmd;
    viewportCmd._viewport.set(0, 0, fb_width, fb_height);
    GFX::SetViewPort(buffer, viewportCmd);

    GFX::SetCameraCommand cameraCmd;
    cameraCmd._camera = Camera::utilityCamera(Camera::UtilityCamera::_2D_FLIP_Y);
    GFX::SetCamera(buffer, cameraCmd);

    GFX::DrawIMGUICommand drawIMGUI;
    drawIMGUI._data = pDrawData;
    GFX::AddDrawIMGUICommand(buffer, drawIMGUI);

    if (switchWindow) {
        GFX::SwitchWindowCommand switchWindowCmd;
        switchWindowCmd.windowGUID = previousGUID;
        GFX::AddSwitchWindow(buffer, switchWindowCmd);
    }

    GFX::EndDebugScopeCommand endDebugScope;
    GFX::EndDebugScope(buffer, endDebugScope);

    _context.gfx().flushCommandBuffer(buffer);
}

/// Key pressed: return true if input was consumed
bool Editor::onKeyDown(const Input::KeyEvent& key) {
    if (!_running) {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[key._key] = true;

    io.KeyCtrl = key._key == Input::KeyCode::KC_LCONTROL || key._key == Input::KeyCode::KC_RCONTROL;
    io.KeyShift = key._key == Input::KeyCode::KC_LSHIFT || key._key == Input::KeyCode::KC_RSHIFT;
    io.KeyAlt = key._key == Input::KeyCode::KC_LMENU || key._key == Input::KeyCode::KC_RMENU;
    io.KeySuper = false;

    return ImGui::GetIO().WantCaptureKeyboard || _windowManager->HasWantCaptureKeyboard();
}

/// Key released: return true if input was consumed
bool Editor::onKeyUp(const Input::KeyEvent& key) {
    if (!_running) {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
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

    return ImGui::GetIO().WantCaptureKeyboard || _windowManager->HasWantCaptureKeyboard();
}

/// Mouse moved: return true if input was consumed
bool Editor::mouseMoved(const Input::MouseEvent& arg) {
    if (!_running) {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)arg._event.state.X.abs,
                         (float)arg._event.state.Y.abs);
    io.MouseWheel += (float)arg._event.state.Z.rel / 60.0f;

    return ImGui::GetIO().WantCaptureMouse || _windowManager->HasWantCaptureMouse();
}

/// Mouse button pressed: return true if input was consumed
bool Editor::mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button) {
    if (!_running) {
        return false;
    }

    ACKNOWLEDGE_UNUSED(arg);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[button == OIS::MB_Left ? 0 : button == OIS::MB_Right ? 1 : 2] = true;

    return ImGui::GetIO().WantCaptureMouse || _windowManager->HasWantCaptureMouse();
}

/// Mouse button released: return true if input was consumed
bool Editor::mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button) {
    if (!_running) {
        return false;
    }

    ACKNOWLEDGE_UNUSED(arg);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[button == OIS::MB_Left ? 0 : button == OIS::MB_Right ? 1 : 2] = false;

    return ImGui::GetIO().WantCaptureMouse || _windowManager->HasWantCaptureMouse();
}

bool Editor::joystickButtonPressed(const Input::JoystickEvent &arg, Input::JoystickButton button) {
    if (!_running) {
        return false;
    }

    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(button);
    return false;
}

bool Editor::joystickButtonReleased(const Input::JoystickEvent &arg, Input::JoystickButton button) {
    if (!_running) {
        return false;
    }

    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(button);
    return false;
}

bool Editor::joystickAxisMoved(const Input::JoystickEvent &arg, I8 axis) {
    if (!_running) {
        return false;
    }

    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(axis);
    return false;
}

bool Editor::joystickPovMoved(const Input::JoystickEvent &arg, I8 pov) {
    if (!_running) {
        return false;
    }

    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(pov);
    return false;
}

bool Editor::joystickSliderMoved(const Input::JoystickEvent &arg, I8 index) {
    if (!_running) {
        return false;
    }

    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(index);
    return false;
}

bool Editor::joystickvector3Moved(const Input::JoystickEvent &arg, I8 index) {
    if (!_running) {
        return false;
    }

    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(index);
    return false;
}

bool Editor::OnClose() {
    if (!_running) {
        return false;
    }

    return true;
}

void Editor::OnFocus(bool bHasFocus) {
    if (!_running) {
        return;
    }

    ACKNOWLEDGE_UNUSED(bHasFocus);
}

void Editor::OnSize(int iWidth, int iHeight) {
    vec2<U16> display_size = _mainWindow->getDrawableSize();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)iWidth, (float)iHeight);
    io.DisplayFramebufferScale = ImVec2(iWidth > 0 ? ((float)display_size.w / iWidth) : 0, iHeight > 0 ? ((float)display_size.h / iHeight) : 0);
}

void Editor::OnUTF8(const char* text) {
    if (!_running) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(text);
}

}; //namespace Divide
