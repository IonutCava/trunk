#include "stdafx.h"

#include "Headers/PanelManager.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#ifndef STBI_INCLUDE_STB_IMAGE_H
#ifndef IMGUI_NO_STB_IMAGE_STATIC
#define STB_IMAGE_STATIC
#endif //IMGUI_NO_STB_IMAGE_STATIC
#ifndef IMGUI_NO_STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif //IMGUI_NO_STB_IMAGE_IMPLEMENTATION
#include "imgui/addons/imguibindings/stb_image.h"
#ifndef IMGUI_NO_STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_IMPLEMENTATION
#endif //IMGUI_NO_STB_IMAGE_IMPLEMENTATION
#ifndef IMGUI_NO_STB_IMAGE_STATIC
#undef STB_IMAGE_STATIC
#endif //IMGUI_NO_STB_IMAGE_STATIC
#endif //STBI_INCLUDE_STB_IMAGE_H

namespace Divide {
    static PanelManager* g_panelManager = nullptr;
    static void DrawDockedWindows(ImGui::PanelManagerWindowData& wd) {
        if (g_panelManager != nullptr) {
            g_panelManager->drawDockedWindows(wd);
        }
    }

    namespace {
        static const char* DockedWindowNames[] = { "Solution Explorer","Toolbox","Property Window","Find Window","Output Window","Application Output","Preferences" };
        static const char* ToggleWindowNames[] = { "Toggle Window 1","Toggle Window 2","Toggle Window 3","Toggle Window 4" };

        ImGui::TabWindow tabWindows[5]; // 0 = center, 1 = left, 2 = right, 3 = top, 4 = bottom
                                                                            // These callbacks are not ImGui::TabWindow callbacks, but ImGui::PanelManager callbacks, set in AddTabWindowIfSupported(...) right below
        static void DrawDockedTabWindows(ImGui::PanelManagerWindowData& wd) {
            // See more generic DrawDockedWindows(...) below...
            ImGui::TabWindow& tabWindow = tabWindows[(wd.dockPos == ImGui::PanelManager::LEFT) ? 1 :
                (wd.dockPos == ImGui::PanelManager::RIGHT) ? 2 :
                (wd.dockPos == ImGui::PanelManager::TOP) ? 3
                : 4];
            tabWindow.render();
        }
        static void ShowExampleMenuFile()
        {
            ImGui::MenuItem("(dummy menu)", NULL, false, false);
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            if (ImGui::BeginMenu("Open Recent"))
            {
                ImGui::MenuItem("fish_hat.c");
                ImGui::MenuItem("fish_hat.inl");
                ImGui::MenuItem("fish_hat.h");
                if (ImGui::BeginMenu("More.."))
                {
                    ImGui::MenuItem("Hello");
                    ImGui::MenuItem("Sailor");
                    if (ImGui::BeginMenu("Recurse.."))
                    {
                        ShowExampleMenuFile();
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {}
            if (ImGui::MenuItem("Save As..")) {}
            ImGui::Separator();
            if (ImGui::BeginMenu("Options"))
            {
                static bool enabled = true;
                ImGui::MenuItem("Enabled", "", &enabled);
                ImGui::BeginChild("child", ImVec2(0, 60), true);
                for (int i = 0; i < 10; i++)
                    ImGui::Text("Scrolling Text %d", i);
                ImGui::EndChild();
                static float f = 0.5f;
                ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
                ImGui::InputFloat("Input", &f, 0.1f);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Colors"))
            {
                for (int i = 0; i < ImGuiCol_COUNT; i++)
                    ImGui::MenuItem(ImGui::GetStyleColorName((ImGuiCol)i));
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Disabled", false)) // Disabled
            {
                IM_ASSERT(0);
            }
            if (ImGui::MenuItem("Checked", NULL, true)) {}
            if (ImGui::MenuItem("Quit", "Alt+F4")) {}
        }
        static void ShowExampleMenuBar(bool isMainMenu = false)
        {
            //static bool ids[2];
            //ImGui::PushID((const void*) isMainMenu ? &ids[0] : &ids[1]);
            const bool open = isMainMenu ? ImGui::BeginMainMenuBar() : ImGui::BeginMenuBar();
            if (open)
            {
                if (ImGui::BeginMenu("File"))
                {
                    ShowExampleMenuFile();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit"))
                {
                    if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
                    if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
                    ImGui::Separator();
                    if (ImGui::MenuItem("Cut", "CTRL+X")) {}
                    if (ImGui::MenuItem("Copy", "CTRL+C")) {}
                    if (ImGui::MenuItem("Paste", "CTRL+V")) {}
                    ImGui::Separator();

                    // Just a test to check/uncheck multiple checkItems without closing the Menu (with the RMB)
                    static bool booleanProps[3] = { true,false,true };
                    static const char* names[3] = { "Boolean Test 1","Boolean Test 2","Boolean Test 3" };
                    const bool isRightMouseButtonClicked = ImGui::IsMouseClicked(1);    // cached
                    for (int i = 0;i<3;i++) {
                        ImGui::MenuItem(names[i], NULL, &booleanProps[i]);
                        if (isRightMouseButtonClicked && ImGui::IsItemHovered()) booleanProps[i] = !booleanProps[i];
                    }

                    ImGui::EndMenu();
                }
                if (isMainMenu) {
                    //gMainMenuBarSize = ImGui::GetWindowSize();
                    ImGui::EndMainMenuBar();
                } else ImGui::EndMenuBar();
            }
            //ImGui::PopID();
        }
        void AddTabWindowIfSupported(ImGui::PanelManagerPane* pane) {
            const ImTextureID texId = ImGui::TabWindow::DockPanelIconTextureID;
            IM_ASSERT(pane && texId);
            ImVec2 buttonSize(32, 32);
            static const char* names[4] = { "TabWindow Left","TabWindow Right","TabWindow Top", "TabWindow Bottom" };
            const int index = (pane->pos == ImGui::PanelManager::LEFT) ? 0 : (pane->pos == ImGui::PanelManager::RIGHT) ? 1 : (pane->pos == ImGui::PanelManager::TOP) ? 2 : 3;
            if (index<2) buttonSize.x = 24;
            const int uvIndex = (index == 0) ? 3 : (index == 2) ? 0 : (index == 3) ? 2 : index;
            ImVec2 uv0(0.75f, (float)uvIndex*0.25f), uv1(uv0.x + 0.25f, uv0.y + 0.25f);
            pane->addButtonAndWindow(ImGui::Toolbutton(names[index], texId, uv0, uv1, buttonSize),              // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                     ImGui::PanelManagerPaneAssociatedWindow(names[index], -1, &DrawDockedTabWindows, NULL, ImGuiWindowFlags_NoScrollbar));    //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
        }

        ImTextureID LoadTextureFromMemory(ResourceCache& cache, Texture_ptr& target, const stringImpl& name, const unsigned char* filenameInMemory, int filenameInMemorySize, int req_comp = 0) {
            int w, h, n;
            unsigned char* pixels = stbi_load_from_memory(filenameInMemory, filenameInMemorySize, &w, &h, &n, req_comp);
            if (!pixels) {
                return 0;
            }
            if (req_comp>0 && req_comp <= 4) n = req_comp;

            ImTextureID texId = NULL;

            SamplerDescriptor sampler;
            sampler.setFilters(TextureFilter::LINEAR);

            TextureDescriptor descriptor(TextureType::TEXTURE_2D,
                                         GFXImageFormat::RGBA8,
                                         GFXDataFormat::UNSIGNED_BYTE);
            descriptor.setSampler(sampler);

            ResourceDescriptor textureDescriptor(name);

            textureDescriptor.setThreadedLoading(false);
            textureDescriptor.setFlag(true);
            textureDescriptor.setPropertyDescriptor(descriptor);

            target = CreateResource<Texture>(cache, textureDescriptor);
            assert(target);

            Texture::TextureLoadInfo info;

            target->loadData(info, (bufferPtr)pixels, vec2<U16>(w, h));

            texId = reinterpret_cast<void*>((intptr_t)target->getHandle());

            stbi_image_free(pixels);

            return texId;
        }

        // Static methods to load/save all the tabWindows (done mainly to avoid spreading too many definitions around)
        static const char tabWindowsSaveName[] = "myTabWindow.layout";
        static const char tabWindowsSaveNamePersistent[] = "/persistent_folder/myTabWindow.layout";  // Used by emscripten only, and only if YES_IMGUIEMSCRIPTENPERSISTENTFOLDER is defined (and furthermore it's buggy...).
        static bool LoadTabWindowsIfSupported() {
            bool loadedFromFile = false;
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_LOAD))
            const char* pSaveName = tabWindowsSaveName;
#   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
            if (ImGuiHelper::FileExists(tabWindowsSaveNamePersistent)) pSaveName = tabWindowsSaveNamePersistent;
#   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
            //loadedFromFile = tabWindow.load(pSaveName);   // This is good for a single TabWindow
            loadedFromFile = ImGui::TabWindow::Load(pSaveName, &tabWindows[0], sizeof(tabWindows) / sizeof(tabWindows[0]));  // This is OK for a multiple TabWindows
#   endif //!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && ...
            return loadedFromFile;
        }
        static bool SaveTabWindowsIfSupported() {
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_SAVE))
            const char* pSaveName = tabWindowsSaveName;
#   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
            pSaveName = tabWindowsSaveNamePersistent;
#   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
            //if (parent.save(pSaveName))   // This is OK for a single TabWindow
            if (ImGui::TabWindow::Save(pSaveName, &tabWindows[0], sizeof(tabWindows) / sizeof(tabWindows[0])))  // This is OK for a multiple TabWindows
            {
#   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                ImGui::EmscriptenFileSystemHelper::Sync();
#   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                return true;
            }
#   endif //!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && ...
            return false;
        }

        // Here are refactored the load / save methods of the ImGui::PanelManager(mainly the hover and docked sizes of all windows and the button states of the 4 toolbars)
        // Please note that it should be possible to save settings this in the same as above ("myTabWindow.layout"), but the API is more complex.
        static const char panelManagerSaveName[] = "myPanelManager.layout";
        static const char panelManagerSaveNamePersistent[] = "/persistent_folder/myPanelManager.layout";  // Used by emscripten only, and only if YES_IMGUIEMSCRIPTENPERSISTENTFOLDER is defined (and furthermore it's buggy...).
        static bool LoadPanelManagerIfSupported(ImGui::PanelManager& mgr) {
            bool loadingOk = false;
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_LOAD))
            const char* pSaveName = panelManagerSaveName;
#   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
            if (ImGuiHelper::FileExists(panelManagerSaveNamePersistent)) pSaveName = panelManagerSaveNamePersistent;
#   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
            loadingOk = ImGui::PanelManager::Load(mgr, pSaveName);
#   endif //!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !...
            return loadingOk;
        }
        static bool SavePanelManagerIfSupported(ImGui::PanelManager& mgr) {
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_SAVE))
            const char* pSaveName = panelManagerSaveName;
#   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
            pSaveName = panelManagerSaveNamePersistent;
#   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
            if (ImGui::PanelManager::Save(mgr, pSaveName)) {
#	ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                ImGui::EmscriptenFileSystemHelper::Sync();
#	endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                return true;
            }
#   endif //!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !...
            return false;
        }
    };

    PanelManager::PanelManager(PlatformContext& context)
        : PlatformContextComponent(context),
          _showMainMenuBar(nullptr),
          _showCentralWindow(nullptr),
          _manager(std::make_unique<ImGui::PanelManager>())
    {
        g_panelManager = this;
    }

    PanelManager::~PanelManager()
    {

    }

    void PanelManager::draw() {
        if (_showMainMenuBar && *_showMainMenuBar) ShowExampleMenuBar(true);

        if (_showCentralWindow && *_showCentralWindow) {
            const ImVec2& iqs = _manager->getCentralQuadSize();
            if (iqs.x>ImGui::GetStyle().WindowMinSize.x && iqs.y>ImGui::GetStyle().WindowMinSize.y) {
                ImGui::SetNextWindowPos(_manager->getCentralQuadPosition());
                ImGui::SetNextWindowSize(_manager->getCentralQuadSize());
                if (ImGui::Begin("Central Window", NULL, ImVec2(0, 0), _manager->getDockedWindowsAlpha(), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | _manager->getDockedWindowsExtraFlags() /*| ImGuiWindowFlags_NoBringToFrontOnFocus*/)) {
#                   ifndef NO_IMGUITABWINDOW
                    tabWindows[0].render(); // Must be called inside "its" window (and sets isInited() to false). [ChildWindows can't be used here (but they can be used inside Tab Pages). Basically all the "Central Window" must be given to 'tabWindow'.]
#                   else // NO_IMGUITABWINDOW
                    ImGui::Text("Example central window");
#                   endif // NO_IMGUITABWINDOW
                }
                ImGui::End();
            }
        }

        // Here we render mgr (our ImGui::PanelManager)
        ImGui::PanelManagerPane* pressedPane = NULL;  // Optional
        int pressedPaneButtonIndex = -1;            // Optional
        if (_manager->render(&pressedPane, &pressedPaneButtonIndex)) {
            //const ImVec2& iqp = mgr.getCentralQuadPosition();
            //const ImVec2& iqs = mgr.getCentralQuadSize();
        }

        // (Optional) Some manual feedback to the user (actually I detect gpShowMainMenuBar pressures here too, but pleese read below...):
        if (pressedPane && pressedPaneButtonIndex != -1)
        {
            static const char* paneNames[] = { "LEFT","RIGHT","TOP","BOTTOM" };
            if (!pressedPane->getWindowName(pressedPaneButtonIndex)) {
                ImGui::Toolbutton* pButton = NULL;
                pressedPane->getButtonAndWindow(pressedPaneButtonIndex, &pButton);
                if (pButton->isToggleButton) {
                    if (pressedPane->pos == ImGui::PanelManager::TOP && pressedPaneButtonIndex == (int)pressedPane->getSize() - 2) {
                        // For this we could have just checked if *gpShowMainMenuBar had changed its value before and after mgr.render()...
                        setPanelManagerBoundsToIncludeMainMenuIfPresent();
                    }
                } 
            } else {
                ImGui::Toolbutton* pButton = NULL;
                pressedPane->getButtonAndWindow(pressedPaneButtonIndex, &pButton);
            }

        }
    }

    void PanelManager::init() {
        ResourceCache& cache = context().gfx().parent().resourceCache();
        if (!ImGui::TabWindow::DockPanelIconTextureID) {
            int dockPanelImageBufferSize = 0;
            const unsigned char* dockPanelImageBuffer = ImGui::TabWindow::GetDockPanelIconImagePng(&dockPanelImageBufferSize);
            ImGui::TabWindow::DockPanelIconTextureID = 
                LoadTextureFromMemory(cache,
                                      _textures[to_base(TextureUsage::Dock)],
                                      "Docking Texture",
                                      dockPanelImageBuffer,
                                      dockPanelImageBufferSize);
        }

        // Here we setup mgr (our ImGui::PanelManager)
        if (_manager->isEmpty()) {
            SamplerDescriptor sampler;
            sampler.setMinFilter(TextureFilter::NEAREST);
            sampler.setAnisotropy(0);
            sampler.setWrapMode(TextureWrap::CLAMP);
            // splash shader doesn't do gamma correction since that's a post processing step
            // so fake gamma correction by loading an sRGB image as a linear one.
            sampler.toggleSRGBColourSpace(false);

            TextureDescriptor descriptor(TextureType::TEXTURE_2D);
            descriptor.setSampler(sampler);

            ResourceDescriptor texture("Panel Manager Texture 1");

            texture.setThreadedLoading(false);
            texture.setFlag(true);
            texture.setResourceName("Tile8x8.png");
            texture.setResourceLocation(Paths::g_assetsLocation + Paths::g_GUILocation);
            texture.setPropertyDescriptor(descriptor);

            _textures[to_base(TextureUsage::Tile)] = CreateResource<Texture>(cache, texture);

            texture.setName("Panel Manager Texture 2");
            texture.setResourceName("myNumbersTexture.png");
            _textures[to_base(TextureUsage::Numbers)] = CreateResource<Texture>(cache, texture);

            // Hp) All the associated windows MUST have an unique name WITHOUT using the '##' chars that ImGui supports
            void* myImageTextureVoid = reinterpret_cast<void*>((intptr_t)_textures[to_base(TextureUsage::Tile)]->getHandle());         // 8x8 tiles
            void* myImageTextureVoid2 = reinterpret_cast<void*>((intptr_t)_textures[to_base(TextureUsage::Numbers)]->getHandle());       // 3x3 tiles
            ImVec2 uv0(0, 0), uv1(0, 0);int tileNumber = 0;

            // LEFT PANE
            {
                ImGui::PanelManager::Pane* pane = _manager->addPane(ImGui::PanelManager::LEFT, "myFirstToolbarLeft##foo");
                if (pane) {
                    // Here we add the "proper" docked buttons and windows:
                    const ImVec2 buttonSize(24, 32);
                    for (int i = 0;i < 3;i++) {
                        // Add to left pane the first 3 windows DrawDockedWindows[i], with Toolbuttons with the first 3 images of myImageTextureVoid (8x8 tiles):
                        tileNumber = i;uv0 = ImVec2((float)(tileNumber % 8) / 8.f, (float)(tileNumber / 8) / 8.f);uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                        pane->addButtonAndWindow(ImGui::Toolbutton(DockedWindowNames[i], myImageTextureVoid, uv0, uv1, buttonSize),         // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                                 ImGui::PanelManagerPaneAssociatedWindow(DockedWindowNames[i], -1, &DrawDockedWindows));  //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
                    }
                    AddTabWindowIfSupported(pane);
                    pane->addSeparator(48); // Note that a separator "eats" one toolbutton index as if it was a real button

                                            // Here we add two "automatic" toggle buttons (i.e. toolbuttons + associated windows): only the last args of Toolbutton change.
                    const ImVec2 toggleButtonSize(24, 24);
                    tileNumber = 0;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonAndWindow(ImGui::Toolbutton(ToggleWindowNames[0], myImageTextureVoid2, uv0, uv1, toggleButtonSize, true, false),        // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                             ImGui::PanelManagerPaneAssociatedWindow(ToggleWindowNames[0], -1, &DrawDockedWindows));              //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
                    tileNumber = 1;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonAndWindow(ImGui::Toolbutton(ToggleWindowNames[1], myImageTextureVoid2, uv0, uv1, toggleButtonSize, true, false),        // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                             ImGui::PanelManagerPaneAssociatedWindow(ToggleWindowNames[1], -1, &DrawDockedWindows));              //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
                    pane->addSeparator(48); // Note that a separator "eats" one toolbutton index as if it was a real button


                                            // Here we add two "manual" toggle buttons (i.e. toolbuttons only):
                    const ImVec2 extraButtonSize(24, 24);
                    tileNumber = 0;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 1", myImageTextureVoid2, uv0, uv1, extraButtonSize, true, false));
                    tileNumber = 1;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 2", myImageTextureVoid2, uv0, uv1, extraButtonSize, true, false));

                    // Optional line that affects the look of the Toolbutton in this pane: NOPE: we'll override them later for all the panes
                    //pane->setDisplayProperties(ImVec2(0.25f,0.9f),ImVec4(0.85,0.85,0.85,1));

                    // Optional line to manually specify alignment as ImVec2 (by default Y alignment is 0.0f for LEFT and RIGHT panes, and X alignment is 0.5f for TOP and BOTTOM panes)
                    // Be warned that the component that you don't use (X in this case, must be set to 0.f for LEFT or 1.0f for RIGHT, unless you want to do strange things)
                    //pane->setToolbarProperties(true,false,ImVec2(0.f,0.5f));  // place this pane at Y center (instead of Y top)
                }
            }
            // RIGHT PANE
            {
                ImGui::PanelManager::Pane* pane = _manager->addPane(ImGui::PanelManager::RIGHT, "myFirstToolbarRight##foo");
                if (pane) {
                    // Here we use (a part of) the left pane to clone windows (handy since we don't support drag and drop):
                    if (_manager->getPaneLeft()) pane->addClonedPane(*_manager->getPaneLeft(), false, 0, 2); // note that only the "docked" part of buttons/windows are clonable ("manual" buttons are simply ignored): TO FIX: for now please avoid leaving -1 as the last argument, as this seems to mess up button indices: just explicitely copy NonTogglable-DockButtons yourself.
                                                                                                 // To clone single buttons (and not the whole pane) please use: pane->addClonedButtonAndWindow(...);
                                                                                                 // IMPORTANT: Toggle Toolbuttons (and associated windows) can't be cloned and are just skipped if present
                    AddTabWindowIfSupported(pane);

                    // here we could add new docked windows as well in the usual way now... but we don't
                    pane->addSeparator(48);   // Note that a separator "eats" one toolbutton index as if it was a real button

                                              // Here we add two other "manual" toggle buttons:
                    tileNumber = 2;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 3", myImageTextureVoid2, uv0, uv1, ImVec2(24, 32), true, false));
                    tileNumber = 3;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 4", myImageTextureVoid2, uv0, uv1, ImVec2(24, 32), true, false));

                    // Here we add two "manual" normal buttons (actually "normal" buttons are always "manual"):
                    pane->addSeparator(48);   // Note that a separator "eats" one toolbutton index as if it was a real button
                    tileNumber = 4;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Manual normal button 1", myImageTextureVoid2, uv0, uv1, ImVec2(24, 32), false, false));
                    tileNumber = 5;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 2", myImageTextureVoid2, uv0, uv1, ImVec2(24, 32), false, false));

                }
            }
            // BOTTOM PANE
            {
                ImGui::PanelManager::Pane* pane = _manager->addPane(ImGui::PanelManager::BOTTOM, "myFirstToolbarBottom##foo");
                if (pane) {
                    // Here we add the "proper" docked buttons and windows:
                    const ImVec2 buttonSize(32, 32);
                    for (int i = 3;i < 6;i++) {
                        // Add to left pane the windows DrawDockedWindows[i] from 3 to 6, with Toolbuttons with the images from 3 to 6 of myImageTextureVoid (8x8 tiles):
                        tileNumber = i;uv0 = ImVec2((float)(tileNumber % 8) / 8.f, (float)(tileNumber / 8) / 8.f);uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                        pane->addButtonAndWindow(ImGui::Toolbutton(DockedWindowNames[i], myImageTextureVoid, uv0, uv1, buttonSize),         // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                                 ImGui::PanelManagerPaneAssociatedWindow(DockedWindowNames[i], -1, &DrawDockedWindows));  //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
                    }
                    AddTabWindowIfSupported(pane);
                    pane->addSeparator(64); // Note that a separator "eats" one toolbutton index as if it was a real button

                                            // Here we add two "automatic" toggle buttons (i.e. toolbuttons + associated windows): only the last args of Toolbutton change.
                    const ImVec2 toggleButtonSize(32, 32);
                    tileNumber = 2;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonAndWindow(ImGui::Toolbutton(ToggleWindowNames[2], myImageTextureVoid2, uv0, uv1, toggleButtonSize, true, false),        // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                             ImGui::PanelManagerPaneAssociatedWindow(ToggleWindowNames[2], -1, &DrawDockedWindows));              //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
                    tileNumber = 3;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonAndWindow(ImGui::Toolbutton(ToggleWindowNames[3], myImageTextureVoid2, uv0, uv1, toggleButtonSize, true, false),        // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                             ImGui::PanelManagerPaneAssociatedWindow(ToggleWindowNames[3], -1, &DrawDockedWindows));              //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
                    pane->addSeparator(64); // Note that a separator "eats" one toolbutton index as if it was a real button

                                            // Here we add two "manual" toggle buttons:
                    const ImVec2 extraButtonSize(32, 32);
                    tileNumber = 4;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 4", myImageTextureVoid2, uv0, uv1, extraButtonSize, true, false));
                    tileNumber = 5;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 5", myImageTextureVoid2, uv0, uv1, extraButtonSize, true, false));

                }
            }
            // TOP PANE
            {
                // Here we create a top pane.
                ImGui::PanelManager::Pane* pane = _manager->addPane(ImGui::PanelManager::TOP, "myFirstToolbarTop##foo");
                if (pane) {
                    // Here we add the "proper" docked buttons and windows:
                    const ImVec2 buttonSize(32, 32);
                    for (int i = 6;i < 7;i++) {
                        // Add to left pane the windows DrawDockedWindows[i] from 3 to 6, with Toolbuttons with the images from 3 to 6 of myImageTextureVoid (8x8 tiles):
                        tileNumber = i;uv0 = ImVec2((float)(tileNumber % 8) / 8.f, (float)(tileNumber / 8) / 8.f);uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                        pane->addButtonAndWindow(ImGui::Toolbutton(DockedWindowNames[i], myImageTextureVoid, uv0, uv1, buttonSize),         // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                                 ImGui::PanelManagerPaneAssociatedWindow(DockedWindowNames[i], -1, &DrawDockedWindows));  //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
                    }
                    AddTabWindowIfSupported(pane);
                    pane->addSeparator(64); // Note that a separator "eats" one toolbutton index as if it was a real button

                    const ImVec2 extraButtonSize(32, 32);
                    pane->addButtonOnly(ImGui::Toolbutton("Normal Manual Button 1", myImageTextureVoid2, ImVec2(0, 0), ImVec2(1.f / 3.f, 1.f / 3.f), extraButtonSize));//,false,false,ImVec4(0,1,0,1)));  // Here we add a free button
                    tileNumber = 1;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Normal Manual Button 2", myImageTextureVoid2, uv0, uv1, extraButtonSize));  // Here we add a free button
                    tileNumber = 2;uv0 = ImVec2((float)(tileNumber % 3) / 3.f, (float)(tileNumber / 3) / 3.f);uv1 = ImVec2(uv0.x + 1.f / 3.f, uv0.y + 1.f / 3.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Normal Manual Button 3", myImageTextureVoid2, uv0, uv1, extraButtonSize));  // Here we add a free button
                    pane->addSeparator(32);  // Note that a separator "eats" one toolbutton index as if it was a real button

                                             // Here we add two manual toggle buttons, but we'll use them later to show/hide menu and show/hide a central window
                    const ImVec2 toggleButtonSize(32, 32);
                    tileNumber = 51;uv0 = ImVec2((float)(tileNumber % 8) / 8.f, (float)(tileNumber / 8) / 8.f);uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Show/Hide Main Menu Bar", myImageTextureVoid, uv0, uv1, toggleButtonSize, true, true));  // [*] Here we add a manual toggle button we'll simply bind to "gpShowMainMenuBar" later. Start value is last arg.
                    tileNumber = 5;uv0 = ImVec2((float)(tileNumber % 8) / 8.f, (float)(tileNumber / 8) / 8.f);uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                    pane->addButtonOnly(ImGui::Toolbutton("Show/Hide central window", myImageTextureVoid, uv0, uv1, toggleButtonSize, true, true));  // [**] Here we add a manual toggle button we'll process later [**]

                                                                                                                                                     // Ok. Now all the buttons/windows have been added to the TOP Pane.
                                                                                                                                                     // We can safely bind our bool pointers without any risk (ImVector reallocations could have invalidated them).
                                                                                                                                                     // Please note that it's not safe to add EVERYTHING (even separators) to this (TOP) pane afterwards (unless we bind the booleans again).
                    _showMainMenuBar = &pane->bar.getButton(pane->getSize() - 2)->isDown;            // [*]
                    _showCentralWindow = &pane->bar.getButton(pane->getSize() - 1)->isDown;          // [**]

                }
            }

            // Optional. Loads the layout (just selectedButtons, docked windows sizes and stuff like that)
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_LOAD))
            LoadPanelManagerIfSupported(*_manager);
#   endif //!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !...

            // Optional line that affects the look of all the Toolbuttons in the Panes inserted so far:
            _manager->overrideAllExistingPanesDisplayProperties(ImVec2(0.25f, 0.9f), ImVec4(0.85, 0.85, 0.85, 1));

            // These line is only necessary to accomodate space for the global menu bar we're using:
            setPanelManagerBoundsToIncludeMainMenuIfPresent();
        }
    }

    void PanelManager::destroy() {

    }


    // Here are two static methods useful to handle the change of size of the togglable mainMenu we will use
    // Returns the height of the main menu based on the current font (from: ImGui::CalcMainMenuHeight() in imguihelper.h)
    float PanelManager::calcMainMenuHeight() {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        ImFont* font = ImGui::GetFont();
        if (!font) {
            if (io.Fonts->Fonts.size()>0) font = io.Fonts->Fonts[0];
            else return (14) + style.FramePadding.y * 2.0f;
        }
        return (io.FontGlobalScale * font->Scale * font->FontSize) + style.FramePadding.y * 2.0f;
    }

    void  PanelManager::setPanelManagerBoundsToIncludeMainMenuIfPresent(int displayX, int displayY) {
        if (_showMainMenuBar) {
            if (displayX <= 0) displayX = ImGui::GetIO().DisplaySize.x;
            if (displayY <= 0) displayY = ImGui::GetIO().DisplaySize.y;
            ImVec4 bounds(0, 0, (float)displayX, (float)displayY);   // (0,0,-1,-1) defaults to (0,0,io.DisplaySize.x,io.DisplaySize.y)
            if (*_showMainMenuBar) {
                const float mainMenuHeight = calcMainMenuHeight();
                bounds = ImVec4(0, mainMenuHeight, displayX, displayY - mainMenuHeight);
            }
            _manager->setDisplayPortion(bounds);
        }
    }

    void PanelManager::resize(int w, int h) {
        static ImVec2 initialSize(w, h);
        _manager->setToolbarsScaling((float)w / initialSize.x, (float)h / initialSize.y);  // Scales the PanelManager bounmds based on the initialSize
        setPanelManagerBoundsToIncludeMainMenuIfPresent(w, h);                  // This line is only necessary if we have a global menu bar
    }


    void PanelManager::drawDockedWindows(ImGui::PanelManagerWindowData& wd) {
        if (!wd.isToggleWindow) {
            // Here we simply draw all the docked windows (in our case DockedWindowNames) without using ImGui::Begin()/ImGui::End().
            // (This is necessary because docked windows are not normal windows: see the title bar for example)
            if (strcmp(wd.name, DockedWindowNames[0]) == 0) {
                // Draw Solution Explorer
                ImGui::Text("%s\n", wd.name);
                static float f;
                ImGui::Text("Hello, world!");
                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
                //show_test_window ^= ImGui::Button("Test Window");
                //show_another_window ^= ImGui::Button("Another Window");

                // Calculate and show framerate
                static float ms_per_frame[120] = { 0 };
                static int ms_per_frame_idx = 0;
                static float ms_per_frame_accum = 0.0f;
                ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
                ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
                ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];
                ms_per_frame_idx = (ms_per_frame_idx + 1) % 120;
                const float ms_per_frame_avg = ms_per_frame_accum / 120;
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_per_frame_avg, 1000.0f / ms_per_frame_avg);
            } else if (strcmp(wd.name, DockedWindowNames[1]) == 0) {
                // Draw Toolbox
                ImGui::Text("%s\n", wd.name);
            } else if (strcmp(wd.name, DockedWindowNames[2]) == 0) {
                ImGui::Text("%s\n", wd.name);
#           ifdef TEST_ICONS_INSIDE_TTF
                ImGui::Spacing();ImGui::Separator();
                ImGui::TextDisabled("Testing icons inside FontAwesome");
                ImGui::Separator();ImGui::Spacing();

                ImGui::Button(ICON_FA_FILE "  File"); // use string literal concatenation, ouputs a file icon and File
#           ifndef NO_IMGUIFILESYSTEM // Testing icons inside ImGuiFs::Dialog
                ImGui::AlignFirstTextHeightToWidgets();ImGui::Text("File:");ImGui::SameLine();
                static ImGuiFs::Dialog dlg;
                ImGui::InputText("###fsdlg", (char*)dlg.getChosenPath(), ImGuiFs::MAX_PATH_BYTES, ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                const bool browseButtonPressed = ImGui::Button("...##fsdlg");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "file chooser dialog\nwith FontAwesome icons");
                dlg.chooseFileDialog(browseButtonPressed, dlg.getLastDirectory());
#           endif //NO_IMGUIFILESYSTEM
                if (ImGui::TreeNode("All FontAwesome Icons")) {
                    DrawAllFontAwesomeIcons();
                    ImGui::TreePop();
                }

                ImGui::Spacing();ImGui::Separator();
                ImGui::TextDisabled("Testing styled check boxes by dougbinks");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9\nThey work better with a Monospace font");
                ImGui::Separator();ImGui::Spacing();

                static bool cb1 = false;ImGui::CheckBoxFont("CheckBoxFont", &cb1);
                static bool cb2 = false;ImGui::CheckBoxTick("CheckBoxTick", &cb2);
                static bool cb3 = false;ImGui::MenuItemCheckBox("MenuItemCheckBox", &cb3);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Probably this works only\ninside a Menu...");


                // We, we can use default fonts as well (std UTF8 chars):
                ImGui::Spacing();ImGui::Separator();
                ImGui::TextDisabled("Testing the same without any icon font");
                ImGui::Separator();ImGui::Spacing();

                static bool cb4 = false;ImGui::CheckBoxFont("CheckBoxDefaultFonts", &cb4, "▣", "□");
                static bool cb5 = false;ImGui::CheckBoxFont("CheckBoxDefaultFonts2", &cb5, "■", "□");
                static bool cb6 = false;ImGui::CheckBoxFont("CheckBoxDefaultFonts3", &cb6, "▼", "▶");
                static bool cb7 = false;ImGui::CheckBoxFont("CheckBoxDefaultFonts4", &cb7, "▽", "▷");
#           endif //TEST_ICONS_INSIDE_TTF
            } else if (strcmp(wd.name, DockedWindowNames[3]) == 0) {
                // Draw Find Window
                ImGui::Text("%s\n", wd.name);
            } else if (strcmp(wd.name, DockedWindowNames[4]) == 0) {
                // Draw Output Window
                ImGui::Text("%s\n", wd.name);
            } else if (strcmp(wd.name, "Preferences") == 0) {
                ImGui::DragFloat("Window Alpha##WA1", &_manager->getDockedWindowsAlpha(), 0.005f, -0.01f, 1.0f, _manager->getDockedWindowsAlpha() < 0.0f ? "(default)" : "%.3f");
                bool border = _manager->getDockedWindowsBorder();
                if (ImGui::Checkbox("Window Borders", &border)) _manager->setDockedWindowsBorder(border); // Sets the window border to all the docked windows
                ImGui::SameLine();
                bool noTitleBar = _manager->getDockedWindowsNoTitleBar();
                if (ImGui::Checkbox("No Window TitleBars", &noTitleBar)) _manager->setDockedWindowsNoTitleBar(noTitleBar);
                if (_showCentralWindow) { ImGui::SameLine();ImGui::Checkbox("Show Central Wndow", (bool*)_showCentralWindow); }
                if (_showMainMenuBar) {
                    ImGui::SameLine();
                    if (ImGui::Checkbox("Show Main Menu", (bool*)_showMainMenuBar)) setPanelManagerBoundsToIncludeMainMenuIfPresent();
                }
                // Here we test saving/loading the ImGui::PanelManager layout (= the sizes of the 4 docked windows and the buttons that are selected on the 4 toolbars)
                // Please note that the API should allow loading/saving different items into a single file and loading/saving from/to memory too, but we don't show it now.
#           if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION))
                ImGui::Separator();
                static const char pmTooltip[] = "the ImGui::PanelManager layout\n(the sizes of the 4 docked windows and\nthe buttons that are selected\non the 4 toolbars)";
#           ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
                if (ImGui::Button("Save Panel Manager Layout")) SavePanelManagerIfSupported(*_manager);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save %s", pmTooltip);
#           endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#           ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
                ImGui::SameLine();if (ImGui::Button("Load Panel Manager Layout")) {
                    if (LoadPanelManagerIfSupported(*_manager))   setPanelManagerBoundsToIncludeMainMenuIfPresent();	// That's because we must adjust gpShowMainMenuBar state here (we have used a manual toggle button for it)
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load %s", pmTooltip);
#           endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#           endif //NO_IMGUIHELPER_SERIALIZATION

#           ifndef NO_IMGUITABWINDOW
#		if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION))
                ImGui::Separator();
                static const char twTooltip[] = "the layout of the 5 ImGui::TabWindows\n(this option is also available by right-clicking\non an empty space in the Tab Header)";
#		ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
                if (ImGui::Button("Save TabWindows Layout")) SaveTabWindowsIfSupported();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save %s", twTooltip);
#		endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#		ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
                ImGui::SameLine();if (ImGui::Button("Load TabWindows Layout")) LoadTabWindowsIfSupported();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load %s", twTooltip);
#		endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#		endif //NO_IMGUIHELPER_SERIALIZATION

                //ImGui::Spacing();
                //if (ImGui::Button("Reset Central Window Tabs")) ResetTabWindow(tabWindow);
#           endif //NO_IMGUITABWINDOW
            } else /*if (strcmp(wd.name,DockedWindowNames[5])==0)*/ {
                // Draw Application Window
                ImGui::Text("%s\n", wd.name);
            }
        } else {
            // Here we draw our toggle windows (in our case ToggleWindowNames) in the usual way:
            // We can use -1.f for alpha here, instead of mgr.getDockedWindowsAlpha(), that can be too low (but choose what you like)
            if (ImGui::Begin(wd.name, &wd.open, wd.size, -1.f, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ShowBorders)) {
                if (strcmp(wd.name, ToggleWindowNames[0]) == 0) {
                    // Draw Toggle Window 1
                    ImGui::SetWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x*0.15f, ImGui::GetIO().DisplaySize.y*0.24f), ImGuiSetCond_FirstUseEver);
                    ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x*0.25f, ImGui::GetIO().DisplaySize.y*0.24f), ImGuiSetCond_FirstUseEver);

                    ImGui::Text("Hello world from toggle window \"%s\"", wd.name);
                } else
                {
                    // Draw Toggle Window
                    ImGui::SetWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x*0.25f, ImGui::GetIO().DisplaySize.y*0.34f), ImGuiSetCond_FirstUseEver);
                    ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x*0.5f, ImGui::GetIO().DisplaySize.y*0.34f), ImGuiSetCond_FirstUseEver);
                    ImGui::Text("Hello world from toggle window \"%s\"", wd.name);

                    //ImGui::Checkbox("wd.open",&wd.open);  // This can be used to close the window too
                }
            }
            ImGui::End();
        }
    }
}; //namespace Divide