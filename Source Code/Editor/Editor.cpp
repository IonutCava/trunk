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

#include <SDL/include/SDL_syswm.h>

namespace Divide {

namespace {
    bool show_another_window = false;
    I32 window_opacity = 255;
    I32 previous_window_opacity = 255;
    bool show_test_window = true;

    WindowManager* g_windowManager = nullptr;
    Editor* g_editor = nullptr;

    struct ImGuiViewportData
    {
        DisplayWindow*  _window = nullptr;
        bool            _windowOwned = false;
        Uint32          _windowID = 0;

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
      _showDebugWindow(false),
      _showSampleWindow(false),
      _gizmosVisible(false),
      _enableGizmo(false),
      _editorUpdateTimer(Time::ADD_TIMER("Editor Update Timer")),
      _editorRenderTimer(Time::ADD_TIMER("Editor Render Timer"))
{
    _imguiContext.fill(nullptr);

    _menuBar = std::make_unique<MenuBar>(context, true);

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
    if (_windowListeners.empty()) {
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

    IMGUI_CHECKVERSION();
    for (U8 i = 0; i < to_U8(Context::COUNT); ++i) {
        _imguiContext[i] = i == 0 ? ImGui::CreateContext() : ImGui::CreateContext(GetIO(0).Fonts);
        ImGui::SetCurrentContext(_imguiContext[i]);

        ImGuiIO& io = _imguiContext[i]->IO;
        if (i == 0) {
            U8* pPixels;
            I32 iWidth;
            I32 iHeight;
            io.Fonts->AddFontDefault();
            io.Fonts->GetTexDataAsRGBA32(&pPixels, &iWidth, &iHeight);

            SamplerDescriptor sampler = {};
            sampler._minFilter = TextureFilter::LINEAR;
            sampler._magFilter = TextureFilter::LINEAR;

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
        }

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

        if (_context.config().gui.imgui.multiViewportEnabled) {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
            //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
            //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
            //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

            //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
            io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
        }

        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;        // We can honor io.WantSetMousePos requests (optional, rarely used)
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;  // We can create multi-viewports on the Platform side (optional)
        
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

        vec2<U16> display_size = _mainWindow->getDrawableSize();
        io.DisplaySize = ImVec2((F32)_mainWindow->getDimensions().width, (F32)_mainWindow->getDimensions().height);
        io.DisplayFramebufferScale = ImVec2(io.DisplaySize.x > 0 ? ((F32)display_size.width / io.DisplaySize.x) : 0.f,
                                            io.DisplaySize.y > 0 ? ((F32)display_size.height / io.DisplaySize.y) : 0.f);

        if (_context.config().gui.imgui.multiViewportEnabled) {
            ImGuiPlatformIO& platform_io = _imguiContext[i]->PlatformIO;
            platform_io.Platform_CreateWindow = [](ImGuiViewport* viewport)
            {
                if (!g_windowManager) {
                    return;
                }

                ImGuiViewportData* data = IM_NEW(ImGuiViewportData)();
                viewport->PlatformUserData = data;

                WindowDescriptor descriptor = {};
                descriptor.title = "No Title Yet";
                descriptor.targetDisplay = g_windowManager->getWindow(0u).currentDisplayIndex();
                descriptor.clearColour.set(0.0f, 0.0f, 0.0f, 1.0f);
                descriptor.flags = to_U32(WindowDescriptor::Flags::HIDDEN) | to_U32(WindowDescriptor::Flags::CLEAR_COLOUR) | to_U32(WindowDescriptor::Flags::CLEAR_DEPTH);
                // We don't enable SDL_WINDOW_RESIZABLE because it enforce windows decorations
                descriptor.flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? 0 : to_U32(WindowDescriptor::Flags::DECORATED);
                descriptor.flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? 0 : to_U32(WindowDescriptor::Flags::RESIZEABLE);
                descriptor.flags |= (viewport->Flags & ImGuiViewportFlags_TopMost) ? to_U32(WindowDescriptor::Flags::ALWAYS_ON_TOP) : 0;
                descriptor.flags |= to_U32(WindowDescriptor::Flags::SHARE_CONTEXT);

                descriptor.dimensions.set(viewport->Size.x, viewport->Size.y);
                descriptor.position.set(viewport->Pos.x, viewport->Pos.y);

                ErrorCode err = ErrorCode::NO_ERR;
                U32 windowIndex = g_windowManager->createWindow(descriptor, err);
                data->_window = &g_windowManager->getWindow(windowIndex);
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
                data->_window->setDimensions(w, h);
            };

            platform_io.Platform_GetWindowSize = [](ImGuiViewport* viewport) -> ImVec2 {
                ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
                const vec2<U16>& dim = data->_window->getDimensions();
                return ImVec2((F32)dim.w, (F32)dim.h);
            };

            platform_io.Platform_SetWindowFocus = [](ImGuiViewport* viewport) {
                ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
                data->_window->hasFocus(true);
            };

            platform_io.Platform_GetWindowFocus = [](ImGuiViewport* viewport) -> bool {
                ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
                return data->_window->hasFocus();
            };

            platform_io.Platform_SetWindowTitle = [](ImGuiViewport* viewport, const char* title) {
                ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
                data->_window->title(title);
            };

            platform_io.Platform_RenderWindow = [](ImGuiViewport* viewport, void* /*render_arg*/) {
                if (!g_windowManager) {
                    return;
                }

                ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
                g_windowManager->prepareWindowForRender(*data->_window);
            };

            platform_io.Renderer_RenderWindow = [](ImGuiViewport* viewport, void* /*render_arg*/) {
                if (!g_editor) {
                    return;
                }
                
                g_editor->renderDrawList(viewport->DrawData, false, ((DisplayWindow*)viewport->PlatformHandle)->getGUID());
            };

            platform_io.Platform_SwapBuffers = [](ImGuiViewport* viewport, void*) {
                if (!g_windowManager) {
                    return;
                }

                ImGuiViewportData* data = (ImGuiViewportData*)viewport->PlatformUserData;
                g_windowManager->swapWindow(*data->_window);
            };

            ImGui_UpdateMonitors();

            ImGuiViewportData* data = IM_NEW(ImGuiViewportData)();
            data->_window = _mainWindow;
            data->_windowID = SDL_GetWindowID(_mainWindow->getRawWindow());
            data->_windowOwned = false;

            ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            main_viewport->PlatformUserData = data;
            main_viewport->PlatformHandle = _mainWindow;
        } else {
            ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            main_viewport->PlatformHandle = _mainWindow;
        }

        ImGui::ResetStyle(_currentTheme);
    }

    ImGui::SetCurrentContext(_imguiContext[to_base(Context::Editor)]);

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

    return true;
}

void Editor::close() {
    if (_mainWindow != nullptr) {
        for (U8 i = 0; i < to_base(WindowEvent::COUNT); ++i) {
            vector<I64>& guids = _windowListeners[i];
            for (I64 guid : guids) {
                _mainWindow->removeEventlistener(static_cast<WindowEvent>(i), guid);
            }
            guids.clear();
        }
    }
    _fontTexture.reset();
    _imguiProgram.reset();
    for (U8 i = 0; i < to_base(Context::COUNT); ++i) {
        if (_imguiContext[i] != nullptr) {
            ImGui::SetCurrentContext(_imguiContext[i]);

            ImGui::DestroyPlatformWindows();
            ImGui::DestroyContext(_imguiContext[i]);
            _imguiContext[i] = nullptr;
        }
    }
}

void Editor::toggle(const bool state) {
    _running = state;
    if (!state) {
        _scenePreviewFocused = _sceneHovered = false;
        ImGui::ResetStyle(_scenePreviewFocused ? _currentDimmedTheme : _currentTheme);
    } else {
        _enableGizmo = true;
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

    for (U8 i = 0; i < to_U8(Context::COUNT); ++i) {
        ImGuiIO& io = GetIO(i);
        io.DeltaTime = Time::MicrosecondsToSeconds<F32>(deltaTimeUS);

        if (!needInput()) {
            continue;
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
                renderDrawList(ImGui::GetDrawData(), true, _mainWindow->getGUID());

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
    _gizmosVisible = renderGizmos(evt._timeSinceLastFrameUS);
    return true;
}

bool Editor::framePostRenderStarted(const FrameEvent& evt) {
    Time::ScopedTimer timer(_editorRenderTimer);
    {
        ImGui::SetCurrentContext(_imguiContext[to_base(Context::Editor)]);
        IM_ASSERT(_imguiContext[to_base(Context::Editor)]->IO.Fonts->IsBuilt());
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

    g_windowManager->prepareWindowForRender(*_mainWindow);
    renderDrawList(ImGui::GetDrawData(), false, _mainWindow->getGUID());
    if (GetIO(0).ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    g_windowManager->prepareWindowForRender(*_mainWindow);

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

// Needs to be rendered immediately. *IM*GUI. IMGUI::NewFrame invalidates this data
void Editor::renderDrawList(ImDrawData* pDrawData, bool gizmo, I64 windowGUID)
{
    ImGui::SetCurrentContext(_imguiContext[to_base(gizmo ? Context::Gizmo : Context::Editor)]);

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
    beginDebugScopeCmd._scopeName = gizmo ? "Render IMGUI [Gizmo]" : "Render IMGUI [Full]";
    GFX::EnqueueCommand(buffer, beginDebugScopeCmd);

    if (gizmo) {
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

    if (gizmo) {
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

    if (node == nullptr) {
        _selectedNodes.resize(0);
    } else {
        _selectedNodes.push_back(node);
    }
}

/// Key pressed: return true if input was consumed
bool Editor::onKeyDown(const Input::KeyEvent& key) {
    ImGuiIO& io = GetIO(hasSceneFocus() ? to_U8(Context::Gizmo) : to_U8(Context::Editor));
    io.KeysDown[key._key] = true;
    if (key._text > 0) {
        io.AddInputCharacter(to_U16(key._text));
    }
    io.KeyCtrl = key._key == Input::KeyCode::KC_LCONTROL || key._key == Input::KeyCode::KC_RCONTROL;
    io.KeyShift = key._key == Input::KeyCode::KC_LSHIFT || key._key == Input::KeyCode::KC_RSHIFT;
    io.KeyAlt = key._key == Input::KeyCode::KC_LMENU || key._key == Input::KeyCode::KC_RMENU;
    io.KeySuper = false;

    return needInput() ? io.WantCaptureKeyboard : false;
}

/// Key released: return true if input was consumed
bool Editor::onKeyUp(const Input::KeyEvent& key) {
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

    return needInput() ? io.WantCaptureKeyboard : false;
}

namespace {
    ImGuiViewport* FindViewportByPlatformHandle(SDL_Window* platform_handle)
    {
        ImGuiContext& g = *GImGui;
        for (int i = 0; i != g.Viewports.Size; i++)
            if (((DisplayWindow*)g.Viewports[i]->PlatformHandle)->getRawWindow() == platform_handle)
                return g.Viewports[i];
        return NULL;
    }

}
/// Mouse moved: return true if input was consumed
bool Editor::mouseMoved(const Input::MouseEvent& arg) {
    if (!needInput()) {
        return false;
    }

    WindowManager& winMgr = context().app().windowManager();

    for (U8 i = 0; i < to_base(Context::COUNT); ++i) {
        ImGuiIO& io = GetIO(i);
        io.MouseHoveredViewport = 0;
        if (io.WantSetMousePos) {
            winMgr.setCursorPosition((I32)io.MousePos.x, (I32)io.MousePos.y, io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable);
        } else {
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
                io.MouseWheel += (F32)arg.Z(i == 1).rel / 60.0f;

                I32 mx = -1, my = -1;
                SDL_Window* focusedWindow = SDL_GetKeyboardFocus();
                if (focusedWindow) {
                    I32 wx = -1, wy = -1;
                    SDL_GetWindowPosition(focusedWindow, &wx, &wy);
                    SDL_GetGlobalMouseState(&mx, &my);
                    mx -= wx;
                    my -= wy;
                }
                if (ImGuiViewport* viewport = FindViewportByPlatformHandle((SDL_Window*)focusedWindow)) {
                    io.MousePos = ImVec2(viewport->Pos.x + (F32)mx, viewport->Pos.y + (F32)my);
                }
            } else {
                io.MousePos.x = (F32)arg.X(i == 1).abs;
                io.MousePos.y = (F32)arg.Y(i == 1).abs;
            }
            SDL_CaptureMouse(ImGui::IsAnyMouseDown() ? SDL_TRUE : SDL_FALSE);
        }
    }
    // Check if we are hovering over the scene

    ImGuiIO& io = GetIO(to_base(Context::Editor));
    SceneViewWindow* sceneView = static_cast<SceneViewWindow*>(_dockedWindows[to_base(WindowType::SceneView)]);
    _sceneHovered = sceneView->isHovered() && sceneView->sceneRect().contains(io.MousePos.x, io.MousePos.y);

    return !_scenePreviewFocused ? io.WantCaptureMouse : hasGizmoFocus();
}

/// Mouse button pressed: return true if input was consumed
bool Editor::mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button) {
    ACKNOWLEDGE_UNUSED(arg);

    if (!needInput()) {
        return false;
    }

    ImGui::SetCurrentContext(_imguiContext[hasSceneFocus() ? to_U8(Context::Gizmo) : to_U8(Context::Editor)]);
    ImGuiIO& io = ImGui::GetIO();

    if (button < 5) {
        io.MouseDown[button] = true;
    }
    //context().app().windowManager().captureMouse(true);

    return io.WantCaptureMouse || ImGuizmo::IsOver();
}

/// Mouse button released: return true if input was consumed
bool Editor::mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button) {
    ACKNOWLEDGE_UNUSED(arg);

    if (!needInput()) {
        return false;
    }

    if (_scenePreviewFocused != _sceneHovered) {
        ImGui::SetCurrentContext(_imguiContext[to_U8(Context::Editor)]);
        _scenePreviewFocused = _sceneHovered;
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::ResetStyle(_scenePreviewFocused ? _currentDimmedTheme : _currentTheme, style);

        const Rect<I32>& previewRect = static_cast<SceneViewWindow*>(_dockedWindows[to_base(WindowType::SceneView)])->sceneRect();
        _mainWindow->warp(_scenePreviewFocused, previewRect);
    }

    ImGui::SetCurrentContext(_imguiContext[hasSceneFocus() ? to_U8(Context::Gizmo) : to_U8(Context::Editor)]);
    ImGuiIO& io = ImGui::GetIO();

    if (button < 5) {
        io.MouseDown[button] = false;
    }
    //context().app().windowManager().captureMouse(false);



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
    if (!params.isWindowResize || _mainWindow == nullptr || params.winGUID != _mainWindow->getGUID()) {
        return;
    }

    vec2<U16> display_size = _mainWindow->getDrawableSize();

    for (U8 i = 0; i < to_U8(Context::COUNT); ++i) {
        ImGuiIO& io = GetIO(i);
        io.DisplaySize = ImVec2((F32)params.width, (F32)params.height);
        io.DisplayFramebufferScale = ImVec2(params.width > 0 ? ((F32)display_size.width / params.width) : 0.f,
                                            params.height > 0 ? ((F32)display_size.height / params.height) : 0.f);
    }
}

void Editor::OnSize(I32 iWidth, I32 iHeight) {
    ACKNOWLEDGE_UNUSED(iWidth);
    ACKNOWLEDGE_UNUSED(iHeight);
}

void Editor::OnUTF8(const char* text) {
    if (!needInput()) {
        return;
    }

    GetIO(hasSceneFocus() ? to_base(Context::Gizmo) : to_base(Context::Editor)).AddInputCharactersUTF8(text);
}

void Editor::setSelectedCamera(Camera* camera) {
    _selectedCamera = camera;
}

Camera* Editor::getSelectedCamera() const {
    return _selectedCamera;
}

void Editor::drawIMGUIDebug(const U64 deltaTime) {
    DisplayWindow& window = context().activeWindow();

    static F32 f = 0.0f;
    ImGui::Text("Hello, world!");
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    ImGui::SliderInt("Opacity", &window_opacity, 0, 255);

    FColour colour;
    if (ImGui::ColorEdit4("clear color", colour._v)) {
        window.clearColour(colour);
    }

    if (ImGui::Button("Test Window")) show_test_window ^= 1;
    if (ImGui::Button("Another Window")) show_another_window ^= 1;
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Time since last frame %.3f ms", Time::MicrosecondsToMilliseconds<F32>(deltaTime));
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

bool Editor::needInput() const {
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
