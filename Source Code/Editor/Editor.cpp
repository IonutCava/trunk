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

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/FrameListenerManager.h"

#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Input/Headers/InputInterface.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Rendering/Camera/Headers/Camera.h"

#include <imgui_internal.h>

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
      _running(false),
      _sceneHovered(false),
      _sceneWasHovered(false),
      _scenePreviewFocused(false),
      _scenePreviewWasFocused(false),
      _showDebugWindow(false),
      _showSampleWindow(false),
      _gizmosVisible(false),
      _enableGizmo(false),
      _activeWindowGUID(0),
      _consoleCallbackIndex(0),
      _editorUpdateTimer(Time::ADD_TIMER("Editor Update Timer")),
      _editorRenderTimer(Time::ADD_TIMER("Editor Render Timer"))
{
    _imguiContext.fill(nullptr);

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

    for (U8 i = 0; i < to_U8(Context::COUNT); ++i) {
        _imguiContext[i] = i == 0 ? ImGui::CreateContext() : ImGui::CreateContext(GetIO(0).Fonts);
        ImGui::SetCurrentContext(_imguiContext[i]);

        ImGuiIO& io = ImGui::GetIO();
        if (i == 0) {
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
        }

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
        io.ImeWindowHandle = _mainWindow->handle()._handle;
        io.DisplaySize = ImVec2((float)renderResolution.width, (float)renderResolution.height);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    }

    ImGui::SetCurrentContext(_imguiContext[to_base(Context::Editor)]);
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
    for (U8 i = 0; i < to_base(Context::COUNT); ++i) {
        if (_imguiContext[i] != nullptr) {
            ImGui::SetCurrentContext(_imguiContext[i]);
            ImGui::DestroyContext(_imguiContext[i]);
            _imguiContext[i] = nullptr;
        }
    }
}

void Editor::toggle(const bool state) {
    _running = state;
    if (!state) {
        toggleScenePreview(false);
    } else {
        _enableGizmo = true;
    }
}

bool Editor::running() const {
    return _running;
}

bool Editor::shouldPauseSimulation() const {
    return _panelManager && _panelManager->simulationPauseRequested();
}

void Editor::update(const U64 deltaTimeUS) {
    Time::ScopedTimer timer(_editorUpdateTimer);

    for (U8 i = 0; i < to_U8(Context::COUNT); ++i) {
        ImGuiIO& io = GetIO(i);
        io.DeltaTime = Time::MicrosecondsToSeconds<float>(deltaTimeUS);

        if (!needInput()) {
            continue;
        }

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

        if (io.WantSetMousePos) {
            context().app().windowManager().setCursorPosition((int)io.MousePos.x, (int)io.MousePos.y);
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

bool Editor::renderGizmos(const U64 deltaTime) {
    ACKNOWLEDGE_UNUSED(deltaTime);
   
    TransformValues valuesOut;
    if (_enableGizmo && !_selectedNodes.empty()) {
        SceneGraphNode* sgn = _selectedNodes.front();
        if (sgn != nullptr) {
            TransformComponent* const transform = sgn->get<TransformComponent>();
            if (transform != nullptr) {
                const Camera* camera = Attorney::SceneManagerCameraAccessor::playerCamera(_context.kernel().sceneManager());
                const mat4<F32>& cameraView = camera->getViewMatrix();
                const mat4<F32>& cameraProjection = camera->getProjectionMatrix();
                mat4<F32> matrix(transform->getLocalMatrix());

                ImGui::SetCurrentContext(_imguiContext[to_base(Context::Gizmo)]);
                ImGui::NewFrame();
                ImGuizmo::BeginFrame();
                const ImGuiIO& io = GetIO(to_base(Context::Gizmo));
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
                renderDrawList(ImGui::GetDrawData(), _mainWindow->getGUID(), false);

                return true;
            }
        }
    }

    return false;
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

bool Editor::frameSceneRenderEnded(const FrameEvent& evt) {

    //if (!_running) {
    //  return true;
    //}

    _gizmosVisible = renderGizmos(evt._timeSinceLastFrameUS);
    return true;
}

bool Editor::framePostRenderStarted(const FrameEvent& evt) {
    Time::ScopedTimer timer(_editorRenderTimer);
    {
        ImGui::SetCurrentContext(_imguiContext[to_base(Context::Editor)]);
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
    renderDrawList(ImGui::GetDrawData(), _mainWindow->getGUID(), true);
    
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

void Editor::enableGizmo(bool state) {
    _enableGizmo = state;
}

bool Editor::showDebugWindow() const {
    return _showDebugWindow;
}

bool Editor::showSampleWindow() const {
    return _showSampleWindow;
}

bool Editor::enableGizmo() const {
    return _enableGizmo;
}

void Editor::setTransformSettings(const TransformSettings& settings) {
    _transformSettings = settings;
}

const TransformSettings& Editor::getTransformSettings() const {
    return _transformSettings;
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
void Editor::renderDrawList(ImDrawData* pDrawData, I64 windowGUID, bool isPostPass)
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
    if (isPostPass && switchWindow) {
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
    beginDebugScopeCmd._scopeName = isPostPass ? "Render IMGUI [Post]" : "Render IMGUI [Pre]";
    GFX::BeginDebugScope(buffer, beginDebugScopeCmd);

    if (isPostPass && switchWindow) {
        GFX::SwitchWindowCommand switchWindowCmd;
        switchWindowCmd.windowGUID = _activeWindowGUID;
        GFX::AddSwitchWindow(buffer, switchWindowCmd);
    }

    if (!isPostPass) {
        // Draw the gizmos to the main render target but don't clear anything
        RTDrawDescriptor screenTarget;
        screenTarget.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        screenTarget.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
        screenTarget.drawMask().disableAll();
        screenTarget.drawMask().setEnabled(RTAttachmentType::Colour, 0, true);

        GFX::BeginRenderPassCommand beginRenderPassCmd;
        beginRenderPassCmd._target = RenderTargetID(RenderTargetUsage::SCREEN);
        beginRenderPassCmd._descriptor = screenTarget;
        beginRenderPassCmd._name = "DO_IMGUI_PRE_PASS";
        GFX::BeginRenderPass(buffer, beginRenderPassCmd);
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

    if (isPostPass) {
        if (switchWindow) {
            GFX::SwitchWindowCommand switchWindowCmd;
            switchWindowCmd.windowGUID = previousGUID;
            GFX::AddSwitchWindow(buffer, switchWindowCmd);
        }
    } else {
        GFX::EndRenderPassCommand endRenderPassCmd;
        GFX::EndRenderPass(buffer, endRenderPassCmd);
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
    if (_scenePreviewFocused != state) {
        _scenePreviewWasFocused = _scenePreviewFocused;
        _scenePreviewFocused = state;
        _mainWindow->warp(_scenePreviewFocused, _scenePreviewRect);
        dim(_sceneHovered, _scenePreviewFocused);
    }

    return _scenePreviewFocused;
}

void Editor::checkPreviewRectState() {
    checkPreviewRectState(hasGizmoFocus());
}

void Editor::checkPreviewRectState(bool gizmoFocus) {
    bool hovered = gizmoFocus ||
                   _scenePreviewRect.contains(ImGui::GetIO().MousePos.x,
                                              ImGui::GetIO().MousePos.y);

    if (_sceneWasHovered != hovered) {
        _sceneWasHovered = _sceneHovered;
        _sceneHovered = hovered;
    }
}

void Editor::setScenePreviewRect(const Rect<I32>& rect) {
    _scenePreviewRect.set(rect);
}

void Editor::selectionChangeCallback(PlayerIndex idx, SceneGraphNode* node) {
    if (idx == 0) {
        if (node == nullptr) {
            _selectedNodes.resize(0);
        }
        else {
            _selectedNodes.push_back(node);
        }
    }
}

/// Key pressed: return true if input was consumed
bool Editor::onKeyDown(const Input::KeyEvent& key) {
    if (!needInput()) {
        return false;
    }

    ImGuiIO& io = GetIO(hasSceneFocus() ? to_U8(Context::Gizmo) : to_U8(Context::Editor));
    io.KeysDown[key._key] = true;

    io.KeyCtrl = key._key == Input::KeyCode::KC_LCONTROL || key._key == Input::KeyCode::KC_RCONTROL;
    io.KeyShift = key._key == Input::KeyCode::KC_LSHIFT || key._key == Input::KeyCode::KC_RSHIFT;
    io.KeyAlt = key._key == Input::KeyCode::KC_LMENU || key._key == Input::KeyCode::KC_RMENU;
    io.KeySuper = false;

    return io.WantCaptureKeyboard;
}

/// Key released: return true if input was consumed
bool Editor::onKeyUp(const Input::KeyEvent& key) {
    if (!needInput()) {
        return false;
    }

    ImGuiIO& io = GetIO(hasSceneFocus() ? to_U8(Context::Gizmo) : to_U8(Context::Editor));
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
bool Editor::mouseMoved(const Input::MouseEvent& arg) {
    if (!needInput()) {
        return false;
    }

    for (U8 i = 0; i < to_base(Context::COUNT); ++i) {
        ImGuiIO& io = GetIO(i);
        io.MousePos.x  = (float)arg.X(i == 1).abs;
        io.MousePos.y  = (float)arg.Y(i == 1).abs;
        io.MouseWheel += (float)arg.Z(i == 1).rel / 60.0f;
    }

    bool gizmoFocus = false;
    bool sceneFocus = hasSceneFocus(gizmoFocus);
    checkPreviewRectState(gizmoFocus);

    return sceneFocus ? gizmoFocus : GetIO(to_base(Context::Editor)).WantCaptureMouse;
}

/// Mouse button pressed: return true if input was consumed
bool Editor::mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button) {
    ACKNOWLEDGE_UNUSED(arg);

    if (!needInput()) {
        return false;
    }

    ImGui::SetCurrentContext(_imguiContext[hasSceneFocus() ? to_U8(Context::Gizmo) : to_U8(Context::Editor)]);
    ImGuiIO& io = ImGui::GetIO();

    io.MouseDown[button == OIS::MB_Left ? 0 : button == OIS::MB_Right ? 1 : 2] = true;
    return io.WantCaptureMouse || ImGuizmo::IsOver();
}

/// Mouse button released: return true if input was consumed
bool Editor::mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button) {
    ACKNOWLEDGE_UNUSED(arg);

    if (button == OIS::MB_Left) {
        toggleScenePreview(_sceneHovered && _running);
    }

    if (!needInput()) {
        return false;
    }

    ImGui::SetCurrentContext(_imguiContext[hasSceneFocus() ? to_U8(Context::Gizmo) : to_U8(Context::Editor)]);
    ImGuiIO& io = ImGui::GetIO();

    io.MouseDown[button == OIS::MB_Left ? 0 : button == OIS::MB_Right ? 1 : 2] = false;

    return io.WantCaptureMouse || ImGuizmo::IsOver();
}

bool Editor::joystickButtonPressed(const Input::JoystickEvent &arg, Input::JoystickButton button) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(button);

    return false;
}

bool Editor::joystickButtonReleased(const Input::JoystickEvent &arg, Input::JoystickButton button) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(button);

    return false;
}

bool Editor::joystickAxisMoved(const Input::JoystickEvent &arg, I8 axis) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(axis);

    return false;
}

bool Editor::joystickPovMoved(const Input::JoystickEvent &arg, I8 pov) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(pov);

    return false;
}

bool Editor::joystickSliderMoved(const Input::JoystickEvent &arg, I8 index) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(index);

    return false;
}

bool Editor::joystickvector3Moved(const Input::JoystickEvent &arg, I8 index) {
    ACKNOWLEDGE_UNUSED(arg);
    ACKNOWLEDGE_UNUSED(index);

    return false;
}

bool Editor::OnClose() {
    return needInput();
}

void Editor::OnFocus(bool bHasFocus) {
    ACKNOWLEDGE_UNUSED(bHasFocus);
}

void Editor::onSizeChange(const SizeChangeParams& params) {
    if (_mainWindow != nullptr) {
        for (U8 i = 0; i < to_U8(Context::COUNT); ++i) {
            ImGuiIO& io = GetIO(i);

            if (params.isWindowResize && i == 0) {
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
}

void Editor::OnSize(int iWidth, int iHeight) {
    ACKNOWLEDGE_UNUSED(iWidth);
    ACKNOWLEDGE_UNUSED(iHeight);
}

void Editor::OnUTF8(const char* text) {
    if (!needInput()) {
        return;
    }

    GetIO(hasSceneFocus() ? to_base(Context::Gizmo) : to_base(Context::Editor)).AddInputCharactersUTF8(text);
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
    return _running || _gizmosVisible || showDebugWindow() || showSampleWindow();
}

bool Editor::hasGizmoFocus() {
    ImGuiContext* crtContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(_imguiContext[to_base(Context::Gizmo)]);
    bool imguizmoState = ImGuizmo::IsUsing();
    ImGui::SetCurrentContext(crtContext);
    return imguizmoState;
}

bool Editor::hasSceneFocus(bool& gizmoFocus) {
    gizmoFocus = hasGizmoFocus();
    return _scenePreviewFocused || !_running || gizmoFocus;
}

bool Editor::hasSceneFocus() {
    return _scenePreviewFocused || !_running || hasGizmoFocus();
}

ImGuiIO& Editor::GetIO(U8 idx) {
    assert(idx < _imguiContext.size());
    return _imguiContext[idx]->IO;
}

}; //namespace Divide
