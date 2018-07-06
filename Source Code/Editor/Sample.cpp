#include "stdafx.h"

#include "Headers/Sample.h"

namespace Divide {
    static ImGui::PanelManager mgr;
    ImTextureID myImageTextureId = 0;
    ImTextureID myImageTextureId2 = 0;
    static const bool* gpShowMainMenuBar = nullptr;
    static const bool* gpShowCentralWindow = nullptr;
    static const ImVec4 gDefaultClearColor(0.8f, 0.6f, 0.6f, 1.0f);
    static ImVec4 gClearColor = gDefaultClearColor;

    inline static float CalcMainMenuHeight() {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        ImFont* font = ImGui::GetFont();
        if (!font) {
            if (io.Fonts->Fonts.size()>0) font = io.Fonts->Fonts[0];
            else return (14) + style.FramePadding.y * 2.0f;
        }
        return (io.FontGlobalScale * font->Scale * font->FontSize) + style.FramePadding.y * 2.0f;
    }

    inline static void SetPanelManagerBoundsToIncludeMainMenuIfPresent(int displayX = -1, int displayY = -1) {
        if (gpShowMainMenuBar) {
            if (displayX <= 0) displayX = ImGui::GetIO().DisplaySize.x;
            if (displayY <= 0) displayY = ImGui::GetIO().DisplaySize.y;
            ImVec4 bounds(0, 0, (float)displayX, (float)displayY);   // (0,0,-1,-1) defaults to (0,0,io.DisplaySize.x,io.DisplaySize.y)
            if (*gpShowMainMenuBar) {
                const float mainMenuHeight = CalcMainMenuHeight();
                bounds = ImVec4(0, mainMenuHeight, displayX, displayY - mainMenuHeight);
            }
            mgr.setDisplayPortion(bounds);
        }
    }

    ImGui::TabWindow tabWindows[5];
    static const char tabWindowsSaveName[] = "myTabWindow.layout";
    static bool LoadTabWindowsIfSupported() {
        bool loadedFromFile = false;
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_LOAD))
        const char* pSaveName = tabWindowsSaveName;
        //loadedFromFile = tabWindow.load(pSaveName);   // This is good for a single TabWindow
        loadedFromFile = ImGui::TabWindow::Load(pSaveName, &tabWindows[0], sizeof(tabWindows) / sizeof(tabWindows[0]));  // This is OK for a multiple TabWindows
#   endif
        return loadedFromFile;
    }

    static bool SaveTabWindowsIfSupported() {
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_SAVE))
        const char* pSaveName = tabWindowsSaveName;

        //if (parent.save(pSaveName))   // This is OK for a single TabWindow
        if (ImGui::TabWindow::Save(pSaveName, &tabWindows[0], sizeof(tabWindows) / sizeof(tabWindows[0])))  // This is OK for a multiple TabWindows
        {
            return true;
        }
#   endif //!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && ...
        return false;
    }

    // Callbacks used by all ImGui::TabWindows and set in InitGL()
    void TabContentProvider(ImGui::TabWindow::TabLabel* tab, ImGui::TabWindow& parent, void* userPtr) {
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
#if             (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION))
                const char* saveName = "tabLabelStyle.style";
                const char* saveNamePersistent = "/persistent_folder/tabLabelStyle.style";
                const char* pSaveName = saveName;
#               ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
                if (ImGui::SmallButton("Save##saveGNEStyle1")) {
#               ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    pSaveName = saveNamePersistent;
#               endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    ImGui::TabLabelStyle::Save(ImGui::TabLabelStyle::Get(), pSaveName);
#               ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    ImGui::EmscriptenFileSystemHelper::Sync();
#               endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    changed = false;tab->setModified(false);
                }
                ImGui::SameLine();
#               endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#               ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
                if (ImGui::SmallButton("Load##loadGNEStyle1")) {
#               ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    if (ImGuiHelper::FileExists(saveNamePersistent)) pSaveName = saveNamePersistent;
#               endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    ImGui::TabLabelStyle::Load(ImGui::TabLabelStyle::Get(), pSaveName);
                    changed = false;tab->setModified(false);
                }
                ImGui::SameLine();
#               endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#               endif //NO_IMGUIHELPER_SERIALIZATION

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
            else if (tab->matchLabel("ImGuiMineGame")) {
#           if (defined(YES_IMGUIMINIGAMES) && !defined(NO_IMGUIMINIGAMES_MINE))
                static ImGuiMiniGames::Mine mineGame;
                mineGame.render();
#           else //NO_IMGUIMINIGAMES_MINE
                ImGui::Text("Disabled for this build.");
#           endif // NO_IMGUIMINIGAMES_MINE
            } else if (tab->matchLabel("ImGuiSudokuGame")) {
#           if (defined(YES_IMGUIMINIGAMES) && !defined(NO_IMGUIMINIGAMES_SUDOKU))
                static ImGuiMiniGames::Sudoku sudokuGame;
                sudokuGame.render();
#           else // NO_IMGUIMINIGAMES_SUDOKU
                ImGui::Text("Disabled for this build.");
#           endif // NO_IMGUIMINIGAMES_SUDOKU
            } else if (tab->matchLabel("ImGuiImageEditor")) {
#           ifdef YES_IMGUIIMAGEEDITOR
                static ImGui::ImageEditor imageEditor;
                if (!imageEditor.isInited() && !imageEditor.loadFromFile("./blankImage.png")) {
                    //fprintf(stderr,"Loading \"./blankImage.png\" Failed.\n");
                }
                imageEditor.render();
                tab->setModified(imageEditor.getModified());    // actually this should be automatic if we use the TabLabel extension approach inside our TabWindows (ImageEditor derives from TabLabel by default, but it's not tested)
#           else //YES_IMGUIIMAGEEDITOR
                ImGui::Text("Disabled for this build.");
#           endif //YES_IMGUIIMAGEEDITOR
            } else if (tab->matchLabel("ImGuiStyleChooser")) {
#           ifndef IMGUISTYLESERIALIZER_H_
                ImGui::Text("GUI Style Chooser Disabled for this build.");
                ImGui::Spacing();ImGui::Separator();
#           endif //IMGUISTYLESERIALIZER_H_

                ImGui::Spacing();
                ImGui::TextDisabled("%s", "Some controls to chenge the GUI style:");
                ImGui::PushItemWidth(275);
                if (ImGui::DragFloat("Global Font Scale", &ImGui::GetIO().FontGlobalScale, 0.005f, 0.3f, 2.0f, "%.2f")) SetPanelManagerBoundsToIncludeMainMenuIfPresent();  // This is because the Main Menu height changes with the Font Scale
                ImGui::PopItemWidth();
                if (ImGui::GetIO().FontGlobalScale != 1.f) {
                    ImGui::SameLine(0, 10);
                    if (ImGui::SmallButton("Reset##glFontGlobalScale")) {
                        ImGui::GetIO().FontGlobalScale = 1.f;
                        SetPanelManagerBoundsToIncludeMainMenuIfPresent();  // This is because the Main Menu height changes with the Font Scale
                    }
                }
                ImGui::Spacing();

                ImGui::PushItemWidth(275);
                ImGui::ColorEdit3("glClearColor", &gClearColor.x);
                ImGui::PopItemWidth();
                if (gClearColor.x != gDefaultClearColor.x || gClearColor.y != gDefaultClearColor.y || gClearColor.z != gDefaultClearColor.z) {
                    ImGui::SameLine(0, 10);
                    if (ImGui::SmallButton("Reset##glClearColorReset")) gClearColor = gDefaultClearColor;
                }
                ImGui::Spacing();

#           ifdef IMGUISTYLESERIALIZER_H_
                static int styleEnumNum = 0;
                ImGui::PushItemWidth(ImGui::GetWindowWidth()*0.44f);
                if (ImGui::Combo("Main Style Chooser", &styleEnumNum, ImGui::GetDefaultStyleNames(), (int)ImGuiStyle_Count, (int)ImGuiStyle_Count)) {
                    ImGui::ResetStyle(styleEnumNum);
                }
                ImGui::PopItemWidth();
                if (ImGui::IsItemHovered()) {
                    if (styleEnumNum == ImGuiStyle_Default)      ImGui::SetTooltip("%s", "\"Default\"\nThis is the default\nclassic ImGui theme");
                    else if (styleEnumNum == ImGuiStyle_DefaultDark)      ImGui::SetTooltip("%s", "\"DefaultDark\"\nThis is the default\ndark ImGui theme");
                    else if (styleEnumNum == ImGuiStyle_Gray)   ImGui::SetTooltip("%s", "\"Gray\"\nThis is the default theme of first demo");
                    else if (styleEnumNum == ImGuiStyle_OSX)   ImGui::SetTooltip("%s", "\"OSX\"\nPosted by @itamago here:\nhttps://github.com/ocornut/imgui/pull/511\n(hope I can use it)");
                    else if (styleEnumNum == ImGuiStyle_DarkOpaque)   ImGui::SetTooltip("%s", "\"DarkOpaque\"\nA dark-grayscale style with\nno transparency (by default)");
                    else if (styleEnumNum == ImGuiStyle_OSXOpaque)   ImGui::SetTooltip("%s", "\"OSXOpaque\"\nPosted by @dougbinks here:\nhttps://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9\n(hope I can use it)");
                    else if (styleEnumNum == ImGuiStyle_Soft) ImGui::SetTooltip("%s", "\"Soft\"\nPosted by @olekristensen here:\nhttps://github.com/ocornut/imgui/issues/539\n(hope I can use it)");
                    else if (styleEnumNum == ImGuiStyle_EdinBlack || styleEnumNum == ImGuiStyle_EdinWhite) ImGui::SetTooltip("%s", "Based on an image posted by @edin_p\n(hope I can use it)");
                    else if (styleEnumNum == ImGuiStyle_Maya) ImGui::SetTooltip("%s", "\"Maya\"\nPosted by @ongamex here:\nhttps://gist.github.com/ongamex/4ee36fb23d6c527939d0f4ba72144d29\n(hope I can use it)");
                }
                ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
#           endif // IMGUISTYLESERIALIZER_H_

                ImGui::TextDisabled("%s", "These are also present in the \"Preferences\" Panel:");
                ImGui::DragFloat("Window Alpha##WA2", &mgr.getDockedWindowsAlpha(), 0.005f, -0.01f, 1.0f, mgr.getDockedWindowsAlpha() < 0.0f ? "(default)" : "%.3f");
                ImGui::Spacing();
                bool border = mgr.getDockedWindowsBorder();
                if (ImGui::Checkbox("Window Borders##WB2", &border)) mgr.setDockedWindowsBorder(border); // Sets the window border to all the docked windows



            } else ImGui::Text("Here is the content of tab label: \"%s\"\n", tab->getLabel());
            ImGui::PopID();
        } else { ImGui::Text("EMPTY TAB LABEL DOCKING SPACE.");ImGui::Text("PLEASE DRAG AND DROP TAB LABELS HERE!"); }
        ImGui::Separator();ImGui::Spacing();
    }
    void TabLabelPopupMenuProvider(ImGui::TabWindow::TabLabel* tab, ImGui::TabWindow& parent, void* userPtr) {
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

#       if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION))
            ImGui::Separator();
#       ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
            if (ImGui::MenuItem("Save Layout")) SaveTabWindowsIfSupported();
#       endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#       ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
            if (ImGui::MenuItem("Load Layout")) LoadTabWindowsIfSupported();
#       endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#       endif //NO_IMGUIHELPER_SERIALIZATION

            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);
    }

    // These callbacks are not ImGui::TabWindow callbacks, but ImGui::PanelManager callbacks, set in AddTabWindowIfSupported(...) right below
    static void DrawDockedTabWindows(ImGui::PanelManagerWindowData& wd) {
        // See more generic DrawDockedWindows(...) below...
        ImGui::TabWindow& tabWindow = tabWindows[(wd.dockPos == ImGui::PanelManager::LEFT) ? 1 :
            (wd.dockPos == ImGui::PanelManager::RIGHT) ? 2 :
            (wd.dockPos == ImGui::PanelManager::TOP) ? 3
            : 4];
        tabWindow.render();
    }


    // This adds a tabWindows[i] to the matching Pane of the PanelManager (called in InitGL() AFAIR)
    void AddTabWindowIfSupported(ImGui::PanelManagerPane* pane) {
#ifndef NO_IMGUITABWINDOW
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
#endif //NO_IMGUITABWINDOW
    }

    // Here are refactored the load/save methods of the ImGui::PanelManager (mainly the hover and docked sizes of all windows and the button states of the 4 toolbars)
    // Please note that it should be possible to save settings this in the same as above ("myTabWindow.layout"), but the API is more complex.
    static const char panelManagerSaveName[] = "myPanelManager.layout";
    static const char panelManagerSaveNamePersistent[] = "/persistent_folder/myPanelManager.layout";  // Used by emscripten only, and only if YES_IMGUIEMSCRIPTENPERSISTENTFOLDER is defined (and furthermore it's buggy...).
    static bool LoadPanelManagerIfSupported() {
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
    static bool SavePanelManagerIfSupported() {
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

    // These variables/methods are used in InitGL()
    static const char* DockedWindowNames[] = { "Solution Explorer","Toolbox","Property Window","Find Window","Output Window","Application Output","Preferences" };
    static const char* ToggleWindowNames[] = { "Toggle Window 1","Toggle Window 2","Toggle Window 3","Toggle Window 4" };
    static void DrawDockedWindows(ImGui::PanelManagerWindowData& wd);   // defined below


    void Init(ImTextureID texture1, ImTextureID texture2, ImTextureID dockTexture) {
        myImageTextureId = texture1;
        myImageTextureId2 = texture2;
        ImGui::TabWindow::DockPanelIconTextureID = dockTexture;

#ifndef NO_IMGUITABWINDOW
        ImGui::TabWindow::SetWindowContentDrawerCallback(&TabContentProvider, NULL); // Mandatory
        ImGui::TabWindow::SetTabLabelPopupMenuDrawerCallback(&TabLabelPopupMenuProvider, NULL);  // Optional (if you need context-menu)
        ImGui::TabWindow::SetTabLabelGroupPopupMenuDrawerCallback(&TabLabelGroupPopupMenuProvider, NULL);    // Optional (fired when RMB is clicked on an empty spot in the tab area)
#endif //NO_IMGUITABWINDOW

#ifdef TEST_ICONS_INSIDE_TTF
#   ifndef NO_IMGUIFILESYSTEM  // Optional stuff to enhance file system dialogs with icons
        ImGuiFs::Dialog::DrawFileIconCallback = &MyFSDrawFileIconCb;
        ImGuiFs::Dialog::DrawFolderIconCallback = &MyFSDrawFolderIconCb;
#   endif //NO_IMGUIFILESYSTEM
#   if (defined(YES_IMGUIMINIGAMES) && !defined(NO_IMGUIMINIGAMES_MINE))
        ImGuiMiniGames::Mine::Style& mineStyle = ImGuiMiniGames::Mine::Style::Get();
        strcpy(mineStyle.characters[ImGuiMiniGames::Mine::Style::Character_Flag], ICON_FA_FLAG); // ICON_FA_FLAG_0 ICON_FA_FLAG_CHECKERED
        strcpy(mineStyle.characters[ImGuiMiniGames::Mine::Style::Character_Mine], ICON_FA_BOMB);
#   endif //if (defined(YES_IMGUIMINIGAMES) && !defined(NO_IMGUIMINIGAMES_MINE))
#endif //TEST_ICONS_INSIDE_TTF

#ifdef YES_IMGUIIMAGEEDITOR
        ImGui::ImageEditor::Style& ies = ImGui::ImageEditor::Style::Get();
        strcpy(&ies.arrowsChars[0][0], "◀");
        strcpy(&ies.arrowsChars[1][0], "▶");
        strcpy(&ies.arrowsChars[2][0], "▲");
        strcpy(&ies.arrowsChars[3][0], "▼");
#endif //YES_IMGUIIMAGEEDITOR

        // Here we setup mgr (our ImGui::PanelManager)
        if (mgr.isEmpty()) {
            // Hp) All the associated windows MUST have an unique name WITHOUT using the '##' chars that ImGui supports
            void* myImageTextureVoid = reinterpret_cast<void*>(myImageTextureId);         // 8x8 tiles
            void* myImageTextureVoid2 = reinterpret_cast<void*>(myImageTextureId2);       // 3x3 tiles
            ImVec2 uv0(0, 0), uv1(0, 0);int tileNumber = 0;

            // LEFT PANE
            {
                ImGui::PanelManager::Pane* pane = mgr.addPane(ImGui::PanelManager::LEFT, "myFirstToolbarLeft##foo");
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
                ImGui::PanelManager::Pane* pane = mgr.addPane(ImGui::PanelManager::RIGHT, "myFirstToolbarRight##foo");
                if (pane) {
                    // Here we use (a part of) the left pane to clone windows (handy since we don't support drag and drop):
                    if (mgr.getPaneLeft()) pane->addClonedPane(*mgr.getPaneLeft(), false, 0, 2); // note that only the "docked" part of buttons/windows are clonable ("manual" buttons are simply ignored): TO FIX: for now please avoid leaving -1 as the last argument, as this seems to mess up button indices: just explicitely copy NonTogglable-DockButtons yourself.
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
                ImGui::PanelManager::Pane* pane = mgr.addPane(ImGui::PanelManager::BOTTOM, "myFirstToolbarBottom##foo");
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
                ImGui::PanelManager::Pane* pane = mgr.addPane(ImGui::PanelManager::TOP, "myFirstToolbarTop##foo");
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
                    gpShowMainMenuBar = &pane->bar.getButton(pane->getSize() - 2)->isDown;            // [*]
                    gpShowCentralWindow = &pane->bar.getButton(pane->getSize() - 1)->isDown;          // [**]

                }
            }

            // Optional. Loads the layout (just selectedButtons, docked windows sizes and stuff like that)
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_LOAD))
            LoadPanelManagerIfSupported();
#   endif //!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !...


            // Optional line that affects the look of all the Toolbuttons in the Panes inserted so far:
            mgr.overrideAllExistingPanesDisplayProperties(ImVec2(0.25f, 0.9f), ImVec4(0.85, 0.85, 0.85, 1));

            // These line is only necessary to accomodate space for the global menu bar we're using:
            SetPanelManagerBoundsToIncludeMainMenuIfPresent();
        }

        // The following block used to be in DrawGL(), but it's better to move it here (it's part of the initalization)
#ifndef NO_IMGUITABWINDOW
        // Here we load all the Tabs, if their config file is available
        // Otherwise we create some Tabs in the central tabWindow (tabWindows[0])
        ImGui::TabWindow& tabWindow = tabWindows[0];
        if (!tabWindow.isInited()) {
            // tabWindow.isInited() becomes true after the first call to tabWindow.render() [in DrawGL()]
            for (int i = 0;i < 5;i++) tabWindows[i].clear();  // for robustness (they're already empty)
            if (!LoadTabWindowsIfSupported()) {
                // Here we set the starting configurations of the tabs in our ImGui::TabWindows
                static const char* tabNames[] = { "TabLabelStyle","Render","Layers","Capture","Scene","World","Object","Constraints","Modifiers","Data","Material","Texture","Particle","Physics" };
                static const int numTabs = sizeof(tabNames) / sizeof(tabNames[0]);
                static const char* tabTooltips[numTabs] = { "Edit the look of the tab labels","Render Tab Tooltip","Layers Tab Tooltip","Capture Tab Tooltip","non-draggable","Another Tab Tooltip","","","","non-draggable","Tired to add tooltips...","" };
                for (int i = 0;i < numTabs;i++) {
                    tabWindow.addTabLabel(tabNames[i], tabTooltips[i], i % 3 != 0, i % 5 != 4);
                }
#       ifdef YES_IMGUIMINIGAMES
#           ifndef NO_IMGUIMINIGAMES_MINE
                tabWindow.addTabLabel("ImGuiMineGame", "a mini-game", false, true, NULL, NULL, 0, ImGuiWindowFlags_NoScrollbar);
#           endif // NO_IMGUIMINIGAMES_MINE
#           ifndef NO_IMGUIMINIGAMES_SUDOKU
                tabWindow.addTabLabel("ImGuiSudokuGame", "a mini-game", false, true, NULL, NULL, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
#           endif // NO_IMGUIMINIGAMES_SUDOKU
#       endif // YES_IMGUIMINIGAMES
#       ifdef YES_IMGUIIMAGEEDITOR
                tabWindow.addTabLabel("ImGuiImageEditor", "a tiny image editor", false, true, NULL, NULL, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
#       endif //YES_IMGUIIMAGEEDITOR
#       ifndef NO_IMGUISTYLESERIALIZER
                tabWindow.addTabLabel("ImGuiStyleChooser", "Edit the look of the GUI", false);
#       endif //NO_IMGUISTYLESERIALIZER
            }
        }
#endif // NO_IMGUITABWINDOW
    }

        // These "ShowExampleMenu..." methods are just copied and pasted from imgui_demo.cpp
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


        void Resize(int w, int h)	// Mandatory
        {
            //fprintf(stderr,"ResizeGL(%d,%d); ImGui::DisplaySize(%d,%d);\n",w,h,(int)ImGui::GetIO().DisplaySize.x,(int)ImGui::GetIO().DisplaySize.y);
            static ImVec2 initialSize(w, h);
            mgr.setToolbarsScaling((float)w / initialSize.x, (float)h / initialSize.y);  // Scales the PanelManager bounmds based on the initialSize
            SetPanelManagerBoundsToIncludeMainMenuIfPresent(w, h);                  // This line is only necessary if we have a global menu bar
        }

        void DrawDockedWindows(ImGui::PanelManagerWindowData& wd) {
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
                    ImGui::DragFloat("Window Alpha##WA1", &mgr.getDockedWindowsAlpha(), 0.005f, -0.01f, 1.0f, mgr.getDockedWindowsAlpha() < 0.0f ? "(default)" : "%.3f");
                    bool border = mgr.getDockedWindowsBorder();
                    if (ImGui::Checkbox("Window Borders", &border)) mgr.setDockedWindowsBorder(border); // Sets the window border to all the docked windows
                    ImGui::SameLine();
                    bool noTitleBar = mgr.getDockedWindowsNoTitleBar();
                    if (ImGui::Checkbox("No Window TitleBars", &noTitleBar)) mgr.setDockedWindowsNoTitleBar(noTitleBar);
                    if (gpShowCentralWindow) { ImGui::SameLine();ImGui::Checkbox("Show Central Wndow", (bool*)gpShowCentralWindow); }
                    if (gpShowMainMenuBar) {
                        ImGui::SameLine();
                        if (ImGui::Checkbox("Show Main Menu", (bool*)gpShowMainMenuBar)) SetPanelManagerBoundsToIncludeMainMenuIfPresent();
                    }
                    // Here we test saving/loading the ImGui::PanelManager layout (= the sizes of the 4 docked windows and the buttons that are selected on the 4 toolbars)
                    // Please note that the API should allow loading/saving different items into a single file and loading/saving from/to memory too, but we don't show it now.
#           if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION))
                    ImGui::Separator();
                    static const char pmTooltip[] = "the ImGui::PanelManager layout\n(the sizes of the 4 docked windows and\nthe buttons that are selected\non the 4 toolbars)";
#           ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
                    if (ImGui::Button("Save Panel Manager Layout")) SavePanelManagerIfSupported();
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save %s", pmTooltip);
#           endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#           ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
                    ImGui::SameLine();if (ImGui::Button("Load Panel Manager Layout")) {
                        if (LoadPanelManagerIfSupported())   SetPanelManagerBoundsToIncludeMainMenuIfPresent();	// That's because we must adjust gpShowMainMenuBar state here (we have used a manual toggle button for it)
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


        void Draw()	// Mandatory
        {
            if (gpShowMainMenuBar && *gpShowMainMenuBar) ShowExampleMenuBar(true);

            // Actually the following "if block" that displays the Central Window was placed at the very bottom (after mgr.render),
            // but I've discovered that if I set some "automatic toggle window" to be visible at startup, it would be covered by the central window otherwise.
            // Update: no matter, we can use the ImGuiWindowFlags_NoBringToFrontOnFocus flag for that if we want.
            if (gpShowCentralWindow && *gpShowCentralWindow) {
                const ImVec2& iqs = mgr.getCentralQuadSize();
                if (iqs.x>ImGui::GetStyle().WindowMinSize.x && iqs.y>ImGui::GetStyle().WindowMinSize.y) {
                    ImGui::SetNextWindowPos(mgr.getCentralQuadPosition());
                    ImGui::SetNextWindowSize(mgr.getCentralQuadSize());
                    if (ImGui::Begin("Central Window", NULL, ImVec2(0, 0), mgr.getDockedWindowsAlpha(), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | mgr.getDockedWindowsExtraFlags() /*| ImGuiWindowFlags_NoBringToFrontOnFocus*/)) {
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
            if (mgr.render(&pressedPane, &pressedPaneButtonIndex)) {
                //const ImVec2& iqp = mgr.getCentralQuadPosition();
                //const ImVec2& iqs = mgr.getCentralQuadSize();
                //fprintf(stderr,"Inner Quad Size changed to {%1.f,%1.f,%1.f,%1.f}\n",iqp.x,iqp.y,iqs.x,iqs.y);
            }

            // (Optional) Some manual feedback to the user (actually I detect gpShowMainMenuBar pressures here too, but pleese read below...):
            if (pressedPane && pressedPaneButtonIndex != -1)
            {
                static const char* paneNames[] = { "LEFT","RIGHT","TOP","BOTTOM" };
                if (!pressedPane->getWindowName(pressedPaneButtonIndex)) {
                    ImGui::Toolbutton* pButton = NULL;
                    pressedPane->getButtonAndWindow(pressedPaneButtonIndex, &pButton);
                    if (pButton->isToggleButton) {
                        printf("Pressed manual toggle button (number: %d on pane: %s)\n", pressedPaneButtonIndex, paneNames[pressedPane->pos]);
                        if (pressedPane->pos == ImGui::PanelManager::TOP && pressedPaneButtonIndex == (int)pressedPane->getSize() - 2) {
                            // For this we could have just checked if *gpShowMainMenuBar had changed its value before and after mgr.render()...
                            SetPanelManagerBoundsToIncludeMainMenuIfPresent();
                        }
                    } else printf("Pressed manual button (number: %d on pane: %s)\n", pressedPaneButtonIndex, paneNames[pressedPane->pos]);
                    fflush(stdout);
                } else {
                    ImGui::Toolbutton* pButton = NULL;
                    pressedPane->getButtonAndWindow(pressedPaneButtonIndex, &pButton);
                    if (pButton->isToggleButton) printf("Pressed toggle button (number: %d on pane: %s)\n", pressedPaneButtonIndex, paneNames[pressedPane->pos]);
                    else printf("Pressed dock button (number: %d on pane: %s)\n", pressedPaneButtonIndex, paneNames[pressedPane->pos]);
                    fflush(stdout);
                }

            }
        }

#   define USE_ADVANCED_SETUP
}; //namespace Divide