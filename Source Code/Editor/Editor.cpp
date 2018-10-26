#include "stdafx.h"

#include "Headers/Editor.h"
#include "Headers/Sample.h"

#include "Editor/Widgets/Headers/MenuBar.h"

#include "Editor/Widgets/DockedWindows/Headers/OutputWindow.h"
#include "Editor/Widgets/DockedWindows/Headers/PropertyWindow.h"
#include "Editor/Widgets/DockedWindows/Headers/SceneViewWindow.h"
#include "Editor/Widgets/DockedWindows/Headers/SolutionExplorerWindow.h"

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
    bool show_another_window = false;
    I32 window_opacity = 255;
    I32 previous_window_opacity = 255;
    bool show_test_window = true;

    WindowManager* g_windowManager = nullptr;
    Editor* g_editor = nullptr;

    static std::array<U32, 5> g_sdlButtons = {
        (U32)SDL_BUTTON_LEFT,
        (U32)SDL_BUTTON_RIGHT,
        (U32)SDL_BUTTON_MIDDLE,
        (U32)SDL_BUTTON_X1,
        (U32)SDL_BUTTON_X2
    };

    struct ImGuiViewportData
    {
        DisplayWindow*  _window = nullptr;
        bool            _windowOwned = false;

        ~ImGuiViewportData() { IM_ASSERT(_window == nullptr); }
    };

    // FIXME-PLATFORM: SDL doesn't have an event to notify the application of display/monitor changes
    // ToDo: Remove this?
    void ImGui_UpdateMonitors() {
        const vector<WindowManager::MonitorData>& monitors = g_windowManager->monitorData();

        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        platform_io.Monitors.resize(0);
        platform_io.Monitors.reserve(to_I32(monitors.size()));

        for (const WindowManager::MonitorData& monitor : monitors) {
            // Warning: the validity of monitor DPI information on Windows depends on the application DPI awareness settings, which generally needs to be set in the manifest or at runtime.
            ImGuiPlatformMonitor imguiMonitor;
            
            imguiMonitor.MainPos = ImVec2((F32)monitor.viewport.x, (F32)monitor.viewport.y);
            imguiMonitor.MainSize = ImVec2((F32)monitor.viewport.z, (F32)monitor.viewport.w);
            imguiMonitor.WorkPos = ImVec2((F32)monitor.drawableArea.x, (F32)monitor.drawableArea.y);
            imguiMonitor.WorkSize = ImVec2((F32)monitor.drawableArea.z, (F32)monitor.drawableArea.w);
            imguiMonitor.DpiScale = monitor.dpi / 96.0f;
            platform_io.Monitors.push_back(imguiMonitor);
        }
    }
};

Editor::Editor(PlatformContext& context, ImGuiStyleEnum theme, ImGuiStyleEnum dimmedTheme)
    : PlatformContextComponent(context),
      _currentTheme(theme),
      _currentDimmedTheme(dimmedTheme),
      _mainWindow(nullptr),
      _running(false),
      _sceneHovered(false),
      _scenePreviewFocused(false),
      _selectedCamera(nullptr),
      _gizmo(nullptr),
      _imguiContext(nullptr),
      _showSampleWindow(false),
      _editorUpdateTimer(Time::ADD_TIMER("Editor Update Timer")),
      _editorRenderTimer(Time::ADD_TIMER("Editor Render Timer"))
{
    _menuBar = std::make_unique<MenuBar>(context, true);

    _mouseButtonPressed.fill(false);
    _dockedWindows.fill(nullptr);
    g_windowManager = &context.app().windowManager();
    g_editor = this;
    REGISTER_FRAME_LISTENER(this, 99999);

    //Test stuff
    _unsavedElements.push_back(1);
    _unsavedElements.push_back(2);
    _unsavedElements.push_back(3);
    _unsavedElements.push_back(4);
    _unsavedElements.push_back(5);
}

Editor::~Editor()
{

    close();
    for (DockedWindow* window : _dockedWindows) {
        MemoryManager::SAFE_DELETE(window);
    }
    _dockedWindows.fill(nullptr);

    UNREGISTER_FRAME_LISTENER(this);

    g_windowManager = nullptr;
    g_editor = nullptr;
}

void Editor::idle() {
    if (window_opacity != previous_window_opacity) {
        context().activeWindow().opacity(to_U8(window_opacity));
        previous_window_opacity = window_opacity;
    }
}

bool Editor::init(const vec2<U16>& renderResolution) {
    ACKNOWLEDGE_UNUSED(renderResolution);

    if (_mainWindow != nullptr) {
        // double init
        return false;
    }
    
    createDirectories((Paths::g_saveLocation + Paths::Editor::g_saveLocation).c_str());
    _mainWindow = &g_windowManager->getWindow(0u);

    IMGUI_CHECKVERSION();
    _imguiContext = ImGui::CreateContext();
    ImGuiIO& io = _imguiContext->IO;

    U8* pPixels;
    I32 iWidth;
    I32 iHeight;
    io.Fonts->AddFontDefault();
    io.Fonts->GetTexDataAsRGBA32(&pPixels, &iWidth, &iHeight);

    SamplerDescriptor sampler = {};
    sampler._minFilter = TextureFilter::LINEAR;
    sampler._magFilter = TextureFilter::LINEAR;

    TextureDescriptor texDescriptor(TextureType::TEXTURE_2D,
                                    GFXImageFormat::RGBA8,
                                    GFXDataFormat::UNSIGNED_BYTE);
    texDescriptor.setSampler(sampler);

    ResourceDescriptor resDescriptor("IMGUI_font_texture");
    resDescriptor.setThreadedLoading(false);
    resDescriptor.setFlag(true);
    resDescriptor.setPropertyDescriptor(texDescriptor);

    ResourceCache& parentCache = _context.kernel().resourceCache();
    _fontTexture = CreateResource<Texture>(parentCache, resDescriptor);
    assert(_fontTexture);

    Texture::TextureLoadInfo info;
    _fontTexture->loadData(info, (bufferPtr)pPixels, vec2<U16>(iWidth, iHeight));

    ResourceDescriptor shaderDescriptor("IMGUI");
    shaderDescriptor.setThreadedLoading(false);
    _imguiProgram = CreateResource<ShaderProgram>(parentCache, shaderDescriptor);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)_fontTexture->getHandle();


    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

    if (_context.config().gui.imgui.multiViewportEnabled) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsDecoration;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcon;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
    }

    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;        // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;  // We can create multi-viewports on the Platform side (optional)
        
    io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
    io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
    io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
    io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
    io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
    io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
    io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
    io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
    io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

    io.SetClipboardTextFn = SetClipboardText;
    io.GetClipboardTextFn = GetClipboardText;
    io.ClipboardUserData = nullptr;

    vec2<U16> display_size = _mainWindow->getDrawableSize();
    io.DisplaySize = ImVec2((F32)_mainWindow->getDimensions().width, (F32)_mainWindow->getDimensions().height);
    io.DisplayFramebufferScale = ImVec2(io.DisplaySize.x > 0 ? ((F32)display_size.width / io.DisplaySize.x) : 0.f,
                                        io.DisplaySize.y > 0 ? ((F32)display_size.height / io.DisplaySize.y) : 0.f);

    if (_context.config().gui.imgui.multiViewportEnabled) {
        ImGuiPlatformIO& platform_io = _imguiContext->PlatformIO;
        platform_io.Platform_CreateWindow = [](ImGuiViewport* viewport)
        {
            if (!g_windowManager) {
                return;
            }

            ImGuiViewportData* data = IM_NEW(ImGuiViewportData)();
            viewport->PlatformUserData = data;

            WindowDescriptor winDescriptor = {};
            winDescriptor.title = "No Title Yet";
            winDescriptor.targetDisplay = g_windowManager->getWindow(0u).currentDisplayIndex();
            winDescriptor.clearColour.set(0.0f, 0.0f, 0.0f, 1.0f);
            winDescriptor.flags = to_U32(WindowDescriptor::Flags::HIDDEN) | to_U32(WindowDescriptor::Flags::CLEAR_COLOUR) | to_U32(WindowDescriptor::Flags::CLEAR_DEPTH);
            // We don't enable SDL_WINDOW_RESIZABLE because it enforce windows decorations
            winDescriptor.flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? 0 : to_U32(WindowDescriptor::Flags::DECORATED) | to_U32(WindowDescriptor::Flags::RESIZEABLE);
            winDescriptor.flags |= (viewport->Flags & ImGuiViewportFlags_TopMost) ? to_U32(WindowDescriptor::Flags::ALWAYS_ON_TOP) : 0;
            winDescriptor.flags |= to_U32(WindowDescriptor::Flags::SHARE_CONTEXT);

            winDescriptor.dimensions.set(viewport->Size.x, viewport->Size.y);
            winDescriptor.position.set(viewport->Pos.x, viewport->Pos.y);
            winDescriptor.externalClose = true;

            data->_window = g_windowManager->createWindow(winDescriptor);
            data->_window->hidden(false);
            data->_window->bringToFront();
            data->_window->addEventListener(WindowEvent::CLOSE_REQUESTED, [viewport](const DisplayWindow::WindowEventArgs& args) { ACKNOWLEDGE_UNUSED(args); viewport->PlatformRequestClose = true; return true; });
            data->_window->addEventListener(WindowEvent::MOVED, [viewport](const DisplayWindow::WindowEventArgs& args) { ACKNOWLEDGE_UNUSED(args); viewport->PlatformRequestMove = true; return true; });
            data->_window->addEventListener(WindowEvent::RESIZED, [viewport](const DisplayWindow::WindowEventArgs& args) { ACKNOWLEDGE_UNUSED(args);  viewport->PlatformRequestResize = true;  return true; });

            data->_windowOwned = true;

            viewport->PlatformHandle = (void*)data->_window;
        };

        platform_io.Platform_DestroyWindow = [](ImGuiViewport* viewport)
        {
            if (!g_windowManager) {
                return;
            }

            if (ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData)
            {
                if (data->_window && data->_windowOwned) {
                    g_windowManager->destroyWindow(data->_window);
                }
                data->_window = nullptr;
                IM_DELETE(data);
            }
            viewport->PlatformUserData = viewport->PlatformHandle = nullptr;
        };

        platform_io.Platform_ShowWindow = [](ImGuiViewport* viewport) {
            ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
            data->_window->hidden(false);
        };

        platform_io.Platform_SetWindowPos = [](ImGuiViewport* viewport, ImVec2 pos) {
            ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
            data->_window->setPosition((I32)pos.x, (I32)pos.y);
        };

        platform_io.Platform_GetWindowPos = [](ImGuiViewport* viewport) -> ImVec2 {
            ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
            const vec2<I32>& pos = data->_window->getPosition();
            return ImVec2((F32)pos.x, (F32)pos.y);
        };

        platform_io.Platform_SetWindowSize = [](ImGuiViewport* viewport, ImVec2 size) {
            ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
            U16 w = to_U16(size.x);
            U16 h = to_U16(size.y);
            WAIT_FOR_CONDITION(data->_window->setDimensions(w, h));
        };

        platform_io.Platform_GetWindowSize = [](ImGuiViewport* viewport) -> ImVec2 {
            ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
            const vec2<U16>& dim = data->_window->getDimensions();
            return ImVec2((F32)dim.w, (F32)dim.h);
        };

        platform_io.Platform_SetWindowFocus = [](ImGuiViewport* viewport) {
            ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
            data->_window->bringToFront();
        };

        platform_io.Platform_GetWindowFocus = [](ImGuiViewport* viewport) -> bool {
            ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
            return data->_window->hasFocus();
        };

        platform_io.Platform_SetWindowTitle = [](ImGuiViewport* viewport, const char* title) {
            ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
            data->_window->title(title);
        };

        platform_io.Platform_RenderWindow = [](ImGuiViewport* viewport, void* platformContext) {
            PlatformContext* context = (PlatformContext*)platformContext;
            context->gfx().beginFrame(*(DisplayWindow*)viewport->PlatformHandle, false);
        };

        platform_io.Renderer_RenderWindow = [](ImGuiViewport* viewport, void* platformContext) {
            ACKNOWLEDGE_UNUSED(platformContext);

            if (!g_editor) {
                return;
            }

            ImGui::SetCurrentContext(g_editor->_imguiContext);
            g_editor->renderDrawList(viewport->DrawData, false, ((DisplayWindow*)viewport->PlatformHandle)->getGUID());
        };

        platform_io.Platform_SwapBuffers = [](ImGuiViewport* viewport, void* platformContext) {
            if (!g_windowManager) {
                return;
            }
            PlatformContext* context = (PlatformContext*)platformContext;
            context->gfx().endFrame(*(DisplayWindow*)viewport->PlatformHandle, false);
        };

        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        main_viewport->PlatformHandle = _mainWindow;

        ImGui_UpdateMonitors();

        ImGuiViewportData* data = IM_NEW(ImGuiViewportData)();
        data->_window = _mainWindow;
        data->_windowOwned = false;

        main_viewport->PlatformUserData = data;
    } else {
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        main_viewport->PlatformHandle = _mainWindow;
    }

    ImGui::ResetStyle(_currentTheme);
    
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    DockedWindow::Descriptor descriptor = {};
    descriptor.position = ImVec2(0, 0);
    descriptor.size = ImVec2(300, 550);
    descriptor.name = "Solution Explorer";
    _dockedWindows[to_base(WindowType::SolutionExplorer)] = MemoryManager_NEW SolutionExplorerWindow(*this, _context, descriptor);

    descriptor.position = ImVec2(to_F32(renderResolution.width) - 300, 0);
    descriptor.name = "Property Explorer";
    _dockedWindows[to_base(WindowType::Properties)] = MemoryManager_NEW PropertyWindow(*this, _context, descriptor);

    descriptor.position = ImVec2(0, 550);
    descriptor.size = ImVec2(to_F32(renderResolution.width), to_F32(renderResolution.height) - 550 - 3);
    descriptor.name = "Application Output";
    descriptor.flags |= ImGuiWindowFlags_NoTitleBar;
    _dockedWindows[to_base(WindowType::Output)] = MemoryManager_NEW OutputWindow(*this, descriptor);

    descriptor.position = ImVec2(150, 150);
    descriptor.size = ImVec2(640, 480);
    descriptor.name = "Scene View";
    descriptor.minSize = ImVec2(100, 100);
    descriptor.flags = 0;
    _dockedWindows[to_base(WindowType::SceneView)] = MemoryManager_NEW SceneViewWindow(*this, descriptor);


    _gizmo = std::make_unique<Gizmo>(*this, _imguiContext, _mainWindow);

    return true;
}

void Editor::close() {
    _fontTexture.reset();
    _imguiProgram.reset();
    _gizmo.reset();

    ImGui::SetCurrentContext(_imguiContext);
    ImGui::DestroyPlatformWindows();
    ImGui::DestroyContext(_imguiContext);
    _imguiContext = nullptr;
}

void Editor::toggle(const bool state) {
    _running = state;

    if (!state) {
        _scenePreviewFocused = _sceneHovered = false;
        ImGui::ResetStyle(_scenePreviewFocused ? _currentDimmedTheme : _currentTheme);
    } else {
        _gizmo->enable(true);
    }
}

bool Editor::running() const {
    return _running;
}

bool Editor::shouldPauseSimulation() const {
    return simulationPauseRequested();
}

void Editor::update(const U64 deltaTimeUS) {
    Time::ScopedTimer timer(_editorUpdateTimer);

    Attorney::GizmoEditor::update(*_gizmo, deltaTimeUS);

    ImGuiIO& io = _imguiContext->IO;
    io.DeltaTime = Time::MicrosecondsToSeconds<F32>(deltaTimeUS);

    if (!needInput()) {
        return;
    }
        
    ToggleCursor(!io.MouseDrawCursor);
    if (io.MouseDrawCursor || ImGui::GetMouseCursor() == ImGuiMouseCursor_None) {
        WindowManager::setCursorStyle(CursorStyle::NONE);
    } else if (io.MousePos.x != -1.f && io.MousePos.y != -1.f) {
        switch (ImGui::GetCurrentContext()->MouseCursor)
        {
            case ImGuiMouseCursor_Arrow:
                WindowManager::setCursorStyle(CursorStyle::ARROW);
                break;
            case ImGuiMouseCursor_TextInput:         // When hovering over InputText, etc.
                WindowManager::setCursorStyle(CursorStyle::TEXT_INPUT);
                break;
            case ImGuiMouseCursor_ResizeAll:         // Unused
                WindowManager::setCursorStyle(CursorStyle::HAND);
                break;
            case ImGuiMouseCursor_ResizeNS:          // Unused
                WindowManager::setCursorStyle(CursorStyle::RESIZE_NS);
                break;
            case ImGuiMouseCursor_ResizeEW:          // When hovering over a column
                WindowManager::setCursorStyle(CursorStyle::RESIZE_EW);
                break;
            case ImGuiMouseCursor_ResizeNESW:        // Unused
                WindowManager::setCursorStyle(CursorStyle::RESIZE_NESW);
                break;
            case ImGuiMouseCursor_ResizeNWSE:        // When hovering over the bottom-right corner of a window
                WindowManager::setCursorStyle(CursorStyle::RESIZE_NWSE);
                break;
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

bool Editor::renderMinimal(const U64 deltaTime) {
    ACKNOWLEDGE_UNUSED(deltaTime);

    if (showSampleWindow()) {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
        ImGui::ShowDemoWindow(&show_test_window);
    }

    return true;
}

bool Editor::renderFull(const U64 deltaTime) {
    static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (opt_flags & ImGuiDockNodeFlags_PassthruDockspace) {
        ImGui::SetNextWindowBgAlpha(0.0f);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("", NULL, windowFlags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    ImGuiID dockspaceId = ImGui::GetID("EditorDockspace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), opt_flags);

    drawMenuBar();

    for (DockedWindow* window : _dockedWindows) {
        window->draw();
    }

    renderMinimal(deltaTime);

    ImGui::End();

    return true;
}

bool Editor::frameSceneRenderEnded(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);

    Attorney::GizmoEditor::render(*_gizmo, 
                                  *Attorney::SceneManagerCameraAccessor::playerCamera(_context.kernel().sceneManager()));
    return true;
}

bool Editor::framePostRenderStarted(const FrameEvent& evt) {
    Time::ScopedTimer timer(_editorRenderTimer);
    {
        ImGui::SetCurrentContext(_imguiContext);
        IM_ASSERT(_imguiContext->IO.Fonts->IsBuilt());
        updateMousePosAndButtons();
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
    renderDrawList(ImGui::GetDrawData(), false, _mainWindow->getGUID());

    if (_imguiContext->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable){
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(&context());
    }

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

void Editor::drawMenuBar() {
    if (_menuBar) {
        _menuBar->draw();
    }
}

void Editor::showSampleWindow(bool state) {
    _showSampleWindow = state;
}

bool Editor::showSampleWindow() const {
    return _showSampleWindow;
}

void Editor::setTransformSettings(const TransformSettings& settings) {
    Attorney::GizmoEditor::setTransformSettings(*_gizmo, settings);
}

const TransformSettings& Editor::getTransformSettings() const {
    return Attorney::GizmoEditor::getTransformSettings(*_gizmo);
}

// Needs to be rendered immediately. *IM*GUI. IMGUI::NewFrame invalidates this data
void Editor::renderDrawList(ImDrawData* pDrawData, bool overlayOnScene, I64 windowGUID)
{
    if (windowGUID == -1) {
        windowGUID = _mainWindow->getGUID();
    }

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    I32 fb_width = (I32)(pDrawData->DisplaySize.x * io.DisplayFramebufferScale.x);
    I32 fb_height = (I32)(pDrawData->DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) {
        return;
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
    pipelineDesc._shaderProgramHandle = _imguiProgram->getID();

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = std::numeric_limits<U16>::max();
    beginDebugScopeCmd._scopeName = overlayOnScene ? "Render IMGUI [Overlay]" : "Render IMGUI [Full]";
    GFX::EnqueueCommand(buffer, beginDebugScopeCmd);

    if (overlayOnScene) {
        // Draw the gizmos and overlayed graphics to the main render target but don't clear anything
        RTDrawDescriptor screenTarget;
        screenTarget.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        screenTarget.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
        screenTarget.drawMask().disableAll();
        screenTarget.drawMask().setEnabled(RTAttachmentType::Colour, 0, true);

        GFX::BeginRenderPassCommand beginRenderPassCmd;
        beginRenderPassCmd._target = RenderTargetID(RenderTargetUsage::SCREEN);
        beginRenderPassCmd._descriptor = screenTarget;
        beginRenderPassCmd._name = "DO_IMGUI_PRE_PASS";
        GFX::EnqueueCommand(buffer, beginRenderPassCmd);
    }

    GFX::SetBlendCommand blendCmd;
    blendCmd._blendProperties = BlendingProperties{
        BlendProperty::SRC_ALPHA,
        BlendProperty::INV_SRC_ALPHA,
        BlendOperation::ADD
    };
    GFX::EnqueueCommand(buffer, blendCmd);

    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = _context.gfx().newPipeline(pipelineDesc);
    GFX::EnqueueCommand(buffer, pipelineCmd);

    GFX::SetViewportCommand viewportCmd;
    viewportCmd._viewport.set(0, 0, fb_width, fb_height);
    GFX::EnqueueCommand(buffer, viewportCmd);

    float L = pDrawData->DisplayPos.x;
    float R = pDrawData->DisplayPos.x + pDrawData->DisplaySize.x;
    float T = pDrawData->DisplayPos.y;
    float B = pDrawData->DisplayPos.y + pDrawData->DisplaySize.y;
    const F32 ortho_projection[4][4] =
    {
        { 2.0f / (R - L),    0.0f,               0.0f,   0.0f },
        { 0.0f,              2.0f / (T - B),     0.0f,   0.0f },
        { 0.0f,              0.0f,              -1.0f,   0.0f },
        { (R + L) / (L - R), (T + B) / (B - T),  0.0f,   1.0f },
    };

    GFX::SetCameraCommand cameraCmd;
    cameraCmd._cameraSnapshot = Camera::utilityCamera(Camera::UtilityCamera::_2D_FLIP_Y)->snapshot();
    memcpy(cameraCmd._cameraSnapshot._projectionMatrix.m, ortho_projection, sizeof(F32) * 16);
    GFX::EnqueueCommand(buffer, cameraCmd);

    GFX::DrawIMGUICommand drawIMGUI;
    drawIMGUI._data = pDrawData;
    drawIMGUI._windowGUID = windowGUID;
    GFX::EnqueueCommand(buffer, drawIMGUI);

    if (overlayOnScene) {
        GFX::EndRenderPassCommand endRenderPassCmd;
        GFX::EnqueueCommand(buffer, endRenderPassCmd);
    }

    GFX::EndDebugScopeCommand endDebugScope;
    GFX::EnqueueCommand(buffer, endDebugScope);

    _context.gfx().flushCommandBuffer(buffer);
}

void Editor::selectionChangeCallback(PlayerIndex idx, SceneGraphNode* node) {
    if (idx != 0) {
        return;
    }

    Attorney::GizmoEditor::updateSelection(*_gizmo, node);
}

/// Key pressed: return true if input was consumed
bool Editor::onKeyDown(const Input::KeyEvent& key) {
    return _gizmo->onKeyDown(key);;
}

/// Key released: return true if input was consumed
bool Editor::onKeyUp(const Input::KeyEvent& key) {
    return _gizmo->onKeyUp(key);
}

namespace {
    ImGuiViewport* FindViewportByPlatformHandle(ImGuiContext* context, DisplayWindow* window) {
        if (window != nullptr) {
            for (I32 i = 0; i != context->Viewports.Size; i++) {
                DisplayWindow* it = (DisplayWindow*)context->Viewports[i]->PlatformHandle;

                if (it != nullptr && it->getGUID() == window->getGUID()) {
                    return context->Viewports[i];
                }
            }
        }

        return nullptr;
    }

}
/// Mouse moved: return true if input was consumed
bool Editor::mouseMoved(const Input::MouseEvent& arg) {
    ImGuiIO& io = _imguiContext->IO;
    SceneViewWindow* sceneView = static_cast<SceneViewWindow*>(_dockedWindows[to_base(WindowType::SceneView)]);
    _sceneHovered = sceneView->isHovered() && sceneView->sceneRect().contains(io.MousePos.x, io.MousePos.y);
    
    return !_scenePreviewFocused ? io.WantCaptureMouse : _gizmo->mouseMoved(arg);
}

/// Mouse button pressed: return true if input was consumed
bool Editor::mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button) {
    return _gizmo->mouseButtonPressed(arg, button);
}

/// Mouse button released: return true if input was consumed
bool Editor::mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button) {
    if (_scenePreviewFocused != _sceneHovered) {
        ImGui::SetCurrentContext(_imguiContext);
        _scenePreviewFocused = _sceneHovered;
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::ResetStyle(_scenePreviewFocused ? _currentDimmedTheme : _currentTheme, style);

        const Rect<I32>& previewRect = static_cast<SceneViewWindow*>(_dockedWindows[to_base(WindowType::SceneView)])->sceneRect();
        _mainWindow->warp(_scenePreviewFocused, previewRect);
    }

    return _gizmo->mouseButtonReleased(arg, button);
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

void Editor::updateMousePosAndButtons() {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseHoveredViewport = 0;

    if (!io.WantSetMousePos) {
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    } else {
        g_windowManager->setCursorPosition((I32)io.MousePos.x,
                                           (I32)io.MousePos.y,
                                           (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0);
    }

    vec2<I32> mPos(-1);
    U32 state = WindowManager::getMouseState(mPos, false);

    bool anyDown = false;
    DisplayWindow* focusedWindow = g_windowManager->getFocusedWindow();
    if (focusedWindow != nullptr) {
        vec2<I32> wPos = focusedWindow->getPosition();
        //mPos -= wPos;
        if (ImGuiViewport* viewport = FindViewportByPlatformHandle(_imguiContext, focusedWindow)) {
            io.MousePos = ImVec2(viewport->Pos.x + (F32)mPos.x, viewport->Pos.y + (F32)mPos.y);
            /*static bool print = false;
            if (print) {
                Console::printfn("X = %5.2f, Y = %5.2f, CAPTURE: %s", io.MousePos.x, io.MousePos.y, anyDown ? "true" : "false");
            }
            print = !print;*/
        }
    }

    for (size_t i = 0; i < g_sdlButtons.size(); ++i) {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = _mouseButtonPressed[i] || (state & SDL_BUTTON(g_sdlButtons[i])) != 0;
        anyDown = anyDown || io.MouseDown[i];
    }
    _mouseButtonPressed.fill(false);
    WindowManager::setCaptureMouse(anyDown);
}

bool Editor::onSDLInputEvent(SDL_Event event) {
    if (!needInput()) {
        return false;
    }

    ImGuiIO& io = _imguiContext->IO;
    switch (event.type) {
        case SDL_TEXTINPUT:
        {
            io.AddInputCharactersUTF8(event.text.text);
            return true;
        }break;
        case SDL_MOUSEWHEEL:
        {
            if (event.wheel.x > 0) {
                io.MouseWheelH += 1;
            }
            if (event.wheel.x < 0) {
                io.MouseWheelH -= 1;
            }
            if (event.wheel.y > 0) {
                io.MouseWheel += 1;
            }
            if (event.wheel.y < 0) {
                io.MouseWheel -= 1;
            }
            return true;
        };
        case SDL_MOUSEBUTTONDOWN:
        {
            for (size_t i = 0; i < g_sdlButtons.size(); ++i) {
                if (event.button.button == g_sdlButtons[i]) {
                    _mouseButtonPressed[i] = true;
                }
            }

            return true;
        };
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            I32 key = event.key.keysym.scancode;
            IM_ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
            io.KeysDown[key] = (event.type == SDL_KEYDOWN);
            io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
            io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
            io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
            io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
            return true;
        };
    };
    return false;
}

void Editor::onSizeChange(const SizeChangeParams& params) {
    if (!params.isWindowResize || _mainWindow == nullptr || params.winGUID != _mainWindow->getGUID()) {
        return;
    }

    vec2<U16> displaySize = _mainWindow->getDrawableSize();

    ImGuiIO& io = _imguiContext->IO;
    io.DisplaySize = ImVec2((F32)params.width, (F32)params.height);
    io.DisplayFramebufferScale = ImVec2(params.width > 0 ? ((F32)displaySize.width / params.width) : 0.f,
                                        params.height > 0 ? ((F32)displaySize.height / params.height) : 0.f);

    Attorney::GizmoEditor::onSizeChange(*_gizmo, params, displaySize);
}

void Editor::setSelectedCamera(Camera* camera) {
    _selectedCamera = camera;
}

Camera* Editor::getSelectedCamera() const {
    return _selectedCamera;
}

bool Editor::needInput() const {
    return _running || showSampleWindow();
}

bool Editor::simulationPauseRequested() const {
    SceneViewWindow* sceneView = static_cast<SceneViewWindow*>(_dockedWindows[to_base(WindowType::SceneView)]);
    return !sceneView->step();
}

bool Editor::hasUnsavedElements() const {
    return !_unsavedElements.empty();
}

void Editor::saveElement(I64 elementGUID) {
    if (elementGUID == -1) {
        _unsavedElements.clear();
        return;
    }

    auto it = std::find(std::cbegin(_unsavedElements),
                        std::cend(_unsavedElements),
                        elementGUID);

    if (it != std::cend(_unsavedElements)) {
        _unsavedElements.erase(it);
    }
}
}; //namespace Divide
