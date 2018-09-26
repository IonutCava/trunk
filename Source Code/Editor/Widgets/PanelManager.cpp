#include "stdafx.h"

#include "Headers/PanelManager.h"
#include "Headers/PanelManagerPane.h"
#include "Headers/TabbedWindow.h"
#include "Headers/DockedWindow.h"

#include "DockedWindows/Headers/OutputWindow.h"
#include "DockedWindows/Headers/PropertyWindow.h"
#include "DockedWindows/Headers/SolutionExplorerWindow.h"

#include "Editor/Headers/Editor.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include <imgui_internal.h>

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

    ResourceCache* PanelManager::s_globalCache = nullptr;
    hashMap<U32, Texture_ptr> PanelManager::s_imageEditorCache;

    namespace {

        void DrawDockedWindows(ImGui::PanelManagerWindowData& wd) {
            reinterpret_cast<DockedWindow*>(wd.userData)->draw();
        }

        static const ImVec2 buttonSize(PanelManager::ButtonWidth, PanelManager::ButtonHeight);
        static const ImVec2 buttonSizeSq(PanelManager::ButtonHeight, PanelManager::ButtonHeight);

        void TabContentProvider(ImGui::TabWindow::TabLabel* tab, ImGui::TabWindow& parent, void* userPtr) {
            ACKNOWLEDGE_UNUSED(parent);
            PanelManager* mgr = reinterpret_cast<PanelManager*>(userPtr);
            DisplayWindow& appWindow = mgr->context().activeWindow();

            // Users will use tab->userPtr here most of the time
            ImGui::Spacing();ImGui::Separator();
            if (tab) {
                ImGui::PushID(tab);
                if (tab->matchLabel("TabLabelStyle")) {
                    /*// Color Mode
                    static int colorEditMode = ImGuiColorEditMode_RGB;
                    static const char* btnlbls[2]={"HSV##myColorBtnType1","RGB##myColorBtnType1"};
                    if (colorEditMode!=ImGuiColorEditMode_RGB)  {
                    if (ImGui::SmallButton(btnlbls[0])) {
                    colorEditMode = ImGuiColorEditMode_RGB;
                    ImGui::ColorEditMode(colorEditMode);
                    }
                    }
                    else if (colorEditMode!=ImGuiColorEditMode_HSV)  {
                    if (ImGui::SmallButton(btnlbls[1])) {
                    colorEditMode = ImGuiColorEditMode_HSV;
                    ImGui::ColorEditMode(colorEditMode);
                    }
                    }
                    ImGui::SameLine(0);ImGui::Text("Color Mode");
                    ImGui::Separator();*/
                    ImGui::Spacing();
                    //ImGui::ColorEditMode(colorEditMode);
                    bool changed = ImGui::TabLabelStyle::Edit(ImGui::TabLabelStyle::Get());
                    ImGui::Separator();
                    const char* saveName = "tabLabelStyle.style";
                    const char* pSaveName = saveName;
                    if (ImGui::SmallButton("Save##saveGNEStyle1")) {
                        ImGui::TabLabelStyle::Save(ImGui::TabLabelStyle::Get(), pSaveName);
                        changed = false;tab->setModified(false);
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Load##loadGNEStyle1")) {
                        ImGui::TabLabelStyle::Load(ImGui::TabLabelStyle::Get(), pSaveName);
                        changed = false;tab->setModified(false);
                    }
                    ImGui::SameLine();

                    if (ImGui::SmallButton("Reset##resetGNEStyle1")) {
                        ImGui::TabLabelStyle::Reset(ImGui::TabLabelStyle::Get());
                        changed = false;tab->setModified(false);
                    }

                    ImGui::Spacing();
                    if (changed) tab->setModified(true);
                }
                /*else if (tab->matchLabel("Render")) {
                // Just some experiments here
                ImGui::BeginChild("MyChildWindow",ImVec2(0,50),true);
                //if (ImGui::IsMouseDragging(0,1.f)) ImGui::SetTooltip("%s","Mouse Dragging");
                ImGui::EndChild();
                ImGui::BeginChild("MyChildWindow2",ImVec2(0,0),true);
                ImGui::Text("Here is the content of tab label: \"%s\"\n",tab->getLabel());
                ImGui::EndChild();
                }*/
                else if (tab->matchLabel("MineGame")) {
#           if (defined(YES_IMGUIMINIGAMES) && !defined(NO_IMGUIMINIGAMES_MINE))
                    static ImGuiMiniGames::Mine mineGame;
                    mineGame.render();
#           else //NO_IMGUIMINIGAMES_MINE
                    ImGui::Text("Disabled for this build.");
#           endif // NO_IMGUIMINIGAMES_MINE
                } else if (tab->matchLabel("SudokuGame")) {
#           if (defined(YES_IMGUIMINIGAMES) && !defined(NO_IMGUIMINIGAMES_SUDOKU))
                    static ImGuiMiniGames::Sudoku sudokuGame;
                    sudokuGame.render();
#           else // NO_IMGUIMINIGAMES_SUDOKU
                    ImGui::Text("Disabled for this build.");
#           endif // NO_IMGUIMINIGAMES_SUDOKU
                } else if (tab->matchLabel("ImageEditor")) {
#           ifdef YES_IMGUIIMAGEEDITOR
                    static ImGui::ImageEditor imageEditor;
                    stringImpl file = Paths::g_assetsLocation + Paths::g_GUILocation;
                    file.append("blankImage.png");

                    if (!imageEditor.isInited() && !imageEditor.loadFromFile(file.c_str())) {
                        //fprintf(stderr,"Loading \"./blankImage.png\" Failed.\n");
                    }
                    imageEditor.render();
                    tab->setModified(imageEditor.getModified());    // actually this should be automatic if we use the TabLabel extension approach inside our TabWindows (ImageEditor derives from TabLabel by default, but it's not tested)
#           else //YES_IMGUIIMAGEEDITOR
                    ImGui::Text("Disabled for this build.");
#           endif //YES_IMGUIIMAGEEDITOR
                } else if (tab->matchLabel("Scene")) {
                    ImVec2 size(0.0f, 0.0f);
                    ImGuiWindow* window = ImGui::GetCurrentWindow();

                    const RenderTarget& rt = mgr->context().gfx().renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::EDITOR));
                    const Texture_ptr& gameView = rt.getAttachment(RTAttachmentType::Colour, 0).texture();

                    int w = (int)gameView->getWidth();
                    int h = (int)gameView->getHeight();

                    if (window && !window->SkipItems && h > 0) {

                        float zoom = 0.85f;
                        ImVec2 curPos = ImGui::GetCursorPos();
                        const ImVec2 wndSz(size.x > 0 ? size.x : ImGui::GetWindowSize().x - curPos.x, size.y > 0 ? size.y : ImGui::GetWindowSize().y - curPos.y);
                        IM_ASSERT(wndSz.x != 0 && wndSz.y != 0 && zoom != 0);

                        ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + wndSz.x, window->DC.CursorPos.y + wndSz.y));
                        ImGui::ItemSize(bb);
                        if (ImGui::ItemAdd(bb, NULL)) {

                            ImVec2 imageSz = wndSz - ImVec2(0.2f, 0.2f);
                            ImVec2 remainingWndSize(0, 0);
                            const float aspectRatio = (float)w / (float)h;

                            if (aspectRatio != 0) {
                                const float wndAspectRatio = wndSz.x / wndSz.y;
                                if (aspectRatio >= wndAspectRatio) {
                                    imageSz.y = imageSz.x / aspectRatio;
                                    remainingWndSize.y = wndSz.y - imageSz.y;
                                } else {
                                    imageSz.x = imageSz.y*aspectRatio;
                                    remainingWndSize.x = wndSz.x - imageSz.x;
                                }
                            }

                            const float zoomFactor = .5f / zoom;
                            ImVec2 uvExtension = ImVec2(2.f*zoomFactor, 2.f*zoomFactor);
                            if (remainingWndSize.x > 0) {
                                const float remainingSizeInUVSpace = 2.f*zoomFactor*(remainingWndSize.x / imageSz.x);
                                const float deltaUV = uvExtension.x;
                                const float remainingUV = 1.f - deltaUV;
                                if (deltaUV<1) {
                                    float adder = (remainingUV < remainingSizeInUVSpace ? remainingUV : remainingSizeInUVSpace);
                                    uvExtension.x += adder;
                                    remainingWndSize.x -= adder * zoom * imageSz.x;
                                    imageSz.x += adder * zoom * imageSz.x;
                                }
                            }
                            if (remainingWndSize.y > 0) {
                                const float remainingSizeInUVSpace = 2.f*zoomFactor*(remainingWndSize.y / imageSz.y);
                                const float deltaUV = uvExtension.y;
                                const float remainingUV = 1.f - deltaUV;
                                if (deltaUV<1) {
                                    float adder = (remainingUV < remainingSizeInUVSpace ? remainingUV : remainingSizeInUVSpace);
                                    uvExtension.y += adder;
                                    remainingWndSize.y -= adder * zoom * imageSz.y;
                                    imageSz.y += adder * zoom * imageSz.y;
                                }
                            }


                            ImVec2 startPos = bb.Min, endPos = bb.Max;
                            startPos.x += remainingWndSize.x*.5f;
                            startPos.y += remainingWndSize.y*.5f;
                            endPos.x = startPos.x + imageSz.x;
                            endPos.y = startPos.y + imageSz.y;

                            const ImGuiID id = (ImGuiID)((U64)mgr) + 1;
                            ImGui::PushID(id);

                            window->DrawList->AddImage((void *)(intptr_t)gameView->getHandle(), startPos, endPos, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

                            Attorney::EditorPanelManager::setScenePreviewRect(mgr->context().editor(), Rect<I32>(startPos.x, startPos.y, endPos.x, endPos.y));

                            ImGui::PopID();
                        }
                    }
                } else if (tab->matchLabel("StyleChooser")) {
#           ifndef IMGUISTYLESERIALIZER_H_
                    ImGui::Text("GUI Style Chooser Disabled for this build.");
                    ImGui::Spacing();ImGui::Separator();
#           endif //IMGUISTYLESERIALIZER_H_

                    ImGui::Spacing();
                    ImGui::TextDisabled("%s", "Some controls to chenge the GUI style:");
                    ImGui::PushItemWidth(275);
                    if (ImGui::DragFloat("Global Font Scale", &ImGui::GetIO().FontGlobalScale, 0.005f, 0.3f, 2.0f, "%.2f")) mgr->setPanelManagerBoundsToIncludeMainMenuIfPresent();  // This is because the Main Menu height changes with the Font Scale
                    ImGui::PopItemWidth();
                    if (ImGui::GetIO().FontGlobalScale != 1.f) {
                        ImGui::SameLine(0, 10);
                        if (ImGui::SmallButton("Reset##glFontGlobalScale")) {
                            ImGui::GetIO().FontGlobalScale = 1.f;
                            mgr->setPanelManagerBoundsToIncludeMainMenuIfPresent();  // This is because the Main Menu height changes with the Font Scale
                        }
                    }
                    ImGui::Spacing();

                    ImGui::PushItemWidth(275);
                    ImGui::ColorEdit4("glClearColor", appWindow.clearColour()._v);
                    ImGui::PopItemWidth();
                    if (appWindow.clearColour() != appWindow.originalClearColour()) {
                        ImGui::SameLine(0, 10);
                        if (ImGui::SmallButton("Reset##glClearColorReset")) appWindow.clearColour(appWindow.originalClearColour());
                    }
                    ImGui::Spacing();

#           ifdef IMGUISTYLESERIALIZER_H_
                    static int styleEnumNum = 1;
                    ImGui::PushItemWidth(ImGui::GetWindowWidth()*0.44f);
                    if (ImGui::Combo("Main Style Chooser", &styleEnumNum, ImGui::GetDefaultStyleNames(), (int)ImGuiStyle_Count, (int)ImGuiStyle_Count)) {
                        ImGui::ResetStyle(styleEnumNum);
                    }
                    ImGui::PopItemWidth();
                    if (ImGui::IsItemHovered()) {
                        if (styleEnumNum == ImGuiStyle_DefaultClassic)      ImGui::SetTooltip("%s", "\"Default\"\nThis is the default\nclassic ImGui theme");
                        else if (styleEnumNum == ImGuiStyle_DefaultDark)      ImGui::SetTooltip("%s", "\"DefaultDark\"\nThis is the default\ndark ImGui theme");
                        else if (styleEnumNum == ImGuiStyle_DefaultLight)      ImGui::SetTooltip("%s", "\"DefaultLight\"\nThis is the default\nlight ImGui theme");
                        else if (styleEnumNum == ImGuiStyle_Gray)   ImGui::SetTooltip("%s", "\"Gray\"\nThis is the default\ntheme of this demo");
                        else if (styleEnumNum == ImGuiStyle_BlackCodz01)   ImGui::SetTooltip("%s", "\"GrayCodz01\"\nPosted by @codz01 here:\nhttps://github.com/ocornut/imgui/issues/707\n(hope I can use it)");
                        else if (styleEnumNum == ImGuiStyle_GrayCodz01)   ImGui::SetTooltip("%s", "\"GrayCodz01\"\nPosted by @codz01 here:\nhttps://github.com/ocornut/imgui/issues/1607\n(hope I can use it)");
                        else if (styleEnumNum == ImGuiStyle_DarkOpaque)   ImGui::SetTooltip("%s", "\"DarkOpaque\"\nA dark-grayscale style with\nno transparency (by default)");
                        else if (styleEnumNum == ImGuiStyle_Purple)   ImGui::SetTooltip("%s", "\"Purple\"\nPosted by @fallrisk here:\nhttps://github.com/ocornut/imgui/issues/1607\n(hope I can use it)");
                        else if (styleEnumNum == ImGuiStyle_Soft) ImGui::SetTooltip("%s", "\"Soft\"\nPosted by @olekristensen here:\nhttps://github.com/ocornut/imgui/issues/539\n(hope I can use it)");
                        else if (styleEnumNum == ImGuiStyle_EdinBlack || styleEnumNum == ImGuiStyle_EdinWhite) ImGui::SetTooltip("%s", "Based on an image posted by @edin_p\n(hope I can use it)");
                        else if (styleEnumNum == ImGuiStyle_Maya) ImGui::SetTooltip("%s", "\"Maya\"\nPosted by @ongamex here:\nhttps://gist.github.com/ongamex/4ee36fb23d6c527939d0f4ba72144d29\n(hope I can use it)");
                        else if (styleEnumNum == ImGuiStyle_LightGreen) ImGui::SetTooltip("%s", "\"LightGreen\"\nPosted by @ebachard here:\nhttps://github.com/ocornut/imgui/pull/1776\n(hope I can use it)");
                        else if (styleEnumNum == ImGuiStyle_Design) ImGui::SetTooltip("%s", "\"Design\"\nPosted by @usernameiwantedwasalreadytaken here:\nhttps://github.com/ocornut/imgui/issues/707\n(hope I can use it)");
                    }
                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
#           endif // IMGUISTYLESERIALIZER_H_

                    ImGui::TextDisabled("%s", "These are also present in the \"Preferences\" Panel:");
                    ImGui::DragFloat("Window Alpha##WA2", &mgr->ImGuiPanelManager().getDockedWindowsAlpha(), 0.005f, -0.01f, 1.0f, mgr->ImGuiPanelManager().getDockedWindowsAlpha() < 0.0f ? "(default)" : "%.3f");


                } else ImGui::Text("Here is the content of tab label: \"%s\"\n", tab->getLabel());
                ImGui::PopID();
            } else { ImGui::Text("EMPTY TAB LABEL DOCKING SPACE.");ImGui::Text("PLEASE DRAG AND DROP TAB LABELS HERE!"); }
            ImGui::Separator();ImGui::Spacing();
        }

        void TabLabelPopupMenuProvider(ImGui::TabWindow::TabLabel* tab, ImGui::TabWindow& parent, void* userPtr) {
            ACKNOWLEDGE_UNUSED(parent);
            ACKNOWLEDGE_UNUSED(userPtr);
            if (ImGui::BeginPopup(ImGui::TabWindow::GetTabLabelPopupMenuName())) {
                ImGui::PushID(tab);
                ImGui::Text("\"%.*s\" Menu", (int)(strlen(tab->getLabel()) - (tab->getModified() ? 1 : 0)), tab->getLabel());
                ImGui::Separator();
                if (!tab->matchLabel("TabLabelStyle") && !tab->matchLabel("Style")) {
                    if (tab->getModified()) { if (ImGui::MenuItem("Mark as not modified")) tab->setModified(false); } else { if (ImGui::MenuItem("Mark as modified"))     tab->setModified(true); }
                    ImGui::Separator();
                }
                ImGui::MenuItem("Entry 1");
                ImGui::MenuItem("Entry 2");
                ImGui::MenuItem("Entry 3");
                ImGui::MenuItem("Entry 4");
                ImGui::MenuItem("Entry 5");
                ImGui::PopID();
                ImGui::EndPopup();
            }

        }
        void TabLabelGroupPopupMenuProvider(ImVector<ImGui::TabWindow::TabLabel*>& tabs, ImGui::TabWindow& parent, ImGui::TabWindowNode* tabNode, void* userPtr) {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(ImGui::TabLabelStyle::Get().colors[ImGui::TabLabelStyle::Col_TabLabel]));
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ImGui::TabLabelStyle::Get().colors[ImGui::TabLabelStyle::Col_TabLabelText]));
            if (ImGui::BeginPopup(ImGui::TabWindow::GetTabLabelGroupPopupMenuName())) {
                ImGui::Text("TabLabel Group Menu");
                ImGui::Separator();
                if (parent.isMergeble(tabNode) && ImGui::MenuItem("Merge with parent group")) parent.merge(tabNode); // Warning: this invalidates "tabNode" after the call
                if (ImGui::MenuItem("Close all tabs in this group")) {
                    for (int i = 0, isz = tabs.size();i<isz;i++) {
                        ImGui::TabWindow::TabLabel* tab = tabs[i];
                        if (tab->isClosable())  // otherwise even non-closable tabs will be closed
                        {
                            //parent.removeTabLabel(tab);
                            tab->mustCloseNextFrame = true;  // alternative way... this asks for saving if file is modified
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save all tabs in all groups")) { parent.saveAll(); }
                if (ImGui::MenuItem("Close all tabs in all groups")) {
                    // This methods fires a modal dialog if we have unsaved files. To prevent this, we can try calling: parent.saveAll(); before
                    parent.startCloseAllDialog(NULL, true);  // returns true when a modal dialog is stated
                                                             // Note that modal dialogs in ImGui DON'T return soon with the result.
                                                             // However we can't prevent their "Cancel" button, so that if we know that it is started (=return value above),
                                                             // then we can call ImGui::TabWindow::AreSomeDialogsOpen(); and, if the return value is false, it is safe to close the program.
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Save Layout")) {
                    PanelManager* mgr = reinterpret_cast<PanelManager*>(userPtr);
                    mgr->saveTabsToFile();
                }

                if (ImGui::MenuItem("Load Layout")) {
                    PanelManager* mgr = reinterpret_cast<PanelManager*>(userPtr);
                    mgr->loadTabsFromFile();
                }

                ImGui::EndPopup();
            }
            ImGui::PopStyleColor(2);
        }

        ImTextureID LoadTextureFromMemory(Texture_ptr& target, const stringImpl& name, int width, int height, int channels, const unsigned char* pixels, bool useMipmapsIfPossible=false, bool wraps=true, bool wrapt=true, bool minFilterNearest=false, bool magFilterNearest=false) {
        
            CLAMP(channels, 1, 4);

            ImTextureID texId = NULL;

            SamplerDescriptor sampler;
            sampler.setMinFilter(minFilterNearest ? (useMipmapsIfPossible ? TextureFilter::NEAREST_MIPMAP_NEAREST : TextureFilter::NEAREST)
                                                  : (useMipmapsIfPossible ? TextureFilter::LINEAR_MIPMAP_LINEAR : TextureFilter::LINEAR));
            sampler.setMagFilter(magFilterNearest ? TextureFilter::NEAREST : TextureFilter::LINEAR);
            sampler.setWrapModeU(wraps ? TextureWrap::REPEAT : TextureWrap::CLAMP);
            sampler.setWrapModeV(wrapt ? TextureWrap::REPEAT : TextureWrap::CLAMP);

            TextureDescriptor descriptor(TextureType::TEXTURE_2D,
                                         channels > 3 ? GFXImageFormat::RGBA8 :
                                                        channels > 2 ? GFXImageFormat::RGB8 :
                                                                       channels > 1 ? GFXImageFormat::RG8 :
                                                                                      GFXImageFormat::RED,
                                         GFXDataFormat::UNSIGNED_BYTE);
            descriptor.setSampler(sampler);
            descriptor.automaticMipMapGeneration(useMipmapsIfPossible);

            ResourceDescriptor textureDescriptor(name);

            textureDescriptor.setThreadedLoading(false);
            textureDescriptor.setFlag(true);
            textureDescriptor.setPropertyDescriptor(descriptor);

            target = CreateResource<Texture>(*PanelManager::s_globalCache, textureDescriptor);
            assert(target);

            Texture::TextureLoadInfo info;

            target->loadData(info, (bufferPtr)pixels, vec2<U16>(width, height));

            texId = reinterpret_cast<void*>((intptr_t)target->getHandle());

            return texId;
        }

        ImTextureID LoadTextureFromMemory(Texture_ptr& target, const stringImpl& name, const unsigned char* filenameInMemory, int filenameInMemorySize, int req_comp = 0) {
            int w, h, n;
            unsigned char* pixels = stbi_load_from_memory(filenameInMemory, filenameInMemorySize, &w, &h, &n, req_comp);
            if (!pixels) {
                return 0;
            }
            ImTextureID ret = LoadTextureFromMemory(target, name, w, h, req_comp > 0 ? req_comp : n, pixels, false, true, true, false, false);

            stbi_image_free(pixels);

            return ret;
        }
        void FreeTextureDelegate(ImTextureID& texid) {
            hashMap<U32, Texture_ptr>::iterator it = PanelManager::s_imageEditorCache.find((U32)(intptr_t)texid);
            if (it != std::cend(PanelManager::s_imageEditorCache)) {
                PanelManager::s_imageEditorCache.erase(it);
            } else {
                // error
            }
        }

        void GenerateOrUpdateTexture(ImTextureID& imtexid, int width, int height, int channels, const unsigned char* pixels, bool useMipmapsIfPossible, bool wraps, bool wrapt, bool minFilterNearest, bool magFilterNearest) {
            Texture_ptr tex = nullptr;
            LoadTextureFromMemory(tex,
                                  Util::StringFormat("temp_edit_tex_%d", PanelManager::s_imageEditorCache.size()),
                                  width,
                                  height,
                                  channels,
                                  pixels,
                                  useMipmapsIfPossible,
                                  wraps,
                                  wrapt,
                                  minFilterNearest,
                                  magFilterNearest);
            if (tex != nullptr) {
                imtexid = reinterpret_cast<void*>((intptr_t)tex->getHandle());
                PanelManager::s_imageEditorCache[tex->getHandle()] = tex;
            }
        }
    };

    PanelManager::PanelManager(PlatformContext& context)
        : PlatformContextComponent(context),
          _deltaTime(0ULL),
          _sceneStepCount(0),
          _simulationPaused(nullptr),
          _showCentralWindow(nullptr),
          _selectedCamera(nullptr),
          _manager(std::make_unique<ImGui::PanelManager>()),
          _saveFile(Paths::g_saveLocation + Paths::Editor::g_saveLocation + Paths::Editor::g_panelLayoutFile)
    {
        _tabWindows.resize(5);

        MemoryManager::DELETE_VECTOR(_dockedWindows);
        _dockedWindows.push_back(MemoryManager_NEW SolutionExplorerWindow(*this, context));
        _dockedWindows.push_back(MemoryManager_NEW PropertyWindow(*this, context));
        _dockedWindows.push_back(MemoryManager_NEW OutputWindow(*this));
    }


    PanelManager::~PanelManager()
    {
        s_imageEditorCache.clear();
        _tabWindows.clear();
        MemoryManager::DELETE_VECTOR(_dockedWindows);
    }

    bool PanelManager::saveToFile() const {
        return ImGui::PanelManager::Save(*_manager, _saveFile.c_str());
    }

    bool PanelManager::loadFromFile() {
        return ImGui::PanelManager::Load(*_manager, _saveFile.c_str());
    }

    bool PanelManager::saveTabsToFile() const {
        return TabbedWindow::saveToFile(_tabWindows.data(), _tabWindows.size());
    }

    bool PanelManager::loadTabsFromFile() {
        return TabbedWindow::loadFromFile(_tabWindows.data(), _tabWindows.size());
    }

    void PanelManager::idle() {
        if (_sceneStepCount > 0) {
            _sceneStepCount--;
        }

        ImGui::PanelManager::Pane& pane = _toolbars[to_base(PanelPositions::TOP)]->impl();
        ImGui::Toolbar::Button* btn1 = pane.bar.getButton(pane.getSize() - 2);
        if (btn1->isDown) {                          // [***]
            _sceneStepCount = 1;
            btn1->isDown = false;
        }
        ImGui::Toolbar::Button* btn2 = pane.bar.getButton(pane.getSize() - 1);
        if (btn2->isDown) {                          // [****]
            _sceneStepCount = Config::TARGET_FRAME_RATE;
            btn2->isDown = false;
        }
    }


    void PanelManager::draw(const U64 deltaTime) {
        _deltaTime = deltaTime;

        if (_showCentralWindow && *_showCentralWindow) {
            const ImVec2& iqs = _manager->getCentralQuadSize();
            if (iqs.x>ImGui::GetStyle().WindowMinSize.x && iqs.y>ImGui::GetStyle().WindowMinSize.y) {
                ImGui::SetNextWindowPos(_manager->getCentralQuadPosition());
                ImGui::SetNextWindowSize(_manager->getCentralQuadSize());
                if (ImGui::Begin("Central Window", NULL, ImVec2(0, 0), _manager->getDockedWindowsAlpha(), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | _manager->getDockedWindowsExtraFlags() /*| ImGuiWindowFlags_NoBringToFrontOnFocus*/)) {
                    _tabWindows[0].render(); // Must be called inside "its" window (and sets isInited() to false). [ChildWindows can't be used here (but they can be used inside Tab Pages). Basically all the "Central Window" must be given to 'tabWindow'.]
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

    void PanelManager::init(const vec2<U16>& renderResolution) {
        if (s_globalCache == nullptr) {
            s_globalCache = &context().kernel().resourceCache();
        }

        if (!ImGui::TabWindow::DockPanelIconTextureID) {
            ImVector<unsigned char> rgba_buffer;int w, h;
            ImGui::TabWindow::GetDockPanelIconImageRGBA(rgba_buffer, &w, &h);
            ImGui::TabWindow::DockPanelIconTextureID = 
                LoadTextureFromMemory(_textures[to_base(TextureUsage::Dock)],
                                      "Docking Texture",
                                      w,
                                      h,
                                      4,
                                      &rgba_buffer[0]);
        }

        ImGui::TabWindow::SetWindowContentDrawerCallback(&TabContentProvider, this); // Mandatory
        ImGui::TabWindow::SetTabLabelPopupMenuDrawerCallback(&TabLabelPopupMenuProvider, this);  // Optional (if you need context-menu)
        ImGui::TabWindow::SetTabLabelGroupPopupMenuDrawerCallback(&TabLabelGroupPopupMenuProvider, this);    // Optional (fired when RMB is clicked on an empty spot in the tab area)
#   ifdef YES_IMGUIIMAGEEDITOR
        ImGui::ImageEditor::SetGenerateOrUpdateTextureCallback(&GenerateOrUpdateTexture);   // This will be called only with channels=3 or channels=4
        ImGui::ImageEditor::SetFreeTextureCallback(&FreeTextureDelegate);
#   endif
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

            _textures[to_base(TextureUsage::Tile)] = CreateResource<Texture>(*s_globalCache, texture);

            texture.name("Panel Manager Texture 2");
            texture.setResourceName("myNumbersTexture.png");
            _textures[to_base(TextureUsage::Numbers)] = CreateResource<Texture>(*s_globalCache, texture);

            // Hp) All the associated windows MUST have an unique name WITHOUT using the '##' chars that ImGui supports
            void* myImageTextureVoid = reinterpret_cast<void*>((intptr_t)_textures[to_base(TextureUsage::Tile)]->getHandle());         // 8x8 tiles
            void* myImageTextureVoid2 = reinterpret_cast<void*>((intptr_t)_textures[to_base(TextureUsage::Numbers)]->getHandle());       // 3x3 tiles
            ImVec2 uv0(0, 0), uv1(0, 0);int tileNumber = 0;

            _toolbars[to_base(PanelPositions::LEFT)] = std::make_unique<PanelManagerPane>(*this, "LeftToolbar##foo", ImGui::PanelManager::LEFT);
            _toolbars[to_base(PanelPositions::RIGHT)] = std::make_unique<PanelManagerPane>(*this, "RightToolbar##foo", ImGui::PanelManager::RIGHT);
            _toolbars[to_base(PanelPositions::TOP)] = std::make_unique<PanelManagerPane>(*this, "TopToobar##foo", ImGui::PanelManager::TOP);
            _toolbars[to_base(PanelPositions::BOTTOM)] = std::make_unique<PanelManagerPane>(*this, "BottomToolbar##foo", ImGui::PanelManager::BOTTOM);

            // LEFT PANE
            {
                ImGui::PanelManager::Pane& pane = _toolbars[to_base(PanelPositions::LEFT)]->impl();
                tileNumber = 0;
                uv0 = ImVec2((F32)(tileNumber % 8) / 8.f, (F32)(tileNumber / 8) / 8.f);
                uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                pane.addButtonAndWindow(ImGui::Toolbutton(_dockedWindows[0]->name(),
                                                          myImageTextureVoid,
                                                          uv0,
                                                          uv1,
                                                          buttonSize),         // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                         ImGui::PanelManagerPaneAssociatedWindow(_dockedWindows[0]->name(),
                                                                                 -1,
                                                                                 &DrawDockedWindows,
                                                                                 reinterpret_cast<void*>(_dockedWindows[0])));  //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
                pane.addSeparator(48); // Note that a separator "eats" one toolbutton index as if it was a real button

            }
            // RIGHT PANE
            {
                ImGui::PanelManager::Pane& pane = _toolbars[to_base(PanelPositions::RIGHT)]->impl();
                tileNumber = 1;
                uv0 = ImVec2((F32)(tileNumber % 8) / 8.f, (F32)(tileNumber / 8) / 8.f);
                uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                pane.addButtonAndWindow(ImGui::Toolbutton(_dockedWindows[1]->name(),
                                        myImageTextureVoid,
                                        uv0,
                                        uv1,
                                        buttonSize),         // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                        ImGui::PanelManagerPaneAssociatedWindow(_dockedWindows[1]->name(),
                                                                                -1,
                                                                                &DrawDockedWindows,
                                                                                reinterpret_cast<void*>(_dockedWindows[1])));
                pane.addSeparator(48); // Note that a separator "eats" one toolbutton index as if it was a real button
            }
            // BOTTOM PANE
            {
                ImGui::PanelManager::Pane& pane = _toolbars[to_base(PanelPositions::BOTTOM)]->impl();
                // Add to left pane the windows DrawDockedWindows[i] from 3 to 6, with Toolbuttons with the images from 3 to 6 of myImageTextureVoid (8x8 tiles):
                tileNumber = 2;
                uv0 = ImVec2((F32)(tileNumber % 8) / 8.f, (F32)(tileNumber / 8) / 8.f);
                uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                pane.addButtonAndWindow(ImGui::Toolbutton(_dockedWindows[2]->name(),
                                                            myImageTextureVoid,
                                                            uv0,
                                                            uv1,
                                                            buttonSizeSq),         // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                        ImGui::PanelManagerPaneAssociatedWindow(_dockedWindows[2]->name(),
                                                                                -1,
                                                                                &DrawDockedWindows,
                                                                                reinterpret_cast<void*>(_dockedWindows[2])));  //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
            }
            // TOP PANE
            {
                ImGui::PanelManager::Pane& pane = _toolbars[to_base(PanelPositions::TOP)]->impl();
                tileNumber = 5;
                uv0 = ImVec2((F32)(tileNumber % 8) / 8.f, (F32)(tileNumber / 8) / 8.f);
                uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                pane.addButtonOnly(ImGui::Toolbutton("Show/Hide central window", myImageTextureVoid, uv0, uv1, buttonSizeSq, true, true));  // [**] Here we add a manual toggle button we'll process later [**]

                tileNumber = 34;
                uv0 = ImVec2((F32)(tileNumber % 8) / 8.f, (F32)(tileNumber / 8) / 8.f);
                uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                pane.addButtonOnly(ImGui::Toolbutton("Play/Pause", myImageTextureVoid, uv0, uv1, buttonSize, true, false, ImVec4(1,0,0,1)));

                tileNumber = 34;
                uv0 = ImVec2((F32)(tileNumber % 8) / 8.f, (F32)(tileNumber / 8) / 8.f);
                uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                pane.addButtonOnly(ImGui::Toolbutton("1 Frame Step", myImageTextureVoid, uv0, uv1, buttonSize, true, false, ImVec4(1, 1, 0, 1)));

                tileNumber = 34;
                uv0 = ImVec2((F32)(tileNumber % 8) / 8.f, (F32)(tileNumber / 8) / 8.f);
                uv1 = ImVec2(uv0.x + 1.f / 8.f, uv0.y + 1.f / 8.f);
                pane.addButtonOnly(ImGui::Toolbutton("60 Frames Step", myImageTextureVoid, uv0, uv1, buttonSize, true, false, ImVec4(1, 1, 0.5, 1)));

                // Ok. Now all the buttons/windows have been added to the TOP Pane.
                // Please note that it's not safe to add EVERYTHING (even separators) to this (TOP) pane afterwards (unless we bind the booleans again).
                _showCentralWindow = &pane.bar.getButton(pane.getSize() - 4)->isDown;          // [*]
                _simulationPaused = &pane.bar.getButton(pane.getSize() - 3)->isDown;           // [**]
            }

            // Optional. Loads the layout (just selectedButtons, docked windows sizes and stuff like that)
            loadFromFile();

            _manager->setDockedWindowsAlpha(1.0f);

            // Optional line that affects the look of all the Toolbuttons in the Panes inserted so far:
            _manager->overrideAllExistingPanesDisplayProperties(ImVec2(0.25f, 0.9f), ImVec4(0.85, 0.85, 0.85, 1));

            // These line is only necessary to accommodate space for the global menu bar we're using:
            setPanelManagerBoundsToIncludeMainMenuIfPresent();
        }

        // The following block used to be in DrawGL(), but it's better to move it here (it's part of the initalization)
        // Here we load all the Tabs, if their config file is available
        // Otherwise we create some Tabs in the central tabWindow (tabWindows[0])
        ImGui::TabWindow& tabWindow = _tabWindows[0];
        if (!tabWindow.isInited()) {
            // tabWindow.isInited() becomes true after the first call to tabWindow.render() [in DrawGL()]
            for (ImGui::TabWindow& window : _tabWindows) {
                window.clear();  // for robustness (they're already empty)
            }

            
            if (!loadTabsFromFile()) {
                // Here we set the starting configurations of the tabs in our ImGui::TabWindows
                static const char* tabNames[] = { "TabLabelStyle","Scene","Texture" };
                static const int numTabs = sizeof(tabNames) / sizeof(tabNames[0]);
                static const char* tabTooltips[numTabs] = { "Edit the look of the tab labels", "Scene view", "Texture view" };
                for (int i = 0;i<numTabs;i++) {
                    tabWindow.addTabLabel(tabNames[i], tabTooltips[i], i % 3 != 0, i % 5 != 4);
                }
#       ifdef YES_IMGUIMINIGAMES
#           ifndef NO_IMGUIMINIGAMES_MINE
                tabWindow.addTabLabel("MineGame", "a mini-game", false, true, NULL, NULL, 0, ImGuiWindowFlags_NoScrollbar);
#           endif // NO_IMGUIMINIGAMES_MINE
#           ifndef NO_IMGUIMINIGAMES_SUDOKU
                tabWindow.addTabLabel("SudokuGame", "a mini-game", false, true, NULL, NULL, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
#           endif // NO_IMGUIMINIGAMES_SUDOKU
#       endif // YES_IMGUIMINIGAMES
#       ifdef YES_IMGUIIMAGEEDITOR
                tabWindow.addTabLabel("ImageEditor", "a tiny image editor", false, true, NULL, NULL, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
#       endif //YES_IMGUIIMAGEEDITOR
                tabWindow.addTabLabel("StyleChooser", "Edit the look of the GUI", false);
            }
        }

        resize(renderResolution.width, renderResolution.height);
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
        if (displayX <= 0) 
            displayX = (int)ImGui::GetIO().DisplaySize.x;

        if (displayY <= 0) 
            displayY = (int)ImGui::GetIO().DisplaySize.y;

        ImVec4 bounds(0, 0, (float)displayX, (float)displayY);   // (0,0,-1,-1) defaults to (0,0,io.DisplaySize.x,io.DisplaySize.y)
        const float mainMenuHeight = calcMainMenuHeight();
        bounds = ImVec4(0, mainMenuHeight, to_F32(displayX), to_F32(displayY) - mainMenuHeight);
        
        _manager->setDisplayPortion(bounds);
    }

    void PanelManager::resize(int w, int h) {
        static ImVec2 initialSize((float)w, (float)h);
        _manager->setToolbarsScaling((float)w / initialSize.x,
                                     (float)h / initialSize.y);  // Scales the PanelManager bounds based on the initialSize
        setPanelManagerBoundsToIncludeMainMenuIfPresent(w, h);   // This line is only necessary if we have a global menu bar
    }

    void PanelManager::drawDockedTabWindows(ImGui::PanelManagerWindowData& wd) {
        // See more generic DrawDockedWindows(...) below...
        ImGui::TabWindow& tabWindow = _tabWindows[(wd.dockPos == ImGui::PanelManager::LEFT) ? 1 :
            (wd.dockPos == ImGui::PanelManager::RIGHT) ? 2 :
            (wd.dockPos == ImGui::PanelManager::TOP) ? 3
            : 4];
        tabWindow.render();
    }

    void PanelManager::setSelectedCamera(Camera* camera) {
        _selectedCamera = camera;
    }

    Camera* PanelManager::getSelectedCamera() const {
        return _selectedCamera;
    }


    void PanelManager::setTransformSettings(const TransformSettings& settings) {
        Attorney::EditorPanelManager::setTransformSettings(context().editor(), settings);
    }

    const TransformSettings& PanelManager::getTransformSettings() const {
        return Attorney::EditorPanelManager::getTransformSettings(context().editor());
    }



    bool Attorney::PanelManagerDockedWindows::editorEnableGizmo(Divide::PanelManager& mgr) {
        return Attorney::EditorPanelManager::enableGizmo(mgr.context().editor());
    }

    void Attorney::PanelManagerDockedWindows::editorEnableGizmo(Divide::PanelManager& mgr, bool state) {
        Attorney::EditorPanelManager::enableGizmo(mgr.context().editor(), state);
    }
}; //namespace Divide