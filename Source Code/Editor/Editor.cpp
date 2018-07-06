#include "stdafx.h"

#include "Headers/Editor.h"
#include "Headers/Sample.h"
#include "Editor/Widgets/Headers/MenuBar.h"
#include "Editor/Widgets/Headers/PanelManager.h"
#include "Editor/Widgets/Headers/ApplicationOutput.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Math/Headers/MathMatrices.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/FrameListenerManager.h"

#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Input/Headers/InputInterface.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Rendering/Camera/Headers/Camera.h"

#include <imgui_internal.h>
#include <imgui/addons/imguigizmo/ImGuizmo.h>

namespace Divide {

namespace {
    std::array<int, to_base(Editor::Theme::COUNT)> imguiThemeMap = {
        ImGuiStyle_Default,
        ImGuiStyle_DefaultDark,
        ImGuiStyle_Gray,
        ImGuiStyle_OSX,
        ImGuiStyle_DarkOpaque,
        ImGuiStyle_OSXOpaque,
        ImGuiStyle_Soft,
        ImGuiStyle_EdinBlack,
        ImGuiStyle_EdinWhite,
        ImGuiStyle_Maya
    };

    bool show_another_window = false;
    I32 window_opacity = 255;
    I32 previous_window_opacity = 255;
    bool show_test_window = true;
};

Editor::Editor(PlatformContext& context, Theme theme, Theme lostFocusTheme, Theme dimmedTheme)
    : PlatformContextComponent(context),
      _currentTheme(theme),
      _currentLostFocusTheme(lostFocusTheme),
      _currentDimmedTheme(dimmedTheme),
      _mainWindow(nullptr),
      _imguiContext(nullptr),
      _running(false),
      _sceneHovered(false),
      _sceneWasHovered(false),
      _scenePreviewFocused(false),
      _scenePreviewWasFocused(false),
      _showDebugWindow(false),
      _showSampleWindow(false),
      _activeWindowGUID(0),
      _consoleCallbackIndex(0),
      _editorUpdateTimer(Time::ADD_TIMER("Editor Update Timer")),
      _editorRenderTimer(Time::ADD_TIMER("Editor Render Timer"))
{
    _panelManager = std::make_unique<PanelManager>(context);
    _menuBar = std::make_unique<MenuBar>(context, true);
    _applicationOutput = std::make_unique<ApplicationOutput>(context, to_U16(512));

    REGISTER_FRAME_LISTENER(this, 99999);
}

Editor::~Editor()
{
    UNREGISTER_FRAME_LISTENER(this);
    close();
}

void Editor::idle() {
    assert(IS_IN_RANGE_INCLUSIVE(window_opacity, 0, 255));

    if (window_opacity != previous_window_opacity) {
        context().activeWindow().opacity(to_U8(window_opacity));
        previous_window_opacity = window_opacity;
    }

    _panelManager->idle();
}

bool Editor::init(const vec2<U16>& renderResolution) {

    if (_mainWindow != nullptr) {
        // double init
        return false;
    }
    
    createDirectories((Paths::g_saveLocation + Paths::Editor::g_saveLocation).c_str());
    _mainWindow = &_context.app().windowManager().getWindow(0u);
    if (_activeWindowGUID == 0) {
        // Only add these once
        I64 guid = _mainWindow->addEventListener(WindowEvent::CLOSE_REQUESTED, [this](const DisplayWindow::WindowEventArgs& args) { ACKNOWLEDGE_UNUSED(args); OnClose();});
        _windowListeners[to_base(WindowEvent::CLOSE_REQUESTED)].push_back(guid);
        guid = _mainWindow->addEventListener(WindowEvent::GAINED_FOCUS, [this](const DisplayWindow::WindowEventArgs& args) { OnFocus(args._flag);});
        _windowListeners[to_base(WindowEvent::GAINED_FOCUS)].push_back(guid);
        guid = _mainWindow->addEventListener(WindowEvent::RESIZED, [this](const DisplayWindow::WindowEventArgs& args) { OnSize(args.x, args.y);});
        _windowListeners[to_base(WindowEvent::RESIZED)].push_back(guid);
        guid = _mainWindow->addEventListener(WindowEvent::TEXT, [this](const DisplayWindow::WindowEventArgs& args) { OnUTF8(args._text);});
        _windowListeners[to_base(WindowEvent::TEXT)].push_back(guid);
    }
    _activeWindowGUID = _mainWindow->getGUID();

    _imguiContext = ImGui::CreateContext();
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

    ResourceCache& parentCache = _context.kernel().resourceCache();
    _fontTexture = CreateResource<Texture>(parentCache, textureDescriptor);
    assert(_fontTexture);

    Texture::TextureLoadInfo info;
    _fontTexture->loadData(info, (bufferPtr)pPixels, vec2<U16>(iWidth, iHeight));

    ResourceDescriptor shaderDescriptor("IMGUI");
    shaderDescriptor.setThreadedLoading(false);
    _imguiProgram = CreateResource<ShaderProgram>(parentCache, shaderDescriptor);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)_fontTexture->getHandle();
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();

    
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
    io.RenderDrawListsFn = nullptr;
    io.ImeWindowHandle = _mainWindow->handle()._handle;
    io.DisplaySize = ImVec2((float)renderResolution.width, (float)renderResolution.height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    _panelManager->init(_mainWindow->getDrawableSize());

    ImGui::ResetStyle(imguiThemeMap[to_base(_currentTheme)]);

    _consoleCallbackIndex = Console::bindConsoleOutput([this](const Console::OutputEntry& entry) {
        if (_applicationOutput != nullptr) {
            _applicationOutput->printText(entry);
        }
    });

    return true;
}

void Editor::close() {
    Console::unbindConsoleOutput(_consoleCallbackIndex);

    if (_mainWindow != nullptr) {
        for (U8 i = 0; i < to_base(WindowEvent::COUNT); ++i) {
            vectorImpl<I64>& guids = _windowListeners[i];
            for (I64 guid : guids) {
                _mainWindow->removeEventlistener(static_cast<WindowEvent>(i), guid);
            }
            guids.clear();
        }
    }
    _fontTexture.reset();
    _imguiProgram.reset();
    _panelManager->destroy();
    if (_imguiContext != nullptr) {
        ImGui::DestroyContext();
        _imguiContext = nullptr;
    }
}

void Editor::toggle(const bool state) {
    _running = state;
    if (!state) {
        toggleScenePreview(false);
    }
}

bool Editor::running() const {
    return _running;
}

bool Editor::shouldPauseSimulation() const {
    return _panelManager && _panelManager->simulationPauseRequested();
}

void Editor::update(const U64 deltaTimeUS) {
    if (!needInput()) {
        return;
    }
    Time::ScopedTimer timer(_editorUpdateTimer);
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = Time::MicrosecondsToSeconds<float>(deltaTimeUS);

        ToggleCursor(!io.MouseDrawCursor);
        if (io.MouseDrawCursor)
        {
            _mainWindow->setCursorStyle(CursorStyle::NONE);
        }
        else if (io.MousePos.x != -1.f && io.MousePos.y != -1.f)
        {
            switch (ImGui::GetCurrentContext()->MouseCursor)
            {
                case ImGuiMouseCursor_Arrow:
                    _mainWindow->setCursorStyle(CursorStyle::ARROW);
                    break;
                case ImGuiMouseCursor_TextInput:         // When hovering over InputText, etc.
                    _mainWindow->setCursorStyle(CursorStyle::TEXT_INPUT);
                    break;
                case ImGuiMouseCursor_ResizeAll:         // Unused
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

        if (io.WantMoveMouse) {
            context().app().windowManager().setCursorPosition((int)io.MousePos.x, (int)io.MousePos.y);
        }

        //_scenePreviewRect = _platform
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

bool Editor::renderMinimal(const U64 deltaTime) {
    if (showDebugWindow()) {
        drawIMGUIDebug(deltaTime);
    }
    if (showSampleWindow()) {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
        ImGui::ShowDemoWindow(&show_test_window);
    }

    return true;
}

bool Editor::renderFull(const U64 deltaTime) {
    drawMenuBar();
    _panelManager->draw(deltaTime);
    renderMinimal(deltaTime);

    return true;
}

bool Editor::framePostRenderStarted(const FrameEvent& evt) {
    Time::ScopedTimer timer(_editorRenderTimer);
    {
        ImGui::NewFrame();
    }
    
    if (!_running) {
        if (!renderMinimal(evt._timeSinceLastFrameUS)) {
            return false;
        }
    } else {
        if (!renderFull(evt._timeSinceLastFrameUS)) {
            return false;
        }
    }
  
    ImGui::Render();
    renderDrawList(ImGui::GetDrawData(), _mainWindow->getGUID());
    
    return true;
}

bool Editor::framePostRenderEnded(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    return true;
}

bool Editor::frameEnded(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);

    return true;
}

void Editor::drawOutputWindow() {
    if (_applicationOutput) {
        _applicationOutput->draw();
    }
}

void Editor::drawMenuBar() {
    if (_menuBar) {
        _menuBar->draw();
    }
}

void Editor::showDebugWindow(bool state) {
    _showDebugWindow = state;
}

void Editor::showSampleWindow(bool state) {
    _showSampleWindow = state;
}

bool Editor::showDebugWindow() const {
    return _showDebugWindow;
}

bool Editor::showSampleWindow() const {
    return _showSampleWindow;
}

void Editor::savePanelLayout() const {
    if (_panelManager) {
        _panelManager->saveToFile();
    }
}
void Editor::loadPanelLayout() {
    if (_panelManager) {
        _panelManager->loadFromFile();
    }
}

void Editor::saveTabLayout() const {
    if (_panelManager) {
        _panelManager->saveTabsToFile();
    }
}

void Editor::loadTabLayout() {
    if (_panelManager) {
        _panelManager->loadTabsFromFile();
    }
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

    if (pDrawData->CmdListsCount == 0) {
        return;
    }

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

void Editor::dim(bool hovered, bool focused) {
    ImGui::ResetStyle(imguiThemeMap[focused ? to_base(_currentDimmedTheme)
                                            : hovered ? to_base(_currentLostFocusTheme)
                                                      : to_base(_currentTheme)]);
}

bool Editor::toggleScenePreview(bool state) {
    state = state ? _sceneHovered : false;
    if (_scenePreviewFocused != state) {
        _scenePreviewWasFocused = _scenePreviewFocused;
        _scenePreviewFocused = state;
        _mainWindow->warp(_scenePreviewFocused, _scenePreviewRect);
        dim(_sceneHovered, _scenePreviewFocused);
    }

    return _scenePreviewFocused;
}

void Editor::setScenePreviewRect(const Rect<I32>& rect, bool hovered) {
    if (_sceneWasHovered != hovered) {
        _sceneWasHovered = _sceneHovered;
        _sceneHovered = hovered;
        dim(_sceneHovered, _scenePreviewFocused);
    }
    _scenePreviewRect.set(rect);
}

/// Key pressed: return true if input was consumed
bool Editor::onKeyDown(const Input::KeyEvent& key) {
    if (!needInput() || _scenePreviewFocused) {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[key._key] = true;

    io.KeyCtrl = key._key == Input::KeyCode::KC_LCONTROL || key._key == Input::KeyCode::KC_RCONTROL;
    io.KeyShift = key._key == Input::KeyCode::KC_LSHIFT || key._key == Input::KeyCode::KC_RSHIFT;
    io.KeyAlt = key._key == Input::KeyCode::KC_LMENU || key._key == Input::KeyCode::KC_RMENU;
    io.KeySuper = false;

    
    bool ret = ImGui::GetIO().WantCaptureKeyboard;
    return ret;
}

/// Key released: return true if input was consumed
bool Editor::onKeyUp(const Input::KeyEvent& key) {
    if (!needInput() || _scenePreviewFocused) {
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

    bool ret = ImGui::GetIO().WantCaptureKeyboard;
    return ret;
}

/// Mouse moved: return true if input was consumed
bool Editor::mouseMoved(const Input::MouseEvent& arg) {
    if (!needInput()) {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)arg.X(false).abs,
                         (float)arg.Y(false).abs);
    io.MouseWheel += (float)arg.Z(false).rel / 60.0f;

    if (!_scenePreviewFocused) {
        bool ret = ImGui::GetIO().WantCaptureMouse;
        return ret;
    }

    return false;
}

/// Mouse button pressed: return true if input was consumed
bool Editor::mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button) {
    ACKNOWLEDGE_UNUSED(arg);

    if (!needInput() || _scenePreviewFocused) {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[button == OIS::MB_Left ? 0 : button == OIS::MB_Right ? 1 : 2] = true;

    bool ret = ImGui::GetIO().WantCaptureMouse;
    return ret;
}

/// Mouse button released: return true if input was consumed
bool Editor::mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button) {
    ACKNOWLEDGE_UNUSED(arg);

    if (button == OIS::MB_Left) {
        toggleScenePreview(_running);
    }

    if (!needInput()) {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[button == OIS::MB_Left ? 0 : button == OIS::MB_Right ? 1 : 2] = false;

    if (!_scenePreviewFocused) {
        bool ret = ImGui::GetIO().WantCaptureMouse;
        return ret;
    }

    return false;
}

bool Editor::joystickButtonPressed(const Input::JoystickEvent &arg, Input::JoystickButton button) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(button);

    if (!needInput() || _scenePreviewFocused) {
        return false;
    }

    return false;
}

bool Editor::joystickButtonReleased(const Input::JoystickEvent &arg, Input::JoystickButton button) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(button);

    if (!needInput() || _scenePreviewFocused) {
        return false;
    }

    return false;
}

bool Editor::joystickAxisMoved(const Input::JoystickEvent &arg, I8 axis) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(axis);

    if (!needInput() || _scenePreviewFocused) {
        return false;
    }

    return false;
}

bool Editor::joystickPovMoved(const Input::JoystickEvent &arg, I8 pov) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(pov);

    if (!needInput() || _scenePreviewFocused) {
        return false;
    }

    return false;
}

bool Editor::joystickSliderMoved(const Input::JoystickEvent &arg, I8 index) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(index);

    if (!needInput() || _scenePreviewFocused) {
        return false;
    }

    return false;
}

bool Editor::joystickvector3Moved(const Input::JoystickEvent &arg, I8 index) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(index);

    if (!needInput() || _scenePreviewFocused) {
        return false;
    }

    return false;
}

bool Editor::OnClose() {
    if (!needInput()) {
        return false;
    }

    return true;
}

void Editor::OnFocus(bool bHasFocus) {
    if (!needInput()) {
        return;
    }

    ACKNOWLEDGE_UNUSED(bHasFocus);
}

void Editor::onSizeChange(const SizeChangeParams& params) {
    if (_mainWindow != nullptr) {
        ImGuiIO& io = ImGui::GetIO();
        if (params.isWindowResize) {
            _panelManager->resize(params.width, params.height);
        } else {
            io.DisplaySize.x = (float)params.width;
            io.DisplaySize.y = (float)params.height;
        }

        vec2<U16> renderResolution = context().gfx().renderingResolution();
        vec2<U16> display_size = _mainWindow->getDrawableSize();
        io.DisplayFramebufferScale.x = params.width > 0 ? ((float)display_size.w / renderResolution.width) : 0;
        io.DisplayFramebufferScale.y = params.height > 0 ? ((float)display_size.h / renderResolution.height) : 0;
    }
}

void Editor::OnSize(int iWidth, int iHeight) {
    ACKNOWLEDGE_UNUSED(iWidth);
    ACKNOWLEDGE_UNUSED(iHeight);
}

void Editor::OnUTF8(const char* text) {
    if (!needInput()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(text);
}

void Editor::drawIMGUIDebug(const U64 deltaTime) {
    DisplayWindow& window = context().activeWindow();

    static float f = 0.0f;
    ImGui::Text("Hello, world!");
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    ImGui::SliderInt("Opacity", &window_opacity, 0, 255);
    ImGui::ColorEdit4("clear color", window.clearColour()._v);
    if (ImGui::Button("Test Window")) show_test_window ^= 1;
    if (ImGui::Button("Another Window")) show_another_window ^= 1;
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Time since last frame %.3f ms", Time::MicrosecondsToMilliseconds<float>(deltaTime));
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
}

bool Editor::needInput() {
    return _running || showDebugWindow() || showSampleWindow();
}
}; //namespace Divide
