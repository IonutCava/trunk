#include "stdafx.h"

#include "Headers/Editor.h"
#include "Headers/Sample.h"

#include "Editor/Widgets/Headers/MenuBar.h"

#include "Editor/Widgets/DockedWindows/Headers/OutputWindow.h"
#include "Editor/Widgets/DockedWindows/Headers/PropertyWindow.h"
#include "Editor/Widgets/DockedWindows/Headers/SceneViewWindow.h"
#include "Editor/Widgets/DockedWindows/Headers/ContentExplorerWindow.h"
#include "Editor/Widgets/DockedWindows/Headers/SolutionExplorerWindow.h"

#include "Editor/Widgets/Headers/ImGuiExtensions.h"

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
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Rendering/Camera/Headers/Camera.h"

#include <imgui_internal.h>
#include <imgui/addons/imgui_memory_editor/imgui_memory_editor.h>

namespace Divide {

namespace {
    bool show_another_window = false;
    I32 window_opacity = 255;
    I32 previous_window_opacity = 255;
    const char* g_editorSaveFile = "Editor.xml";

    WindowManager* g_windowManager = nullptr;
    Editor* g_editor = nullptr;

    static std::array<Input::MouseButton, 5> g_oisButtons = {
        Input::MouseButton::MB_Left,
        Input::MouseButton::MB_Right,
        Input::MouseButton::MB_Middle,
        Input::MouseButton::MB_Button3,
        Input::MouseButton::MB_Button4,
    };

    struct ImGuiViewportData
    {
        DisplayWindow*  _window = nullptr;
        bool            _windowOwned = false;

        ~ImGuiViewportData() { IM_ASSERT(_window == nullptr); }
    };
};

void InitBasicImGUIState(ImGuiIO& io) {
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
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
}

Editor::Editor(PlatformContext& context, ImGuiStyleEnum theme, ImGuiStyleEnum dimmedTheme)
    : PlatformContextComponent(context),
      _currentTheme(theme),
      _currentDimmedTheme(dimmedTheme),
      _mainWindow(nullptr),
      _running(false),
      _showSampleWindow(false),
      _showMemoryEditor(false),
      _sceneHovered(false),
      _scenePreviewFocused(false),
      _selectedCamera(nullptr),
      _gizmo(nullptr),
      _imguiContext(nullptr),
      _editorUpdateTimer(Time::ADD_TIMER("Editor Update Timer")),
      _editorRenderTimer(Time::ADD_TIMER("Editor Render Timer"))
{
    _menuBar = std::make_unique<MenuBar>(context, true);

    _dockedWindows.fill(nullptr);
    g_windowManager = &context.app().windowManager();
    g_editor = this;
    REGISTER_FRAME_LISTENER(this, 99999);

    _memoryEditorData = std::make_pair(nullptr, 0);

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


    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

    if (_context.config().gui.imgui.multiViewportEnabled) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        if (_context.config().gui.imgui.windowDecorationsEnabled) {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsDecoration;
        }

        io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcon;

        if (_context.config().gui.imgui.dontMergeFloatingWindows) {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;
        }

        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
    }

    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;        // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;  // We can create multi-viewports on the Platform side (optional)
        
    InitBasicImGUIState(io);

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

        const vector<WindowManager::MonitorData>& monitors = g_windowManager->monitorData();
        I32 monitorCount = to_I32(monitors.size());

        platform_io.Monitors.resize(monitorCount);

        for (I32 i = 0; i < monitorCount; ++i) {
            const WindowManager::MonitorData& monitor = monitors[i];
            ImGuiPlatformMonitor& imguiMonitor = platform_io.Monitors[i];

            // Warning: the validity of monitor DPI information on Windows depends on the application DPI awareness settings, which generally needs to be set in the manifest or at runtime.
            imguiMonitor.MainPos = ImVec2((F32)monitor.viewport.x, (F32)monitor.viewport.y);
            imguiMonitor.MainSize = ImVec2((F32)monitor.viewport.z, (F32)monitor.viewport.w);
            imguiMonitor.WorkPos = ImVec2((F32)monitor.drawableArea.x, (F32)monitor.drawableArea.y);
            imguiMonitor.WorkSize = ImVec2((F32)monitor.drawableArea.z, (F32)monitor.drawableArea.w);
            imguiMonitor.DpiScale = monitor.dpi / 96.0f;
        }

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

    descriptor.position = ImVec2(0, 550.0f);
    descriptor.size = ImVec2(to_F32(renderResolution.width * 0.5f), to_F32(renderResolution.height) - 550 - 3);
    descriptor.name = "Content Explorer";
    descriptor.flags |= ImGuiWindowFlags_NoTitleBar;
    _dockedWindows[to_base(WindowType::ContentExplorer)] = MemoryManager_NEW ContentExplorerWindow(*this, descriptor);

    descriptor.position = ImVec2(to_F32(renderResolution.width * 0.5f), 550);
    descriptor.size = ImVec2(to_F32(renderResolution.width * 0.5f), to_F32(renderResolution.height) - 550 - 3);
    descriptor.name = "Application Output";
    _dockedWindows[to_base(WindowType::Output)] = MemoryManager_NEW OutputWindow(*this, descriptor);

    descriptor.position = ImVec2(150, 150);
    descriptor.size = ImVec2(640, 480);
    descriptor.name = "Scene View";
    descriptor.minSize = ImVec2(100, 100);
    descriptor.flags = 0;
    _dockedWindows[to_base(WindowType::SceneView)] = MemoryManager_NEW SceneViewWindow(*this, descriptor);


    _gizmo = std::make_unique<Gizmo>(*this, _imguiContext, _mainWindow);

    loadFromXML();

    return true;
}

void Editor::close() {
    _fontTexture.reset();
    _imguiProgram.reset();
    _gizmo.reset();

    if (_imguiContext != nullptr) {
        ImGui::SetCurrentContext(_imguiContext);
        ImGui::DestroyPlatformWindows();
        ImGui::DestroyContext(_imguiContext);
        _imguiContext = nullptr;
    }
}

void Editor::updateCameraSnapshot() {
    Camera* playerCam = Attorney::SceneManagerCameraAccessor::playerCamera(_context.kernel().sceneManager());
    if (playerCam != nullptr) {
        _cameraSnapshots[playerCam->getGUID()] = playerCam->snapshot();
    }
}

void Editor::toggle(const bool state) {
    if (_running == state) {
        return;
    }

    _running = state;

    _gizmo->enable(state);

    if (!state) {
        _sceneHovered = false;
        scenePreviewFocused(false);
        ImGui::ResetStyle(scenePreviewFocused() ? _currentDimmedTheme : _currentTheme);

        Camera* playerCam = Attorney::SceneManagerCameraAccessor::playerCamera(_context.kernel().sceneManager());
        if (playerCam != nullptr) {
            auto it = _cameraSnapshots.find(playerCam->getGUID());
            if (it != std::end(_cameraSnapshots)) {
                playerCam->fromSnapshot(it->second);
            }
        }

    } else {
        updateCameraSnapshot();
        static_cast<ContentExplorerWindow*>(_dockedWindows[to_base(WindowType::ContentExplorer)])->init();
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
        WindowManager::SetCursorStyle(CursorStyle::NONE);
    } else if (io.MousePos.x != -1.f && io.MousePos.y != -1.f) {
        switch (ImGui::GetCurrentContext()->MouseCursor)
        {
            case ImGuiMouseCursor_Arrow:
                WindowManager::SetCursorStyle(CursorStyle::ARROW);
                break;
            case ImGuiMouseCursor_TextInput:         // When hovering over InputText, etc.
                WindowManager::SetCursorStyle(CursorStyle::TEXT_INPUT);
                break;
            case ImGuiMouseCursor_ResizeAll:         // Unused
                WindowManager::SetCursorStyle(CursorStyle::RESIZE_ALL);
                break;
            case ImGuiMouseCursor_ResizeNS:          // Unused
                WindowManager::SetCursorStyle(CursorStyle::RESIZE_NS);
                break;
            case ImGuiMouseCursor_ResizeEW:          // When hovering over a column
                WindowManager::SetCursorStyle(CursorStyle::RESIZE_EW);
                break;
            case ImGuiMouseCursor_ResizeNESW:        // Unused
                WindowManager::SetCursorStyle(CursorStyle::RESIZE_NESW);
                break;
            case ImGuiMouseCursor_ResizeNWSE:        // When hovering over the bottom-right corner of a window
                WindowManager::SetCursorStyle(CursorStyle::RESIZE_NWSE);
                break;
            case ImGuiMouseCursor_Hand:
                WindowManager::SetCursorStyle(CursorStyle::HAND);
                break;
        }
    }

    static_cast<ContentExplorerWindow*>(_dockedWindows[to_base(WindowType::ContentExplorer)])->update(deltaTimeUS);
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

bool Editor::render(const U64 deltaTime) {
    ACKNOWLEDGE_UNUSED(deltaTime);

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

    if (_showMemoryEditor) {
        if (_memoryEditorData.first != nullptr && _memoryEditorData.second > 0) {
            static MemoryEditor memEditor;
            memEditor.DrawWindow("Memory Editor", _memoryEditorData.first, _memoryEditorData.second);
        }
    }

    if (_showSampleWindow) {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
        ImGui::ShowDemoWindow(&_showSampleWindow);
    }

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
    if (!_running) {
        return true;
    }

    Time::ScopedTimer timer(_editorRenderTimer);
    {
        ImGui::SetCurrentContext(_imguiContext);
        IM_ASSERT(_imguiContext->IO.Fonts->IsBuilt());
        updateMousePosAndButtons();
        ImGui::NewFrame();
    }

    if (render(evt._timeSinceLastFrameUS)) {

        ImGui::Render();
        renderDrawList(ImGui::GetDrawData(), false, _mainWindow->getGUID());

        if (_imguiContext->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(&context());
        }

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

void Editor::drawMenuBar() {
    if (_menuBar) {
        _menuBar->draw();
    }
}

void Editor::setTransformSettings(const TransformSettings& settings) {
    Attorney::GizmoEditor::setTransformSettings(*_gizmo, settings);
}

const TransformSettings& Editor::getTransformSettings() const {
    return Attorney::GizmoEditor::getTransformSettings(*_gizmo);
}

const Rect<I32>& Editor::scenePreviewRect(bool globalCoords) const {
    SceneViewWindow* sceneView = static_cast<SceneViewWindow*>(_dockedWindows[to_base(WindowType::SceneView)]);
    return sceneView->sceneRect(globalCoords);
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

    RenderStateBlock state = {};
    state.setCullMode(CullMode::CCW);
    state.setZRead(false);
    state.setScissorTest(true);

    PipelineDescriptor pipelineDesc = {};
    pipelineDesc._stateHash = state.getHash();
    pipelineDesc._shaderProgramHandle = _imguiProgram->getID();

    GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
    beginDebugScopeCmd._scopeID = std::numeric_limits<U16>::max();
    beginDebugScopeCmd._scopeName = overlayOnScene ? "Render IMGUI [Overlay]" : "Render IMGUI [Full]";
    GFX::EnqueueCommand(buffer, beginDebugScopeCmd);

    if (overlayOnScene) {
        // Draw the gizmos and overlayed graphics to the main render target but don't clear anything
        RTDrawDescriptor screenTarget = {};
        screenTarget.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        screenTarget.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
        screenTarget.drawMask().disableAll();
        screenTarget.drawMask().setEnabled(RTAttachmentType::Colour, 0, true);

        GFX::BeginRenderPassCommand beginRenderPassCmd = {};
        beginRenderPassCmd._target = RenderTargetID(RenderTargetUsage::SCREEN);
        beginRenderPassCmd._descriptor = screenTarget;
        beginRenderPassCmd._name = "DO_IMGUI_PRE_PASS";
        GFX::EnqueueCommand(buffer, beginRenderPassCmd);
    }

    GFX::SetBlendCommand blendCmd = {};
    blendCmd._blendProperties = BlendingProperties{
        BlendProperty::SRC_ALPHA,
        BlendProperty::INV_SRC_ALPHA,
        BlendOperation::ADD
    };
    GFX::EnqueueCommand(buffer, blendCmd);

    GFX::BindPipelineCommand pipelineCmd = {};
    pipelineCmd._pipeline = _context.gfx().newPipeline(pipelineDesc);
    GFX::EnqueueCommand(buffer, pipelineCmd);

    PushConstants pushConstants = {};
    pushConstants.set("toggleChannel", GFX::PushConstantType::IVEC4, vec4<I32>(1, 1, 1, 1));
    pushConstants.set("depthTexture", GFX::PushConstantType::INT, 0);
    pushConstants.set("depthRange", GFX::PushConstantType::VEC2, vec2<F32>(0.0f, 1.0f));

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants = pushConstants;
    GFX::EnqueueCommand(buffer, pushConstantsCommand);

    GFX::SetViewportCommand viewportCmd = {};
    viewportCmd._viewport.set(0, 0, fb_width, fb_height);
    GFX::EnqueueCommand(buffer, viewportCmd);

    F32 L = pDrawData->DisplayPos.x;
    F32 R = pDrawData->DisplayPos.x + pDrawData->DisplaySize.x;
    F32 T = pDrawData->DisplayPos.y;
    F32 B = pDrawData->DisplayPos.y + pDrawData->DisplaySize.y;
    const F32 ortho_projection[4][4] =
    {
        { 2.0f / (R - L),    0.0f,               0.0f,   0.0f },
        { 0.0f,              2.0f / (T - B),     0.0f,   0.0f },
        { 0.0f,              0.0f,              -1.0f,   0.0f },
        { (R + L) / (L - R), (T + B) / (B - T),  0.0f,   1.0f },
    };

    GFX::SetCameraCommand cameraCmd = {};
    cameraCmd._cameraSnapshot = Camera::utilityCamera(Camera::UtilityCamera::_2D_FLIP_Y)->snapshot();
    memcpy(cameraCmd._cameraSnapshot._projectionMatrix.m, ortho_projection, sizeof(F32) * 16);
    GFX::EnqueueCommand(buffer, cameraCmd);

    GFX::DrawIMGUICommand drawIMGUI = {};
    drawIMGUI._data = pDrawData;
    drawIMGUI._windowGUID = windowGUID;
    GFX::EnqueueCommand(buffer, drawIMGUI);

    if (overlayOnScene) {
        GFX::EndRenderPassCommand endRenderPassCmd = {};
        GFX::EnqueueCommand(buffer, endRenderPassCmd);
    }

    GFX::EndDebugScopeCommand endDebugScope = {};
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
    if (scenePreviewFocused()) {
        return _gizmo->onKeyDown(key);
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
bool Editor::onKeyUp(const Input::KeyEvent& key) {
    if (scenePreviewFocused()) {
        return _gizmo->onKeyUp(key);
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

ImGuiViewport* Editor::findViewportByPlatformHandle(ImGuiContext* context, DisplayWindow* window) {
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

/// Mouse moved: return true if input was consumed
bool Editor::mouseMoved(const Input::MouseMoveEvent& arg) {
    if (!arg.wheelEvent()) {
        SceneViewWindow* sceneView = static_cast<SceneViewWindow*>(_dockedWindows[to_base(WindowType::SceneView)]);
        ImVec2 mousePos = _imguiContext->IO.MousePos;
        _sceneHovered = sceneView->isHovered() && sceneView->sceneRect(true).contains(mousePos.x, mousePos.y);
    }

    if (scenePreviewFocused()) {
        return _gizmo->mouseMoved(arg);
    }

    ImGuiIO& io = _imguiContext->IO;

    if (arg.wheelEvent()) {
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

    return io.WantCaptureMouse;
}

/// Mouse button pressed: return true if input was consumed
bool Editor::mouseButtonPressed(const Input::MouseButtonEvent& arg) {
    if (scenePreviewFocused()) {
        return _gizmo->mouseButtonPressed(arg);
    }

    ImGuiIO& io = _imguiContext->IO;
    for (U8 i = 0; i < 5; ++i) {
        if (arg.button == g_oisButtons[i]) {
            io.MouseDown[i] = true;
            break;
        }
    }
    return io.WantCaptureMouse;
}

/// Mouse button released: return true if input was consumed
bool Editor::mouseButtonReleased(const Input::MouseButtonEvent& arg) {
    if (scenePreviewFocused() != _sceneHovered) {
        ImGui::SetCurrentContext(_imguiContext);
        scenePreviewFocused(_sceneHovered);
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::ResetStyle(scenePreviewFocused() ? _currentDimmedTheme : _currentTheme, style);
    }

    if (scenePreviewFocused()) {
        return _gizmo->mouseButtonReleased(arg);
    }

    ImGuiIO& io = _imguiContext->IO;
    for (U8 i = 0; i < 5; ++i) {
        if (arg.button == g_oisButtons[i]) {
            io.MouseDown[i] = false;
            break;
        }
    }
    return io.WantCaptureMouse;
}

bool Editor::joystickButtonPressed(const Input::JoystickEvent &arg) {
    if (scenePreviewFocused()) {
        return _gizmo->joystickButtonPressed(arg);
    }

    return false;
}

bool Editor::joystickButtonReleased(const Input::JoystickEvent &arg) {
    if (scenePreviewFocused()) {
        return _gizmo->joystickButtonReleased(arg);
    }

    return false;
}

bool Editor::joystickAxisMoved(const Input::JoystickEvent &arg) {
    if (scenePreviewFocused()) {
        return _gizmo->joystickAxisMoved(arg);
    }

    return false;
}

bool Editor::joystickPovMoved(const Input::JoystickEvent &arg) {
    if (scenePreviewFocused()) {
        return _gizmo->joystickPovMoved(arg);
    }

    return false;
}

bool Editor::joystickBallMoved(const Input::JoystickEvent &arg) {
    if (scenePreviewFocused()) {
        return _gizmo->joystickBallMoved(arg);
    }

    return false;
}

bool Editor::joystickAddRemove(const Input::JoystickEvent &arg) {
    if (scenePreviewFocused()) {
        return _gizmo->joystickAddRemove(arg);
    }

    return false;
}

bool Editor::joystickRemap(const Input::JoystickEvent &arg) {
    if (scenePreviewFocused()) {
        return _gizmo->joystickRemap(arg);
    }

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
    WindowManager::getMouseState(mPos, false);

    if (ImGuiViewport* viewport = findViewportByPlatformHandle(_imguiContext, g_windowManager->getFocusedWindow())) {
        io.MousePos = ImVec2(viewport->Pos.x + (F32)mPos.x, viewport->Pos.y + (F32)mPos.y);
    }

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (I32 n = 0; n < platform_io.Viewports.Size; n++) {
        ImGuiViewport* viewport = platform_io.Viewports[n];
        DisplayWindow* window = (DisplayWindow*)viewport->PlatformHandle;
        if (window != nullptr) {
            if (window->isHovered() && !(viewport->Flags & ImGuiViewportFlags_NoInputs)) {
                io.MouseHoveredViewport = viewport->ID;
            }
        }
    }

    bool anyDown = false;
    for (size_t i = 0; i < 5; ++i) {
        if (io.MouseDown[i]) {
            anyDown = true;
            break;
        }
    }

    WindowManager::setCaptureMouse(anyDown);
}

bool Editor::onUTF8(const Input::UTF8Event& arg) {
    if (!needInput()) {
        return false;
    }

    ImGuiIO& io = _imguiContext->IO;
    io.AddInputCharactersUTF8(arg._text);

    return io.WantCaptureKeyboard;
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
    return _running;
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

void Editor::toggleMemoryEditor(bool state) {
    _showMemoryEditor = state;
}

struct TextureCallbackData {
    vec4<I32> _colourData = {1, 1, 1, 1};
    GFXDevice* _gfxDevice = nullptr;
    bool _isDepthTexture = false;
    vec2<F32> _depthRange = { 0.0f, 1.0f };
};

bool Editor::modalTextureView(const char* modalName, const Texture_ptr& tex, const vec2<F32>& dimensions, bool preserveAspect) {
    
    ImDrawCallback toggleColours { [](const ImDrawList* parent_list, const ImDrawCmd* cmd) -> void {
        ACKNOWLEDGE_UNUSED(parent_list);

        TextureCallbackData  data = *static_cast<TextureCallbackData*>(cmd->UserCallbackData);

        GFX::ScopedCommandBuffer sBuffer = GFX::allocateScopedCommandBuffer();
        GFX::CommandBuffer& buffer = sBuffer();

        PushConstants pushConstants = {};
        pushConstants.set("toggleChannel", GFX::PushConstantType::IVEC4, data._colourData);
        pushConstants.set("depthTexture", GFX::PushConstantType::INT, data._isDepthTexture ? 1 : 0);
        pushConstants.set("depthRange", GFX::PushConstantType::VEC2, data._depthRange);

        GFX::SendPushConstantsCommand pushConstantsCommand = {};
        pushConstantsCommand._constants = pushConstants;
        GFX::EnqueueCommand(buffer, pushConstantsCommand);
        data._gfxDevice->flushCommandBuffer(buffer);
    } };

    bool closed = false;
    if (ImGui::BeginPopupModal(modalName, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        assert(tex != nullptr);


        assert(modalName != nullptr);
        static TextureCallbackData data = {};
        data._gfxDevice = &_context.gfx();

        static TextureCallbackData defaultData = {};
        defaultData._gfxDevice = &_context.gfx();
        defaultData._isDepthTexture = false;

        static std::array<bool, 4> state = { true, true, true, true };

        data._isDepthTexture = tex->getDescriptor().baseFormat() == GFXImageFormat::DEPTH_COMPONENT;

        U8 numChannels = tex->getDescriptor().numChannels();
        bool isAlphaType = tex->getDescriptor().baseFormat() == GFXImageFormat::LUMINANCE_ALPHA ||
                           tex->getDescriptor().baseFormat() == GFXImageFormat::ALPHA;

        if (numChannels > 0) {
            if (data._isDepthTexture) {
                ImGui::Text("Depth: ");  ImGui::SameLine(); ImGui::ToggleButton("Depth", &state[0]);
                ImGui::SameLine();
                ImGui::Text("Range: "); ImGui::SameLine();
                ImGui::DragFloatRange2("", &data._depthRange[0], &data._depthRange[1], 0.005f, 0.0f, 1.0f);
            } else if (!isAlphaType) {
                ImGui::Text("R: ");  ImGui::SameLine(); ImGui::ToggleButton("R", &state[0]);
            }

            if (numChannels > 1) {
                ImGui::SameLine();
                ImGui::Text("G: ");  ImGui::SameLine(); ImGui::ToggleButton("G", &state[1]);

                if (numChannels > 2) {
                    ImGui::SameLine();
                    ImGui::Text("B: ");  ImGui::SameLine(); ImGui::ToggleButton("B", &state[2]);
                }

                if (numChannels > 3 || isAlphaType)
                {
                    ImGui::SameLine();
                    ImGui::Text("A: ");  ImGui::SameLine(); ImGui::ToggleButton("A", &state[3]);
                }
            }
        }
        bool nonDefaultColours = data._isDepthTexture || !state[0] || !state[1] || !state[2] || !state[3];
        data._colourData.set(state[0] ? 1 : 0, state[1] ? 1 : 0, state[2] ? 1 : 0, state[3] ? 1 : 0);

        if (nonDefaultColours) {
            ImGui::GetWindowDrawList()->AddCallback(toggleColours, &data);
        }

        F32 aspect = 1.0f;
        if (preserveAspect) {
            U16 w = tex->getWidth();
            U16 h = tex->getHeight();
            aspect = w / to_F32(h);
        }

        static F32 zoom = 1.0f;
        static ImVec2 zoomCenter(0.5f, 0.5f);
        ImGui::ImageZoomAndPan((void *)(intptr_t)tex->getData().getHandle(), ImVec2(dimensions.w, dimensions.h / aspect), aspect, zoom, zoomCenter, 2, 3);

        if (nonDefaultColours) {
            ImGui::GetWindowDrawList()->AddCallback(toggleColours, &defaultData);
        }

        ImGui::Text("Mouse: Wheel = scroll | CTRL + Wheel = zoom | Hold Wheel Button = pan");

        if (ImGui::Button("Close")) {
            zoom = 1.0f;
            zoomCenter = ImVec2(0.5f, 0.5f);
            ImGui::CloseCurrentPopup();
            closed = true;
        }
        
        
        ImGui::EndPopup();
    }

    return closed;
}

void Editor::scenePreviewFocused(bool state) {
    _scenePreviewFocused = state;

    ImGuiIO& io = ImGui::GetIO();
    if (state) {
        io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;
    } else {
        io.ConfigFlags &= ~ImGuiConfigFlags_NavNoCaptureKeyboard;
    }
}
ImGuiContext& Editor::imguiContext() {
    return *_imguiContext;
}

ImGuiContext& Editor::imguizmoContext() {
    return _gizmo->getContext();
}

bool Editor::scenePreviewFocused() const {
    return _scenePreviewFocused;
}

void Editor::saveToXML() const {
    boost::property_tree::ptree pt;
    const stringImpl& editorPath = Paths::g_xmlDataLocation + Paths::Editor::g_saveLocation;
    const boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);

    pt.put("showMemEditor", _showMemoryEditor);
    pt.put("showSampleWindow", _showSampleWindow);
    createDirectory(editorPath.c_str());
    copyFile(editorPath, g_editorSaveFile, editorPath, g_editorSaveFile + stringImpl(".bak"), true);
    boost::property_tree::write_xml(editorPath + g_editorSaveFile, pt, std::locale(), settings);
}

void Editor::loadFromXML() {
    boost::property_tree::ptree pt;
    const stringImpl& editorPath = Paths::g_xmlDataLocation + Paths::Editor::g_saveLocation;
    if (!fileExists((editorPath + g_editorSaveFile).c_str())) {
        if (fileExists((editorPath + g_editorSaveFile + ".bak").c_str())) {
            copyFile(editorPath, g_editorSaveFile + stringImpl(".bak"), editorPath, g_editorSaveFile, true);
        }
    }

    if (fileExists((editorPath + g_editorSaveFile).c_str())) {
        boost::property_tree::read_xml(editorPath + g_editorSaveFile, pt);
        _showMemoryEditor = pt.get("showMemEditor", false);
        _showSampleWindow = pt.get("showSampleWindow", false);
    }
}
}; //namespace Divide
