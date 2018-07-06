#include <imgui.h>

ImTextureID myImageTextureId = 0;
ImTextureID myImageTextureId2 = 0;

static ImGui::PanelManager mgr;
static const bool* gpShowMainMenuBar = NULL;      // We'll bind it to a "manual" toggle button of mgr in InitGL().
static const bool* gpShowCentralWindow = NULL;    // We'll bind it to a "manual" toggle button of mgr in InitGL().

// Tweakable definitions
//#define NO_IMGUITABWINDOW         // Optional (but useful to learn ImGui::PanelManager only). [Users usually define this kind of definitions at the project level].
//#define NO_IMGUIHELPER	    // Disables all saving/loading methods. [Users usually define this kind of definitions at the project level].
//#define TEST_ICONS_INSIDE_TTF     // Optional to test FontAwesome (a ttf file containing icons) [Local definition]


#ifdef TEST_ICONS_INSIDE_TTF
#include "fonts/Icons/FontAwesome4/definitions.h"
static void DrawAllFontAwesomeIcons();  // defined at the bottom of this file

#ifndef NO_IMGUIFILESYSTEM  // Optional stuff to enhance file system dialogs with icons
static bool MyFSDrawFileIconCb(int extensionType,const ImVec4* pOptionalColorOverride); // defined at the bottom of this file
static bool MyFSDrawFolderIconCb(bool useOpenFolderIconIfAvailable,const ImVec4* pOptionalColorOverride); // defined at the bottom of this file
#endif //NO_IMGUIFILESYSTEM

// Completely optional styled checkboxes by dougbinks (copied and pasted from: https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9)
// They seem to work better with monospace fonts (unluckily we don't use them in this demo)
namespace ImGui {
    inline bool CheckBoxFont( const char* name_, bool* pB_, const char* pOn_ = "[X]", const char* pOff_="[  ]" )    {
        if( *pB_ )ImGui::Text("%s",pOn_);
        else ImGui::Text("%s",pOff_);
        bool bHover = false;
        bHover = bHover || ImGui::IsItemHovered();
        ImGui::SameLine();
        ImGui::Text("%s", name_ );
        bHover = bHover || ImGui::IsItemHovered();
        if( bHover && ImGui::IsMouseClicked(0) )    {
            *pB_ = ! *pB_;
            return true;
        }
        return false;
    }
    inline bool CheckBoxTick( const char* name_, bool* pB_ )    {
        return CheckBoxFont( name_, pB_, ICON_FA_CHECK_SQUARE_O, ICON_FA_SQUARE_O );
    }
    inline bool MenuItemCheckBox( const char* name_, bool* pB_ )    {
        bool retval = ImGui::MenuItem( name_ );
        ImGui::SameLine();
        if( *pB_ )  ImGui::Text(ICON_FA_CHECK_SQUARE_O);
        else    ImGui::Text(ICON_FA_SQUARE_O);
        if( retval ) *pB_ = ! *pB_;
        return retval;
    }
}
#endif //TEST_ICONS_INSIDE_TTF

static const ImVec4 gDefaultClearColor(0.8f, 0.6f, 0.6f, 1.0f);
static ImVec4 gClearColor = gDefaultClearColor;

// Here are two static methods useful to handle the change of size of the togglable mainMenu we will use
// Returns the height of the main menu based on the current font (from: ImGui::CalcMainMenuHeight() in imguihelper.h)
// However according to https://github.com/ocornut/imgui/issues/252 this approach can fail [Better call ImGui::GetWindowSize().y from inside the menu and store the result somewhere]
inline static float CalcMainMenuHeight() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    ImFont* font = ImGui::GetFont();
    if (!font) {
        if (io.Fonts->Fonts.size()>0) font = io.Fonts->Fonts[0];
        else return (14)+style.FramePadding.y * 2.0f;
    }
    return (io.FontGlobalScale * font->Scale * font->FontSize) + style.FramePadding.y * 2.0f;
}
inline static void SetPanelManagerBoundsToIncludeMainMenuIfPresent(int displayX=-1, int displayY=-1)  {
    if (gpShowMainMenuBar)  {
    if (displayX<=0) displayX = ImGui::GetIO().DisplaySize.x;
    if (displayY<=0) displayY = ImGui::GetIO().DisplaySize.y;
    ImVec4 bounds(0,0,(float)displayX,(float)displayY);   // (0,0,-1,-1) defaults to (0,0,io.DisplaySize.x,io.DisplaySize.y)
        if (*gpShowMainMenuBar) {
            const float mainMenuHeight = CalcMainMenuHeight();
            bounds = ImVec4(0,mainMenuHeight,displayX,displayY-mainMenuHeight);
        }
        mgr.setDisplayPortion(bounds);
    }
}


#ifndef NO_IMGUITABWINDOW
ImGui::TabWindow tabWindows[5]; // 0 = center, 1 = left, 2 = right, 3 = top, 4 = bottom

// Static methods to load/save all the tabWindows (done mainly to avoid spreading too many definitions around)
static const char tabWindowsSaveName[]           = "myTabWindow.layout";
static const char tabWindowsSaveNamePersistent[] = "/persistent_folder/myTabWindow.layout";  // Used by emscripten only, and only if YES_IMGUIEMSCRIPTENPERSISTENTFOLDER is defined (and furthermore it's buggy...).
static bool LoadTabWindowsIfSupported() {
    bool loadedFromFile = false;
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_LOAD))
    const char* pSaveName = tabWindowsSaveName;
#   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
    if (ImGuiHelper::FileExists(tabWindowsSaveNamePersistent)) pSaveName = tabWindowsSaveNamePersistent;
#   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
    //loadedFromFile = tabWindow.load(pSaveName);   // This is good for a single TabWindow
    loadedFromFile = ImGui::TabWindow::Load(pSaveName,&tabWindows[0],sizeof(tabWindows)/sizeof(tabWindows[0]));  // This is OK for a multiple TabWindows
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
    if (ImGui::TabWindow::Save(pSaveName,&tabWindows[0],sizeof(tabWindows)/sizeof(tabWindows[0])))  // This is OK for a multiple TabWindows
    {
#   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
    ImGui::EmscriptenFileSystemHelper::Sync();
#   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
    return true;
    }
#   endif //!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && ...
    return false;
}

// Callbacks used by all ImGui::TabWindows and set in InitGL()
void TabContentProvider(ImGui::TabWindow::TabLabel* tab,ImGui::TabWindow& parent,void* userPtr) {
    // Users will use tab->userPtr here most of the time
    ImGui::Spacing();ImGui::Separator();
    if (tab) {
        ImGui::PushID(tab);
        if (tab->matchLabel("TabLabelStyle"))  {
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
                ImGui::TabLabelStyle::Save(ImGui::TabLabelStyle::Get(),pSaveName);
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
                ImGui::TabLabelStyle::Load(ImGui::TabLabelStyle::Get(),pSaveName);
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
        else if (tab->matchLabel("ImGuiMineGame"))  {
#           if (defined(YES_IMGUIMINIGAMES) && !defined(NO_IMGUIMINIGAMES_MINE))
            static ImGuiMiniGames::Mine mineGame;
            mineGame.render();
#           else //NO_IMGUIMINIGAMES_MINE
            ImGui::Text("Disabled for this build.");
#           endif // NO_IMGUIMINIGAMES_MINE
        }
        else if (tab->matchLabel("ImGuiSudokuGame"))  {
#           if (defined(YES_IMGUIMINIGAMES) && !defined(NO_IMGUIMINIGAMES_SUDOKU))
            static ImGuiMiniGames::Sudoku sudokuGame;
            sudokuGame.render();
#           else // NO_IMGUIMINIGAMES_SUDOKU
            ImGui::Text("Disabled for this build.");
#           endif // NO_IMGUIMINIGAMES_SUDOKU
        }
        else if (tab->matchLabel("ImGuiFifteenGame"))  {
#           if (defined(YES_IMGUIMINIGAMES) && !defined(NO_IMGUIMINIGAMES_FIFTEEN))
            static ImGuiMiniGames::Fifteen fifteenGame;
            fifteenGame.render();
#           else // NO_IMGUIMINIGAMES_FIFTEEN
            ImGui::Text("Disabled for this build.");
#           endif // NO_IMGUIMINIGAMES_FIFTEEN
        }
        else if (tab->matchLabel("ImGuiImageEditor"))   {
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
        }
        else if (tab->matchLabel("ImGuiStyleChooser")) {
#           ifndef IMGUISTYLESERIALIZER_H_
            ImGui::Text("GUI Style Chooser Disabled for this build.");
            ImGui::Spacing();ImGui::Separator();
#           endif //IMGUISTYLESERIALIZER_H_

            ImGui::Spacing();
            ImGui::TextDisabled("%s","Some controls to change the GUI style:");
            ImGui::PushItemWidth(275);
            if (ImGui::DragFloat("Global Font Scale", &ImGui::GetIO().FontGlobalScale, 0.005f, 0.3f, 2.0f, "%.2f")) SetPanelManagerBoundsToIncludeMainMenuIfPresent();  // This is because the Main Menu height changes with the Font Scale
            ImGui::PopItemWidth();
            if (ImGui::GetIO().FontGlobalScale!=1.f)    {
                ImGui::SameLine(0,10);
                if (ImGui::SmallButton("Reset##glFontGlobalScale")) {
                    ImGui::GetIO().FontGlobalScale = 1.f;
                    SetPanelManagerBoundsToIncludeMainMenuIfPresent();  // This is because the Main Menu height changes with the Font Scale
                }
            }
            ImGui::Spacing();

            ImGui::PushItemWidth(275);
            ImGui::ColorEdit3("glClearColor",&gClearColor.x);
            ImGui::PopItemWidth();
            if (gClearColor.x!=gDefaultClearColor.x || gClearColor.y!=gDefaultClearColor.y || gClearColor.z!=gDefaultClearColor.z)    {
                ImGui::SameLine(0,10);
                if (ImGui::SmallButton("Reset##glClearColorReset")) gClearColor = gDefaultClearColor;
            }
            ImGui::Spacing();

#           ifdef IMGUISTYLESERIALIZER_H_
            static int styleEnumNum = 1;
            ImGui::PushItemWidth(ImGui::GetWindowWidth()*0.44f);
            if (ImGui::Combo("Main Style Chooser",&styleEnumNum,ImGui::GetDefaultStyleNames(),(int) ImGuiStyle_Count,(int) ImGuiStyle_Count)) {
                ImGui::ResetStyle(styleEnumNum);
            }
            ImGui::PopItemWidth();
            if (ImGui::IsItemHovered()) {
                if   (styleEnumNum==ImGuiStyle_DefaultClassic)      ImGui::SetTooltip("%s","\"Default\"\nThis is the default\nclassic ImGui theme");
                else if (styleEnumNum==ImGuiStyle_DefaultDark)      ImGui::SetTooltip("%s","\"DefaultDark\"\nThis is the default\ndark ImGui theme");
                else if (styleEnumNum==ImGuiStyle_DefaultLight)      ImGui::SetTooltip("%s","\"DefaultLight\"\nThis is the default\nlight ImGui theme");
                else if (styleEnumNum==ImGuiStyle_Gray)   ImGui::SetTooltip("%s","\"Gray\"\nThis is the default theme of first demo");
                else if (styleEnumNum==ImGuiStyle_OSX)   ImGui::SetTooltip("%s","\"OSX\"\nPosted by @itamago here:\nhttps://github.com/ocornut/imgui/pull/511\n(hope I can use it)");
                else if (styleEnumNum==ImGuiStyle_DarkOpaque)   ImGui::SetTooltip("%s","\"DarkOpaque\"\nA dark-grayscale style with\nno transparency (by default)");
                else if (styleEnumNum==ImGuiStyle_OSXOpaque)   ImGui::SetTooltip("%s","\"OSXOpaque\"\nPosted by @dougbinks here:\nhttps://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9\n(hope I can use it)");
                else if (styleEnumNum==ImGuiStyle_Soft) ImGui::SetTooltip("%s","\"Soft\"\nPosted by @olekristensen here:\nhttps://github.com/ocornut/imgui/issues/539\n(hope I can use it)");
                else if (styleEnumNum==ImGuiStyle_EdinBlack || styleEnumNum==ImGuiStyle_EdinWhite) ImGui::SetTooltip("%s","Based on an image posted by @edin_p\n(hope I can use it)");
                else if (styleEnumNum==ImGuiStyle_Maya) ImGui::SetTooltip("%s","\"Maya\"\nPosted by @ongamex here:\nhttps://gist.github.com/ongamex/4ee36fb23d6c527939d0f4ba72144d29\n(hope I can use it)");
            }
            ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
#           endif // IMGUISTYLESERIALIZER_H_

            ImGui::TextDisabled("%s","These are also present in the \"Preferences\" Panel:");
            ImGui::DragFloat("Window Alpha##WA2", &mgr.getDockedWindowsAlpha(), 0.005f, -0.01f, 1.0f, mgr.getDockedWindowsAlpha() < 0.0f ? "(default)" : "%.3f");



        }
        else if (tab->userInt>=500) {
#       if (!defined(NO_IMGUICODEEDITOR) && !defined(NO_IMGUIFILESYSTEM))
            // This is simply an experimental mess!
            // However I need some kind of spot to further test ImGui::InputTextWithSyntaxHighlighting(...).
            // Note that this test only works for an instance and we never SAVE any modified file!

            static ImGuiID codeEditorID = 0;
            static ImString codeEditorText = "__MUST_INIT__";
            static bool fileNotPresent = false;

            ImGuiInputTextFlags codeEditorFlags = 0;
            const char* chosenPath = "";
            const bool browseButtonPressed = ImGui::Button("Load###LoadCodeEditorFile");

            bool mustReload = false;    // Well, we must find a way to detect when we load the whole TabWindow layout and at that moment reload the (new?) file
            static char lastTabLabelName[ImGuiFs::MAX_PATH_BYTES]="";
            if (!tab->matchLabel(lastTabLabelName)) {
                tab->copyLabelTo(lastTabLabelName,ImGuiFs::MAX_PATH_BYTES);
                IM_ASSERT(tab->matchLabel(lastTabLabelName));
                mustReload = true;
                //fprintf(stderr,"No match with \"%s\"\n",tab->getLabel());
            }

            if ((codeEditorID==0 && codeEditorText=="__MUST_INIT__") || mustReload) {
                codeEditorText="";
                chosenPath = tab->getTooltip();
            }
            else {
                static ImGuiFs::Dialog fsInstance;
                chosenPath = fsInstance.chooseFileDialog(browseButtonPressed,tab->getTooltip(),ImGuiCe::GetSupportedExtensions());
            }

            if (strlen(chosenPath)>0) {
                // A path (chosenPath) has been chosen right now. However we can retrieve it later using: fsInstance.getChosenPath()
                if (chosenPath!=tab->getTooltip()) tab->setTooltip(chosenPath);
                char relativePath[ImGuiFs::MAX_PATH_BYTES]="";
                ImGuiFs::PathGetFileName(chosenPath,relativePath);
                tab->setLabel(relativePath);
                tab->setModified(false);
                fileNotPresent = !ImGuiFs::FileExists(chosenPath);
                if (!fileNotPresent) {
                    ImVector<char> tmp;ImGuiFs::FileGetContent(chosenPath,tmp,true);
                    if (tmp.size()>0) codeEditorText = &tmp[0];
                    ImGuiFs::PathGetExtension(chosenPath,relativePath); // relativePath now is ".cpp" or something like that
                    tab->userInt=500+(int)ImGuiCe::GetLanguageFromExtension(relativePath);
                    codeEditorFlags = ImGuiInputTextFlags_ResetText;
                    //fprintf(stderr,"Loading \"%s\"\n",tab->getTooltip());
                }
            }

            ImGui::SameLine();ImGui::Text("%s",ImGuiCe::GetLanguageNames()[tab->userInt-500]);

            if (fileNotPresent) {
                ImGui::Text("Error: \"%s\" Not present.",tab->getTooltip());
            }
            else if (ImGui::InputTextWithSyntaxHighlighting(codeEditorID,codeEditorText,(ImGuiCe::Language) (tab->userInt-500),ImVec2(0,-1),codeEditorFlags)) tab->setModified(true);
#       else //NO_IMGUICODEEDITOR || NO_IMGUIFILESYSTEM
        ImGui::Text("Disabled for this build.");
#       endif //NO_IMGUICODEEDITOR || NO_IMGUIFILESYSTEM
        }
        else if (tab->matchLabelExtension(".pdf"))  {
#       if (defined(YES_IMGUIPDFVIEWER) && !defined(NO_IMGUIFILESYSTEM))
            // This is simply an experimental mess!
            // However I need some kind of spot to further test ImGui::InputTextWithSyntaxHighlighting(...).
            // Note that this test only works for an instance and we never SAVE any modified file!

            static ImGui::PdfViewer pdfViewer;
            static bool fileNotPresent = false;

            const char* chosenPath = "";
            const bool browseButtonPressed = ImGui::Button("Load###LoadPdfFile");

            bool mustReload = false;    // Well, we must find a way to detect when we load the whole TabWindow layout and at that moment reload the (new?) file
            static char lastTabLabelName[ImGuiFs::MAX_PATH_BYTES]="";
            if (!tab->matchLabel(lastTabLabelName)) {
                tab->copyLabelTo(lastTabLabelName,ImGuiFs::MAX_PATH_BYTES);
                IM_ASSERT(tab->matchLabel(lastTabLabelName));
                mustReload = true;
                //fprintf(stderr,"No match with \"%s\"\n",tab->getLabel());
            }

            if (!pdfViewer.isInited() || mustReload) chosenPath = tab->getTooltip();
            else {
                static ImGuiFs::Dialog fsInstance;
                chosenPath = fsInstance.chooseFileDialog(browseButtonPressed,tab->getTooltip(),".pdf");
            }

            if (strlen(chosenPath)>0) {
                // A path (chosenPath) has been chosen right now. However we can retrieve it later using: fsInstance.getChosenPath()
                if (chosenPath!=tab->getTooltip()) tab->setTooltip(chosenPath);
                char relativePath[ImGuiFs::MAX_PATH_BYTES]="";
                ImGuiFs::PathGetFileName(chosenPath,relativePath);
                tab->setLabel(relativePath);
                tab->setModified(false);
                fileNotPresent = !ImGuiFs::FileExists(chosenPath);
                if (!fileNotPresent) {
                    pdfViewer.loadFromFile(chosenPath);
                    //fprintf(stderr,"Loading \"%s\"\n",tab->getTooltip());
                }
            }

            if (fileNotPresent) {
                ImGui::Text("Error: \"%s\" Not present.",tab->getTooltip());
            }
            else {ImGui::SameLine();pdfViewer.render();}    // ImGui::SameLine() is just to accomodate our Load Button
#       else //YES_IMGUIPDFVIEWER || NO_IMGUIFILESYSTEM
        ImGui::Text("Disabled for this build.");
#       endif //YES_IMGUIPDFVIEWER || NO_IMGUIFILESYSTEM
        }
        else ImGui::Text("Here is the content of tab label: \"%s\"\n",tab->getLabel());
        ImGui::PopID();
    }
    else {ImGui::Text("EMPTY TAB LABEL DOCKING SPACE.");ImGui::Text("PLEASE DRAG AND DROP TAB LABELS HERE!");}
    ImGui::Separator();ImGui::Spacing();
}
void TabLabelPopupMenuProvider(ImGui::TabWindow::TabLabel* tab,ImGui::TabWindow& parent,void* userPtr) {
    if (ImGui::BeginPopup(ImGui::TabWindow::GetTabLabelPopupMenuName()))   {
        ImGui::PushID(tab);
        ImGui::Text("\"%.*s\" Menu",(int)(strlen(tab->getLabel())-(tab->getModified()?1:0)),tab->getLabel());
        ImGui::Separator();
        if (!tab->matchLabel("TabLabelStyle") && !tab->matchLabel("Style"))   {
            if (tab->getModified()) {if (ImGui::MenuItem("Mark as not modified")) tab->setModified(false);}
            else                    {if (ImGui::MenuItem("Mark as modified"))     tab->setModified(true);}
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
void TabLabelGroupPopupMenuProvider(ImVector<ImGui::TabWindow::TabLabel*>& tabs,ImGui::TabWindow& parent,ImGui::TabWindowNode* tabNode,void* userPtr) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImGui::ColorConvertU32ToFloat4(ImGui::TabLabelStyle::Get().colors[ImGui::TabLabelStyle::Col_TabLabel]));
    ImGui::PushStyleColor(ImGuiCol_Text,ImGui::ColorConvertU32ToFloat4(ImGui::TabLabelStyle::Get().colors[ImGui::TabLabelStyle::Col_TabLabelText]));
    if (ImGui::BeginPopup(ImGui::TabWindow::GetTabLabelGroupPopupMenuName()))   {
        ImGui::Text("TabLabel Group Menu");
        ImGui::Separator();
        if (parent.isMergeble(tabNode) && ImGui::MenuItem("Merge with parent group")) parent.merge(tabNode); // Warning: this invalidates "tabNode" after the call
        if (ImGui::MenuItem("Close all tabs in this group")) {
            for (int i=0,isz=tabs.size();i<isz;i++) {
                ImGui::TabWindow::TabLabel* tab = tabs[i];
                if (tab->isClosable())  // otherwise even non-closable tabs will be closed
                {
                    //parent.removeTabLabel(tab);
                    tab->mustCloseNextFrame = true;  // alternative way... this asks for saving if file is modified
                }
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save all tabs in all groups")) {parent.saveAll();}
        if (ImGui::MenuItem("Close all tabs in all groups")) {
            // This methods fires a modal dialog if we have unsaved files. To prevent this, we can try calling: parent.saveAll(); before
            parent.startCloseAllDialog(NULL,true);  // returns true when a modal dialog is stated
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
static void DrawDockedTabWindows(ImGui::PanelManagerWindowData& wd)    {
    // See more generic DrawDockedWindows(...) below...
    ImGui::TabWindow& tabWindow = tabWindows[(wd.dockPos==ImGui::PanelManager::LEFT)?1:
                                            (wd.dockPos==ImGui::PanelManager::RIGHT)?2:
                                            (wd.dockPos==ImGui::PanelManager::TOP)?3
                                            :4];
    tabWindow.render();
}
#endif //NO_IMGUITABWINDOW

// This adds a tabWindows[i] to the matching Pane of the PanelManager (called in InitGL() AFAIR)
void AddTabWindowIfSupported(ImGui::PanelManagerPane* pane) {
#ifndef NO_IMGUITABWINDOW
    const ImTextureID texId = ImGui::TabWindow::DockPanelIconTextureID;
    IM_ASSERT(pane && texId);
    ImVec2 buttonSize(32,32);
    static const char* names[4] = {"TabWindow Left","TabWindow Right","TabWindow Top", "TabWindow Bottom"};
    const int index = (pane->pos == ImGui::PanelManager::LEFT)?0:(pane->pos == ImGui::PanelManager::RIGHT)?1:(pane->pos == ImGui::PanelManager::TOP)?2:3;
    if (index<2) buttonSize.x=24;
    const int uvIndex = (index==0)?3:(index==2)?0:(index==3)?2:index;
    ImVec2 uv0(0.75f,(float)uvIndex*0.25f),uv1(uv0.x+0.25f,uv0.y+0.25f);
    pane->addButtonAndWindow(ImGui::Toolbutton(names[index],texId,uv0,uv1,buttonSize),              // the 1st arg of Toolbutton is only used as a text for the tooltip.
                ImGui::PanelManagerPaneAssociatedWindow(names[index],-1,&DrawDockedTabWindows,NULL,ImGuiWindowFlags_NoScrollbar));    //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
#endif //NO_IMGUITABWINDOW
}

// Here are refactored the load/save methods of the ImGui::PanelManager (mainly the hover and docked sizes of all windows and the button states of the 4 toolbars)
// Please note that it should be possible to save settings this in the same as above ("myTabWindow.layout"), but the API is more complex.
static const char panelManagerSaveName[] = "myPanelManager.layout";
static const char panelManagerSaveNamePersistent[] = "/persistent_folder/myPanelManager.layout";  // Used by emscripten only, and only if YES_IMGUIEMSCRIPTENPERSISTENTFOLDER is defined (and furthermore it's buggy...).
static bool LoadPanelManagerIfSupported() {
    bool loadingOk=false;
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_LOAD))
    const char* pSaveName = panelManagerSaveName;
#   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
    if (ImGuiHelper::FileExists(panelManagerSaveNamePersistent)) pSaveName = panelManagerSaveNamePersistent;
#   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
    loadingOk=ImGui::PanelManager::Load(mgr,pSaveName);
#   endif //!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !...
    return loadingOk;
}
static bool SavePanelManagerIfSupported() {
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_SAVE))
    const char* pSaveName = panelManagerSaveName;
#   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
    pSaveName = panelManagerSaveNamePersistent;
#   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
    if (ImGui::PanelManager::Save(mgr,pSaveName))   {
#	ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
	ImGui::EmscriptenFileSystemHelper::Sync();
#	endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
    return true;
    }
#   endif //!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !...
    return false;
}

// These variables/methods are used in InitGL()
static const char* DockedWindowNames[] = {"Solution Explorer","Toolbox","Property Window","Find Window","Output Window","Application Output","Preferences"};
static const char* ToggleWindowNames[] = {"Toggle Window 1","Toggle Window 2","Toggle Window 3","Toggle Window 4"};
static void DrawDockedWindows(ImGui::PanelManagerWindowData& wd);   // defined below


void InitGL()	// Mandatory
{
/*
if (!ImGui::LoadStyle("./myimgui.style",ImGui::GetStyle()))   {
    fprintf(stderr,"Warning: \"./myimgui.style\" not present.\n");
}
*/
// This is something that does not work properly with all the addons:
//ImGui::GetIO().NavFlags |= ImGuiNavFlags_EnableKeyboard;


if (!myImageTextureId) myImageTextureId = ImImpl_LoadTexture("./Tile8x8.png");
if (!myImageTextureId2) myImageTextureId2 = ImImpl_LoadTexture("./myNumbersTexture.png");

#ifndef NO_IMGUITABWINDOW
    ImGui::TabWindow::SetWindowContentDrawerCallback(&TabContentProvider,NULL); // Mandatory
    ImGui::TabWindow::SetTabLabelPopupMenuDrawerCallback(&TabLabelPopupMenuProvider,NULL);  // Optional (if you need context-menu)
    ImGui::TabWindow::SetTabLabelGroupPopupMenuDrawerCallback(&TabLabelGroupPopupMenuProvider,NULL);    // Optional (fired when RMB is clicked on an empty spot in the tab area)
#endif //NO_IMGUITABWINDOW

#ifdef TEST_ICONS_INSIDE_TTF
#   ifndef NO_IMGUIFILESYSTEM  // Optional stuff to enhance file system dialogs with icons
    ImGuiFs::Dialog::DrawFileIconCallback = &MyFSDrawFileIconCb;
    ImGuiFs::Dialog::DrawFolderIconCallback = &MyFSDrawFolderIconCb;
#   endif //NO_IMGUIFILESYSTEM
#   if (defined(YES_IMGUIMINIGAMES) && !defined(NO_IMGUIMINIGAMES_MINE))
    ImGuiMiniGames::Mine::Style& mineStyle = ImGuiMiniGames::Mine::Style::Get();
    strcpy(mineStyle.characters[ImGuiMiniGames::Mine::Style::Character_Flag],ICON_FA_FLAG); // ICON_FA_FLAG_0 ICON_FA_FLAG_CHECKERED
    strcpy(mineStyle.characters[ImGuiMiniGames::Mine::Style::Character_Mine],ICON_FA_BOMB);
#   endif //if (defined(YES_IMGUIMINIGAMES) && !defined(NO_IMGUIMINIGAMES_MINE))
#endif //TEST_ICONS_INSIDE_TTF

#ifdef YES_IMGUIIMAGEEDITOR
ImGui::ImageEditor::Style& ies = ImGui::ImageEditor::Style::Get();
strcpy(&ies.arrowsChars[0][0],"◀");
strcpy(&ies.arrowsChars[1][0],"▶");
strcpy(&ies.arrowsChars[2][0],"▲");
strcpy(&ies.arrowsChars[3][0],"▼");
#endif //YES_IMGUIIMAGEEDITOR

#if (!defined(NO_IMGUICODEEDITOR) && !defined(NO_IMGUIFILESYSTEM))
{
    // We use (at least one) monopace font for our code editor. We add font number 1 (see main() function below)
    // When using more than one monospace font, all must be of the same size

    // AFAIR a SINGLE non-monospace font is allowed as long as we always pass in the same font (as in this case, but we're using a monospace font),
    // otherwise we can use 4 monospace fonts (one for Regular,Bold,Italic,BoldItalic).

    // In any case all the fonts must be of the same size
    ImFontAtlas* atlas = ImGui::GetIO().Fonts;
    IM_ASSERT(atlas->Fonts.Size>1);
    ImFont* font = atlas->Fonts[1];
    ImGuiCe::CodeEditor::SetFonts(font,font,font,font);
}
#endif

// Here we setup mgr (our ImGui::PanelManager)
if (mgr.isEmpty()) {
    // Hp) All the associated windows MUST have an unique name WITHOUT using the '##' chars that ImGui supports
    void* myImageTextureVoid = reinterpret_cast<void*>(myImageTextureId);         // 8x8 tiles
    void* myImageTextureVoid2 = reinterpret_cast<void*>(myImageTextureId2);       // 3x3 tiles
    ImVec2 uv0(0,0),uv1(0,0);int tileNumber=0;

    // LEFT PANE
    {
        ImGui::PanelManager::Pane* pane = mgr.addPane(ImGui::PanelManager::LEFT,"myFirstToolbarLeft##foo");
        if (pane)   {
            // Here we add the "proper" docked buttons and windows:
            const ImVec2 buttonSize(24,32);
            for (int i=0;i<3;i++)   {
                // Add to left pane the first 3 windows DrawDockedWindows[i], with Toolbuttons with the first 3 images of myImageTextureVoid (8x8 tiles):
                tileNumber=i;uv0=ImVec2((float)(tileNumber%8)/8.f,(float)(tileNumber/8)/8.f);uv1=ImVec2(uv0.x+1.f/8.f,uv0.y+1.f/8.f);
                pane->addButtonAndWindow(ImGui::Toolbutton(DockedWindowNames[i],myImageTextureVoid,uv0,uv1,buttonSize),         // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                         ImGui::PanelManagerPaneAssociatedWindow(DockedWindowNames[i],-1,&DrawDockedWindows));  //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
            }
            AddTabWindowIfSupported(pane);
            pane->addSeparator(48); // Note that a separator "eats" one toolbutton index as if it was a real button

            // Here we add two "automatic" toggle buttons (i.e. toolbuttons + associated windows): only the last args of Toolbutton change.
            const ImVec2 toggleButtonSize(24,24);
            tileNumber=0;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonAndWindow(ImGui::Toolbutton(ToggleWindowNames[0],myImageTextureVoid2,uv0,uv1,toggleButtonSize,true,false),        // the 1st arg of Toolbutton is only used as a text for the tooltip.
                    ImGui::PanelManagerPaneAssociatedWindow(ToggleWindowNames[0],-1,&DrawDockedWindows));              //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
            tileNumber=1;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonAndWindow(ImGui::Toolbutton(ToggleWindowNames[1],myImageTextureVoid2,uv0,uv1,toggleButtonSize,true,false),        // the 1st arg of Toolbutton is only used as a text for the tooltip.
                    ImGui::PanelManagerPaneAssociatedWindow(ToggleWindowNames[1],-1,&DrawDockedWindows));              //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
            pane->addSeparator(48); // Note that a separator "eats" one toolbutton index as if it was a real button


            // Here we add two "manual" toggle buttons (i.e. toolbuttons only):
            const ImVec2 extraButtonSize(24,24);
            tileNumber=0;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 1",myImageTextureVoid2,uv0,uv1,extraButtonSize,true,false));
            tileNumber=1;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 2",myImageTextureVoid2,uv0,uv1,extraButtonSize,true,false));

            // Optional line that affects the look of the Toolbutton in this pane: NOPE: we'll override them later for all the panes
            //pane->setDisplayProperties(ImVec2(0.25f,0.9f),ImVec4(0.85,0.85,0.85,1));

            // Optional line to manually specify alignment as ImVec2 (by default Y alignment is 0.0f for LEFT and RIGHT panes, and X alignment is 0.5f for TOP and BOTTOM panes)
            // Be warned that the component that you don't use (X in this case, must be set to 0.f for LEFT or 1.0f for RIGHT, unless you want to do strange things)
            //pane->setToolbarProperties(true,false,ImVec2(0.f,0.5f));  // place this pane at Y center (instead of Y top)
        }
    }
    // RIGHT PANE
    {
        ImGui::PanelManager::Pane* pane = mgr.addPane(ImGui::PanelManager::RIGHT,"myFirstToolbarRight##foo");
        if (pane)   {
            // Here we use (a part of) the left pane to clone windows (handy since we don't support drag and drop):
            if (mgr.getPaneLeft()) pane->addClonedPane(*mgr.getPaneLeft(),false,0,2); // note that only the "docked" part of buttons/windows are clonable ("manual" buttons are simply ignored): TO FIX: for now please avoid leaving -1 as the last argument, as this seems to mess up button indices: just explicitely copy NonTogglable-DockButtons yourself.
            // To clone single buttons (and not the whole pane) please use: pane->addClonedButtonAndWindow(...);
            // IMPORTANT: Toggle Toolbuttons (and associated windows) can't be cloned and are just skipped if present
            AddTabWindowIfSupported(pane);

            // here we could add new docked windows as well in the usual way now... but we don't
            pane->addSeparator(48);   // Note that a separator "eats" one toolbutton index as if it was a real button

            // Here we add two other "manual" toggle buttons:
            tileNumber=2;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 3",myImageTextureVoid2,uv0,uv1,ImVec2(24,32),true,false));
            tileNumber=3;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 4",myImageTextureVoid2,uv0,uv1,ImVec2(24,32),true,false));

            // Here we add two "manual" normal buttons (actually "normal" buttons are always "manual"):
            pane->addSeparator(48);   // Note that a separator "eats" one toolbutton index as if it was a real button
            tileNumber=4;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonOnly(ImGui::Toolbutton("Manual normal button 1",myImageTextureVoid2,uv0,uv1,ImVec2(24,32),false,false));
            tileNumber=5;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 2",myImageTextureVoid2,uv0,uv1,ImVec2(24,32),false,false));

        }
    }
    // BOTTOM PANE
    {
        ImGui::PanelManager::Pane* pane = mgr.addPane(ImGui::PanelManager::BOTTOM,"myFirstToolbarBottom##foo");
        if (pane)   {
            // Here we add the "proper" docked buttons and windows:
            const ImVec2 buttonSize(32,32);
            for (int i=3;i<6;i++)   {
                // Add to left pane the windows DrawDockedWindows[i] from 3 to 6, with Toolbuttons with the images from 3 to 6 of myImageTextureVoid (8x8 tiles):
                tileNumber=i;uv0=ImVec2((float)(tileNumber%8)/8.f,(float)(tileNumber/8)/8.f);uv1=ImVec2(uv0.x+1.f/8.f,uv0.y+1.f/8.f);
                pane->addButtonAndWindow(ImGui::Toolbutton(DockedWindowNames[i],myImageTextureVoid,uv0,uv1,buttonSize),         // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                         ImGui::PanelManagerPaneAssociatedWindow(DockedWindowNames[i],-1,&DrawDockedWindows));  //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
            }
            AddTabWindowIfSupported(pane);
            pane->addSeparator(64); // Note that a separator "eats" one toolbutton index as if it was a real button

            // Here we add two "automatic" toggle buttons (i.e. toolbuttons + associated windows): only the last args of Toolbutton change.
            const ImVec2 toggleButtonSize(32,32);
            tileNumber=2;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonAndWindow(ImGui::Toolbutton(ToggleWindowNames[2],myImageTextureVoid2,uv0,uv1,toggleButtonSize,true,false),        // the 1st arg of Toolbutton is only used as a text for the tooltip.
                    ImGui::PanelManagerPaneAssociatedWindow(ToggleWindowNames[2],-1,&DrawDockedWindows));              //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
            tileNumber=3;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonAndWindow(ImGui::Toolbutton(ToggleWindowNames[3],myImageTextureVoid2,uv0,uv1,toggleButtonSize,true,false),        // the 1st arg of Toolbutton is only used as a text for the tooltip.
                    ImGui::PanelManagerPaneAssociatedWindow(ToggleWindowNames[3],-1,&DrawDockedWindows));              //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
            pane->addSeparator(64); // Note that a separator "eats" one toolbutton index as if it was a real button

            // Here we add two "manual" toggle buttons:
            const ImVec2 extraButtonSize(32,32);
            tileNumber=4;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 4",myImageTextureVoid2,uv0,uv1,extraButtonSize,true,false));
            tileNumber=5;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonOnly(ImGui::Toolbutton("Manual toggle button 5",myImageTextureVoid2,uv0,uv1,extraButtonSize,true,false));

        }
    }
    // TOP PANE
    {
        // Here we create a top pane.
        ImGui::PanelManager::Pane* pane = mgr.addPane(ImGui::PanelManager::TOP,"myFirstToolbarTop##foo");
        if (pane)   {
            // Here we add the "proper" docked buttons and windows:
            const ImVec2 buttonSize(32,32);
            for (int i=6;i<7;i++)   {
                // Add to left pane the windows DrawDockedWindows[i] from 3 to 6, with Toolbuttons with the images from 3 to 6 of myImageTextureVoid (8x8 tiles):
                tileNumber=i;uv0=ImVec2((float)(tileNumber%8)/8.f,(float)(tileNumber/8)/8.f);uv1=ImVec2(uv0.x+1.f/8.f,uv0.y+1.f/8.f);
                pane->addButtonAndWindow(ImGui::Toolbutton(DockedWindowNames[i],myImageTextureVoid,uv0,uv1,buttonSize),         // the 1st arg of Toolbutton is only used as a text for the tooltip.
                                         ImGui::PanelManagerPaneAssociatedWindow(DockedWindowNames[i],-1,&DrawDockedWindows));  //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
            }
            AddTabWindowIfSupported(pane);
            pane->addSeparator(64); // Note that a separator "eats" one toolbutton index as if it was a real button

            const ImVec2 extraButtonSize(32,32);
            pane->addButtonOnly(ImGui::Toolbutton("Normal Manual Button 1",myImageTextureVoid2,ImVec2(0,0),ImVec2(1.f/3.f,1.f/3.f),extraButtonSize));//,false,false,ImVec4(0,1,0,1)));  // Here we add a free button
            tileNumber=1;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonOnly(ImGui::Toolbutton("Normal Manual Button 2",myImageTextureVoid2,uv0,uv1,extraButtonSize));  // Here we add a free button
            tileNumber=2;uv0=ImVec2((float)(tileNumber%3)/3.f,(float)(tileNumber/3)/3.f);uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);
            pane->addButtonOnly(ImGui::Toolbutton("Normal Manual Button 3",myImageTextureVoid2,uv0,uv1,extraButtonSize));  // Here we add a free button
            pane->addSeparator(32);  // Note that a separator "eats" one toolbutton index as if it was a real button

            // Here we add two manual toggle buttons, but we'll use them later to show/hide menu and show/hide a central window
            const ImVec2 toggleButtonSize(32,32);
            tileNumber=7;uv0=ImVec2((float)(tileNumber%8)/8.f,(float)(tileNumber/8)/8.f);uv1=ImVec2(uv0.x+1.f/8.f,uv0.y+1.f/8.f);
            pane->addButtonOnly(ImGui::Toolbutton("Show/Hide Main Menu Bar",myImageTextureVoid,uv0,uv1,toggleButtonSize,true,true));  // [*] Here we add a manual toggle button we'll simply bind to "gpShowMainMenuBar" later. Start value is last arg.
            tileNumber=5;uv0=ImVec2((float)(tileNumber%8)/8.f,(float)(tileNumber/8)/8.f);uv1=ImVec2(uv0.x+1.f/8.f,uv0.y+1.f/8.f);
            pane->addButtonOnly(ImGui::Toolbutton("Show/Hide central window",myImageTextureVoid,uv0,uv1,toggleButtonSize,true,true));  // [**] Here we add a manual toggle button we'll process later [**]

            // Ok. Now all the buttons/windows have been added to the TOP Pane.
            // We can safely bind our bool pointers without any risk (ImVector reallocations could have invalidated them).
            // Please note that it's not safe to add EVERYTHING (even separators) to this (TOP) pane afterwards (unless we bind the booleans again).
            gpShowMainMenuBar = &pane->bar.getButton(pane->getSize()-2)->isDown;            // [*]
            gpShowCentralWindow = &pane->bar.getButton(pane->getSize()-1)->isDown;          // [**]

        }
    }

    // Optional. Loads the layout (just selectedButtons, docked windows sizes and stuff like that)
#   if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_LOAD))
    LoadPanelManagerIfSupported();
#   endif //!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !...


    // Optional line that affects the look of all the Toolbuttons in the Panes inserted so far:
    mgr.overrideAllExistingPanesDisplayProperties(ImVec2(0.25f,0.9f),ImVec4(0.85,0.85,0.85,1));

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
    for (int i=0;i<5;i++) tabWindows[i].clear();  // for robustness (they're already empty)
    if (!LoadTabWindowsIfSupported()) {
        // Here we set the starting configurations of the tabs in our ImGui::TabWindows
        static const char* tabNames[] = {"TabLabelStyle","Render","Layers","Capture","Scene","World","Object","Constraints","Modifiers","Data","Material","Texture","Particle","Physics"};
        static const int numTabs = sizeof(tabNames)/sizeof(tabNames[0]);
        static const char* tabTooltips[numTabs] = {"Edit the look of the tab labels","Render Tab Tooltip","Layers Tab Tooltip","Capture Tab Tooltip","non-draggable","Another Tab Tooltip","","","","non-draggable","Tired to add tooltips...",""};
        for (int i=0;i<numTabs;i++) {
            tabWindow.addTabLabel(tabNames[i],tabTooltips[i],i%3!=0,i%5!=4);
        }
#       ifdef YES_IMGUIMINIGAMES
#           ifndef NO_IMGUIMINIGAMES_MINE
        tabWindow.addTabLabel("ImGuiMineGame","a mini-game",false,true,NULL,NULL,0,ImGuiWindowFlags_NoScrollbar);
#           endif // NO_IMGUIMINIGAMES_MINE
#           ifndef NO_IMGUIMINIGAMES_SUDOKU
        tabWindow.addTabLabel("ImGuiSudokuGame","a mini-game",false,true,NULL,NULL,0,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
#           endif // NO_IMGUIMINIGAMES_SUDOKU
#           ifndef NO_IMGUIMINIGAMES_FIFTEEN
        tabWindow.addTabLabel("ImGuiFifteenGame","a mini-game",false,true,NULL,NULL,0,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
#           endif // NO_IMGUIMINIGAMES_FIFTEEN
#       endif // YES_IMGUIMINIGAMES
#       ifdef YES_IMGUIIMAGEEDITOR
        tabWindow.addTabLabel("ImGuiImageEditor","a tiny image editor",false,true,NULL,NULL,0,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
#       endif //YES_IMGUIIMAGEEDITOR
#       ifndef NO_IMGUISTYLESERIALIZER
        tabWindow.addTabLabel("ImGuiStyleChooser","Edit the look of the GUI",false);
#       endif //NO_IMGUISTYLESERIALIZER
#       ifndef NO_IMGUIFILESYSTEM  // Optional stuff to enhance file system dialogs with icons
#       ifndef NO_IMGUICODEEDITOR
        ImGui::TabWindow::TabLabel* tabLabel = tabWindow.addTabLabel("Code Editor","",false,true,NULL,"",500,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        tabLabel->userInt = 500;    // We'll use tabLabel->userInt>=500 to detect if it's a code editor
#       endif //NO_IMGUICODEEDITOR
#       ifdef YES_IMGUIPDFVIEWER
        tabWindow.addTabLabel("dummyPdfPath.pdf","",false,true,NULL,"",0,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
#       endif //YES_IMGUIPDFVIEWER
#       endif //NO_IMGUIFILESYSTEM
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
static void ShowExampleMenuBar(bool isMainMenu=false)
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
            static bool booleanProps[3]={true,false,true};
            static const char* names[3]={"Boolean Test 1","Boolean Test 2","Boolean Test 3"};
            const bool isRightMouseButtonClicked = ImGui::IsMouseClicked(1);    // cached
            for (int i=0;i<3;i++)   {
                ImGui::MenuItem(names[i], NULL, &booleanProps[i]);
                if (isRightMouseButtonClicked && ImGui::IsItemHovered()) booleanProps[i]=!booleanProps[i];
            }

            ImGui::EndMenu();
        }
        if (isMainMenu) {
            //gMainMenuBarSize = ImGui::GetWindowSize();
            ImGui::EndMainMenuBar();
        }
        else ImGui::EndMenuBar();
    }
    //ImGui::PopID();
}



void ResizeGL(int w,int h)	// Mandatory
{
    //fprintf(stderr,"ResizeGL(%d,%d); ImGui::DisplaySize(%d,%d);\n",w,h,(int)ImGui::GetIO().DisplaySize.x,(int)ImGui::GetIO().DisplaySize.y);
    static ImVec2 initialSize(w,h);
    mgr.setToolbarsScaling((float)w/initialSize.x,(float)h/initialSize.y);  // Scales the PanelManager bounmds based on the initialSize
    SetPanelManagerBoundsToIncludeMainMenuIfPresent(w, h);                  // This line is only necessary if we have a global menu bar
}


void DrawDockedWindows(ImGui::PanelManagerWindowData& wd)    {
    if (!wd.isToggleWindow)    {
        // Here we simply draw all the docked windows (in our case DockedWindowNames) without using ImGui::Begin()/ImGui::End().
        // (This is necessary because docked windows are not normal windows: see the title bar for example)
        if (strcmp(wd.name,DockedWindowNames[0])==0)  {
            // Draw Solution Explorer
            ImGui::Text("%s\n",wd.name);
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
        }
        else if (strcmp(wd.name,DockedWindowNames[1])==0)    {
            // Draw Toolbox
            ImGui::Text("%s\n",wd.name);
        }
        else if (strcmp(wd.name,DockedWindowNames[2])==0)    {
            ImGui::Text("%s\n",wd.name);
#           ifdef TEST_ICONS_INSIDE_TTF
            ImGui::Spacing();ImGui::Separator();
            ImGui::TextDisabled("Testing icons inside FontAwesome");
            ImGui::Separator();ImGui::Spacing();

            ImGui::Button( ICON_FA_FILE "  File" ); // use string literal concatenation, ouputs a file icon and File
#           ifndef NO_IMGUIFILESYSTEM // Testing icons inside ImGuiFs::Dialog
            ImGui::AlignFirstTextHeightToWidgets();ImGui::Text("File:");ImGui::SameLine();
            static ImGuiFs::Dialog dlg;
            ImGui::InputText("###fsdlg",(char*)dlg.getChosenPath(),ImGuiFs::MAX_PATH_BYTES,ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            const bool browseButtonPressed = ImGui::Button("...##fsdlg");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s","file chooser dialog\nwith FontAwesome icons");
            dlg.chooseFileDialog(browseButtonPressed,dlg.getLastDirectory());
#           endif //NO_IMGUIFILESYSTEM
            if (ImGui::TreeNode("All FontAwesome Icons")){
                DrawAllFontAwesomeIcons();
                ImGui::TreePop();
            }

            ImGui::Spacing();ImGui::Separator();
            ImGui::TextDisabled("Testing styled check boxes by dougbinks");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s","https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9\nThey work better with a Monospace font");
            ImGui::Separator();ImGui::Spacing();

            static bool cb1=false;ImGui::CheckBoxFont("CheckBoxFont",&cb1);
            static bool cb2=false;ImGui::CheckBoxTick("CheckBoxTick",&cb2);
            static bool cb3=false;ImGui::MenuItemCheckBox("MenuItemCheckBox",&cb3);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s","Probably this works only\ninside a Menu...");


            // We, we can use default fonts as well (std UTF8 chars):
            ImGui::Spacing();ImGui::Separator();
            ImGui::TextDisabled("Testing the same without any icon font");
            ImGui::Separator();ImGui::Spacing();

            static bool cb4=false;ImGui::CheckBoxFont("CheckBoxDefaultFonts",&cb4,"▣","□");
            static bool cb5=false;ImGui::CheckBoxFont("CheckBoxDefaultFonts2",&cb5,"■","□");
            static bool cb6=false;ImGui::CheckBoxFont("CheckBoxDefaultFonts3",&cb6,"▼","▶");
            static bool cb7=false;ImGui::CheckBoxFont("CheckBoxDefaultFonts4",&cb7,"▽","▷");
#           endif //TEST_ICONS_INSIDE_TTF
        }
        else if (strcmp(wd.name,DockedWindowNames[3])==0) {
            // Draw Find Window
            ImGui::Text("%s\n",wd.name);
        }
        else if (strcmp(wd.name,DockedWindowNames[4])==0)    {
            // Draw Output Window
            ImGui::Text("%s\n",wd.name);
        }
        else if (strcmp(wd.name,"Preferences")==0)    {
            ImGui::DragFloat("Window Alpha##WA1", &mgr.getDockedWindowsAlpha(), 0.005f, -0.01f, 1.0f, mgr.getDockedWindowsAlpha() < 0.0f ? "(default)" : "%.3f");
            bool noTitleBar = mgr.getDockedWindowsNoTitleBar();
            if (ImGui::Checkbox("No Window TitleBars",&noTitleBar)) mgr.setDockedWindowsNoTitleBar(noTitleBar);
            if (gpShowCentralWindow) {ImGui::SameLine();ImGui::Checkbox("Show Central Wndow",(bool*)gpShowCentralWindow);}
            if (gpShowMainMenuBar)  {
                ImGui::SameLine();
                if (ImGui::Checkbox("Show Main Menu",(bool*)gpShowMainMenuBar)) SetPanelManagerBoundsToIncludeMainMenuIfPresent();
            }
            // Here we test the Nav feature (not serialized)
            ImGui::Spacing();
            unsigned int* pNavFlags = (unsigned int*) &ImGui::GetIO().ConfigFlags;
            ImGui::AlignTextToFramePadding();ImGui::TextUnformatted("ConfigFlags:");
            ImGui::SameLine();ImGui::CheckboxFlags("NavEnableKeyboard",pNavFlags,ImGuiConfigFlags_NavEnableKeyboard);
            if (ImGui::GetIO().BackendFlags&ImGuiBackendFlags_HasGamepad)   {
                ImGui::SameLine();ImGui::CheckboxFlags("NavEnableGamepad",pNavFlags,ImGuiConfigFlags_NavEnableGamepad);
            }
            ImGui::SameLine();ImGui::CheckboxFlags("NavMoveMouse",pNavFlags,ImGuiConfigFlags_NavEnableSetMousePos);
            //ImGui::SameLine();ImGui::CheckboxFlags("NavNoCaptureKeyboard",pNavFlags,ImGuiConfigFlags_NavNoCaptureKeyboard);
            if (ImGui::GetIO().ConfigFlags&ImGuiConfigFlags_NavEnableKeyboard) {ImGui::SameLine(0,30);ImGui::TextDisabled("%s","Keys: CTRL+TAB and CTRL+SHIFT+TAB, Space and Esc, Arrows");}


	    // Here we test saving/loading the ImGui::PanelManager layout (= the sizes of the 4 docked windows and the buttons that are selected on the 4 toolbars)
	    // Please note that the API should allow loading/saving different items into a single file and loading/saving from/to memory too, but we don't show it now.
#           if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION))
	    ImGui::Separator();
	    static const char pmTooltip[] = "the ImGui::PanelManager layout\n(the sizes of the 4 docked windows and\nthe buttons that are selected\non the 4 toolbars)";
#           ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
	    if (ImGui::Button("Save Panel Manager Layout")) SavePanelManagerIfSupported();
	    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save %s",pmTooltip);
#           endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#           ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
	    ImGui::SameLine();if (ImGui::Button("Load Panel Manager Layout")) {
            if (LoadPanelManagerIfSupported())   SetPanelManagerBoundsToIncludeMainMenuIfPresent();	// That's because we must adjust gpShowMainMenuBar state here (we have used a manual toggle button for it)
	    }
	    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load %s",pmTooltip);
#           endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#           endif //NO_IMGUIHELPER_SERIALIZATION

#           ifndef NO_IMGUITABWINDOW
#		if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION))
		ImGui::Separator();
		static const char twTooltip[] = "the layout of the 5 ImGui::TabWindows\n(this option is also available by right-clicking\non an empty space in the Tab Header)";
#		ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
		if (ImGui::Button("Save TabWindows Layout")) SaveTabWindowsIfSupported();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save %s",twTooltip);
#		endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#		ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
		ImGui::SameLine();if (ImGui::Button("Load TabWindows Layout")) LoadTabWindowsIfSupported();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load %s",twTooltip);
#		endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#		endif //NO_IMGUIHELPER_SERIALIZATION

		//ImGui::Spacing();
		//if (ImGui::Button("Reset Central Window Tabs")) ResetTabWindow(tabWindow);
#           endif //NO_IMGUITABWINDOW
        }
        else /*if (strcmp(wd.name,DockedWindowNames[5])==0)*/    {
            // Draw Application Window
            ImGui::Text("%s\n",wd.name);
        }
    }
    else {
        // Here we draw our toggle windows (in our case ToggleWindowNames) in the usual way:
        // We can use -1.f for alpha here, instead of mgr.getDockedWindowsAlpha(), that can be too low (but choose what you like)
        //if (ImGui::Begin(wd.name,&wd.open,wd.size,-1.f,ImGuiWindowFlags_NoSavedSettings)) // Old API
        ImGui::SetNextWindowSize(wd.size, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(wd.name,&wd.open,ImGuiWindowFlags_NoSavedSettings))
        {
            if (strcmp(wd.name,ToggleWindowNames[0])==0)   {
                // Draw Toggle Window 1
                ImGui::SetWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x*0.15f,ImGui::GetIO().DisplaySize.y*0.24f),ImGuiCond_FirstUseEver);
                ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x*0.25f,ImGui::GetIO().DisplaySize.y*0.24f),ImGuiCond_FirstUseEver);

                ImGui::Text("Hello world from toggle window \"%s\"",wd.name);                
            }
            else
            {
                // Draw Toggle Window
                ImGui::SetWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x*0.25f,ImGui::GetIO().DisplaySize.y*0.34f),ImGuiCond_FirstUseEver);
                ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x*0.5f,ImGui::GetIO().DisplaySize.y*0.34f),ImGuiCond_FirstUseEver);
                ImGui::Text("Hello world from toggle window \"%s\"",wd.name);

                //ImGui::Checkbox("wd.open",&wd.open);  // This can be used to close the window too
            }
        }
        ImGui::End();
    }
}


void DrawGL()	// Mandatory
{
        ImImpl_ClearColorBuffer(gClearColor);    // Warning: it does not clear depth buffer

        if (gpShowMainMenuBar && *gpShowMainMenuBar) ShowExampleMenuBar(true);

        // Actually the following "if block" that displays the Central Window was placed at the very bottom (after mgr.render),
        // but I've discovered that if I set some "automatic toggle window" to be visible at startup, it would be covered by the central window otherwise.
        // Update: no matter, we can use the ImGuiWindowFlags_NoBringToFrontOnFocus flag for that if we want.
        if (gpShowCentralWindow && *gpShowCentralWindow)   {
            const ImVec2& iqs = mgr.getCentralQuadSize();
            if (iqs.x>ImGui::GetStyle().WindowMinSize.x && iqs.y>ImGui::GetStyle().WindowMinSize.y) {
                ImGui::SetNextWindowPos(mgr.getCentralQuadPosition());
                ImGui::SetNextWindowSize(mgr.getCentralQuadSize());
                ImGui::SetNextWindowBgAlpha(mgr.getDockedWindowsAlpha());
                if (ImGui::Begin("Central Window",NULL,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove  | ImGuiWindowFlags_NoResize | mgr.getDockedWindowsExtraFlags() /*| ImGuiWindowFlags_NoBringToFrontOnFocus*/))    {
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
        ImGui::PanelManagerPane* pressedPane=NULL;  // Optional
        int pressedPaneButtonIndex = -1;            // Optional
        if (mgr.render(&pressedPane,&pressedPaneButtonIndex))   {
            //const ImVec2& iqp = mgr.getCentralQuadPosition();
            //const ImVec2& iqs = mgr.getCentralQuadSize();
            //fprintf(stderr,"Inner Quad Size changed to {%1.f,%1.f,%1.f,%1.f}\n",iqp.x,iqp.y,iqs.x,iqs.y);
        }

        // (Optional) Some manual feedback to the user (actually I detect gpShowMainMenuBar pressures here too, but pleese read below...):
        if (pressedPane && pressedPaneButtonIndex!=-1)
        {
            static const char* paneNames[]={"LEFT","RIGHT","TOP","BOTTOM"};
            if (!pressedPane->getWindowName(pressedPaneButtonIndex)) {
                ImGui::Toolbutton* pButton = NULL;
                pressedPane->getButtonAndWindow(pressedPaneButtonIndex,&pButton);
                if (pButton->isToggleButton) {
                    printf("Pressed manual toggle button (number: %d on pane: %s)\n",pressedPaneButtonIndex,paneNames[pressedPane->pos]);
                    if (pressedPane->pos==ImGui::PanelManager::TOP && pressedPaneButtonIndex==(int)pressedPane->getSize()-2) {
                        // For this we could have just checked if *gpShowMainMenuBar had changed its value before and after mgr.render()...
                        SetPanelManagerBoundsToIncludeMainMenuIfPresent();
                    }
                }
                else printf("Pressed manual button (number: %d on pane: %s)\n",pressedPaneButtonIndex,paneNames[pressedPane->pos]);
                fflush(stdout);
            }
            else {
                ImGui::Toolbutton* pButton = NULL;
                pressedPane->getButtonAndWindow(pressedPaneButtonIndex,&pButton);
                if (pButton->isToggleButton) printf("Pressed toggle button (number: %d on pane: %s)\n",pressedPaneButtonIndex,paneNames[pressedPane->pos]);
                else printf("Pressed dock button (number: %d on pane: %s)\n",pressedPaneButtonIndex,paneNames[pressedPane->pos]);
                fflush(stdout);
            }

        }


}
void DestroyGL()    // Mandatory
{
    // Here we could save the TabWindows and the PanelManager... but we don't!

    if (myImageTextureId) {ImImpl_FreeTexture(myImageTextureId);}
    if (myImageTextureId2) {ImImpl_FreeTexture(myImageTextureId2);}
}





// Application code
#ifndef IMGUI_USE_AUTO_BINDING_WINDOWS  // IMGUI_USE_AUTO_ definitions get defined automatically (e.g. do NOT touch them!)
int main(int argc, char** argv)
#else //IMGUI_USE_AUTO_BINDING_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int iCmdShow)   // This branch has made my code less concise (I will consider stripping it)
#endif //IMGUI_USE_AUTO_BINDING_WINDOWS
{

        static const ImWchar ranges[] =
        {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x20AC, 0x20AC,	// €
            0x2122, 0x2122,	// ™
            0x2196, 0x2196, // ↖
            0x21D6, 0x21D6, // ⇖
            0x2B01, 0x2B01, // ⬁
            0x2B09, 0x2B09, // ⬉
            0x2921, 0x2922, // ⤡ ⤢
            0x263A, 0x263A, // ☺
            0x266A, 0x266A, // ♪
            0x25B2, 0x25B5, // ▲ △ ▴ ▵
            0x25BC, 0x25BF, // ▼ ▽ ▾ ▿
            0x25B6, 0x25BB, //: ▶ ▷ ▸ ▹ ► ▻
            0x25C0, 0x25C5, // ◀ ◁ ◂ ◃ ◄ ◅
            0x25A0, 0x25A3, // ■ □ ▢ ▣
            0 // € ™ ↖ ⇖ ⬁ ⬉ ⤡ ⤢ ☺ ♪
        };
    const float fontSizeInPixels = 18.f;


    // These lines load an embedded font. [However these files are way too big... inside <imgui.cpp> they used a better format storing bytes at groups of 4, so the files are more concise (1/4?) than mine]
    const unsigned char ttfMemory[] =
//#   include "./fonts/DejaVuSerifCondensed-Bold.ttf.stbz.inl"  // stbz.inl files needs a special definition to be defined in "ImImpl_RenderDrawLists.h"
//#   include "./fonts/DroidSerif-Bold.ttf.stbz.inl"
#   include "./fonts/DejaVuSerifCondensed-Bold.ttf.inl"
//#   include "./fonts/DroidSerif-Bold.ttf.inl"

;

    ImImpl_InitParams gImGuiInitParams(
    -1,-1,NULL,                                                         // optional window width, height, title

    NULL,
    //"./fonts/DejaVuSerifCondensed-Bold.ttf",                          // optional custom font from file (main custom font)
    //"./fonts/DroidSerif-Bold.ttf",                                       // optional custom font from file (main custom font)                                                              // optional white spot in font texture (returned by the console if not set)

    //NULL,0,
    &ttfMemory[0],sizeof(ttfMemory)/sizeof(ttfMemory[0]),               // optional custom font from memory (secondary custom font) WARNING (licensing problem): e.g. embedding a GPL font in your code can make your code GPL as well.

    fontSizeInPixels,
    &ranges[0]
    );
    //gImGuiInitParams.gFpsClampInsideImGui = 30.0f;                                 // Optional Max allowed FPS (default -1 => unclamped). Useful for editors and to save GPU and CPU power.
    //gImGuiInitParams.gFpsClampOutsideImGui = 15.0f;                                 // Optional Max allowed FPS (default -1 => unclamped). Useful for editors and to save GPU and CPU power.

#   ifdef TEST_ICONS_INSIDE_TTF
    {
    static const ImWchar iconFontRanges[] ={ICON_MIN_FA,ICON_MAX_FA,0};
    ImFontConfig fntCfg;fntCfg.MergeMode = true;fntCfg.PixelSnapH = true;
    fntCfg.OversampleV=fntCfg.OversampleH=1;    // To save texture memory (but commenting it out makes icons look better)
    gImGuiInitParams.fonts.push_back(ImImpl_InitParams::FontData("fonts/Icons/FontAwesome4/font.ttf",fontSizeInPixels,&iconFontRanges[0],&fntCfg));
    }
#   endif //TEST_ICONS_INSIDE_TTF

#   if (!defined(NO_IMGUICODEEDITOR) && !defined(NO_IMGUIFILESYSTEM))
    // We use a monopace font for our code editor
    {
    ImFontConfig fntCfg;//fntCfg.MergeMode = true;fntCfg.PixelSnapH = true;
    fntCfg.OversampleV=fntCfg.OversampleH=1;    // To save texture memory (but commenting it out makes icons look better)
    const unsigned char ttfMonospaceMemory[] =
#       include "./fonts/Mono/DejaVuSansMono-Bold-Stripped.ttf.inl"
    ;
    gImGuiInitParams.fonts.push_back(ImImpl_InitParams::FontData(ttfMonospaceMemory,sizeof(ttfMonospaceMemory)/sizeof(ttfMonospaceMemory[0]),ImImpl_InitParams::FontData::COMP_NONE,fontSizeInPixels,NULL,&fntCfg));
    // Note that this font will be assigned to the Code Editor in InitGL() (no need to PushFont()/PopFont() before ImGui::InputTextWithSyntaxHighlighting(...);
    }
#   endif //(!defined(NO_IMGUICODEEDITOR) && !defined(NO_IMGUIFILESYSTEM))


#   ifndef IMGUI_USE_AUTO_BINDING_WINDOWS  // IMGUI_USE_AUTO_ definitions get defined automatically (e.g. do NOT touch them!)
    ImImpl_Main(&gImGuiInitParams,argc,argv);
#   else //IMGUI_USE_AUTO_BINDING_WINDOWS
    ImImpl_WinMain(&gImGuiInitParams,hInstance,hPrevInstance,lpCmdLine,iCmdShow);
#   endif //IMGUI_USE_AUTO_BINDING_WINDOWS

	return 0;
}





#ifdef TEST_ICONS_INSIDE_TTF
#ifndef NO_IMGUIFILESYSTEM
bool MyFSDrawFileIconCb(int extensionType,const ImVec4* pOptionalColorOverride=NULL) {
    using namespace ImGuiFs;
    if (extensionType<0 || extensionType>=FET_COUNT) extensionType=0;   // FET_NONE, or just return false to skip icon drawing
    static const char* icons[FET_COUNT] = {
        ICON_FA_FILE_O,                 // FET_NONE
        ICON_FA_H_SQUARE,               // FET_HPP
        ICON_FA_PLUS_SQUARE,            // FET_CPP
        ICON_FA_FILE_IMAGE_O,           // FET_IMAGE
        ICON_FA_FILE_PDF_O,             // FET_PDF
        ICON_FA_FILE_WORD_O,            // FET_DOCUMENT
        ICON_FA_FILE_TEXT_O,            // FET_TEXT
        ICON_FA_DATABASE,               // FET_DATABASE
        ICON_FA_FILE_EXCEL_O,           // FET_SPREADSHEET
        ICON_FA_FILE_POWERPOINT_O,      // FET_PRESENTATION
        ICON_FA_FILE_ARCHIVE_O,         // FET_ARCHIVE
        ICON_FA_FILE_AUDIO_O,           // FET_AUDIO
        ICON_FA_FILE_VIDEO_O,           // FET_VIDEO
        ICON_FA_FILE_CODE_O,            // FET_XML
        ICON_FA_FILE_CODE_O             // FET_HTML
    };
    if (!pOptionalColorOverride) ImGui::Text("%s",icons[extensionType]);
    else ImGui::TextColored(*pOptionalColorOverride,"%s",icons[extensionType]);
    return true;
}
bool MyFSDrawFolderIconCb(bool useOpenFolderIconIfAvailable,const ImVec4* pOptionalColorOverride) {
    if (!pOptionalColorOverride) {
        if (useOpenFolderIconIfAvailable) ImGui::Text(ICON_FA_FOLDER_OPEN);
        else  ImGui::Text(ICON_FA_FOLDER);
    }
    else {
        if (useOpenFolderIconIfAvailable) ImGui::TextColored(*pOptionalColorOverride,ICON_FA_FOLDER_OPEN);
        else  ImGui::TextColored(*pOptionalColorOverride,ICON_FA_FOLDER);
    }
    return true;
}
#endif //NO_IMGUIFILESYSTEM
void DrawAllFontAwesomeIcons() {
                ImGui::Text( ICON_FA_GLASS );//80\x80"
                ImGui::SameLine();ImGui::Text( ICON_FA_MUSIC );//80\x81"
                ImGui::SameLine();ImGui::Text( ICON_FA_SEARCH );//80\x82"
                ImGui::SameLine();ImGui::Text( ICON_FA_ENVELOPE_O );//80\x83"
                ImGui::SameLine();ImGui::Text( ICON_FA_HEART );//80\x84"
                ImGui::SameLine();ImGui::Text( ICON_FA_STAR );//80\x85"
                ImGui::SameLine();ImGui::Text( ICON_FA_STAR_O );//80\x86"
                ImGui::SameLine();ImGui::Text( ICON_FA_USER );//80\x87"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILM );//80\x88"
                ImGui::SameLine();ImGui::Text( ICON_FA_TH_LARGE );//80\x89"
                ImGui::Text( ICON_FA_TH );//80\x8A"
                ImGui::SameLine();ImGui::Text( ICON_FA_TH_LIST );//80\x8B"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHECK );//80\x8C"
                ImGui::SameLine();ImGui::Text( ICON_FA_TIMES );//80\x8D"
                ImGui::SameLine();ImGui::Text( ICON_FA_SEARCH_PLUS );//80\x8E"
                ImGui::SameLine();ImGui::Text( ICON_FA_SEARCH_MINUS );//80\x90"
                ImGui::SameLine();ImGui::Text( ICON_FA_POWER_OFF );//80\x91"
                ImGui::SameLine();ImGui::Text( ICON_FA_SIGNAL );//80\x92"
                ImGui::SameLine();ImGui::Text( ICON_FA_COG );//80\x93"
                ImGui::SameLine();ImGui::Text( ICON_FA_TRASH_O );//80\x94"
                ImGui::Text( ICON_FA_HOME );//80\x95"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE_O );//80\x96"
                ImGui::SameLine();ImGui::Text( ICON_FA_CLOCK_O );//80\x97"
                ImGui::SameLine();ImGui::Text( ICON_FA_ROAD );//80\x98"
                ImGui::SameLine();ImGui::Text( ICON_FA_DOWNLOAD );//80\x99"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROW_CIRCLE_O_DOWN );//80\x9A"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROW_CIRCLE_O_UP );//80\x9B"
                ImGui::SameLine();ImGui::Text( ICON_FA_INBOX );//80\x9C"
                ImGui::SameLine();ImGui::Text( ICON_FA_PLAY_CIRCLE_O );//80\x9D"
                ImGui::SameLine();ImGui::Text( ICON_FA_REPEAT );//80\x9E"
                ImGui::Text( ICON_FA_REFRESH );//80\xA1"
                ImGui::SameLine();ImGui::Text( ICON_FA_LIST_ALT );//80\xA2"
                ImGui::SameLine();ImGui::Text( ICON_FA_LOCK );//80\xA3"
                ImGui::SameLine();ImGui::Text( ICON_FA_FLAG );//80\xA4"
                ImGui::SameLine();ImGui::Text( ICON_FA_HEADPHONES );//80\xA5"
                ImGui::SameLine();ImGui::Text( ICON_FA_VOLUME_OFF );//80\xA6"
                ImGui::SameLine();ImGui::Text( ICON_FA_VOLUME_DOWN );//80\xA7"
                ImGui::SameLine();ImGui::Text( ICON_FA_VOLUME_UP );//80\xA8"
                ImGui::SameLine();ImGui::Text( ICON_FA_QRCODE );//80\xA9"
                ImGui::SameLine();ImGui::Text( ICON_FA_BARCODE );//80\xAA"
                ImGui::Text( ICON_FA_TAG );//80\xAB"
                ImGui::SameLine();ImGui::Text( ICON_FA_TAGS );//80\xAC"
                ImGui::SameLine();ImGui::Text( ICON_FA_BOOK );//80\xAD"
                ImGui::SameLine();ImGui::Text( ICON_FA_BOOKMARK );//80\xAE"
                ImGui::SameLine();ImGui::Text( ICON_FA_PRINT );//80\xAF"
                ImGui::SameLine();ImGui::Text( ICON_FA_CAMERA );//80\xB0"
                ImGui::SameLine();ImGui::Text( ICON_FA_FONT );//80\xB1"
                ImGui::SameLine();ImGui::Text( ICON_FA_BOLD );//80\xB2"
                ImGui::SameLine();ImGui::Text( ICON_FA_ITALIC );//80\xB3"
                ImGui::SameLine();ImGui::Text( ICON_FA_TEXT_HEIGHT );//80\xB4"
                ImGui::Text( ICON_FA_TEXT_WIDTH );//80\xB5"
                ImGui::SameLine();ImGui::Text( ICON_FA_ALIGN_LEFT );//80\xB6"
                ImGui::SameLine();ImGui::Text( ICON_FA_ALIGN_CENTER );//80\xB7"
                ImGui::SameLine();ImGui::Text( ICON_FA_ALIGN_RIGHT );//80\xB8"
                ImGui::SameLine();ImGui::Text( ICON_FA_ALIGN_JUSTIFY );//80\xB9"
                ImGui::SameLine();ImGui::Text( ICON_FA_LIST );//80\xBA"
                ImGui::SameLine();ImGui::Text( ICON_FA_OUTDENT );//80\xBB"
                ImGui::SameLine();ImGui::Text( ICON_FA_INDENT );//80\xBC"
                ImGui::SameLine();ImGui::Text( ICON_FA_VIDEO_CAMERA );//80\xBD"
                ImGui::SameLine();ImGui::Text( ICON_FA_PICTURE_O );//80\xBE"
                ImGui::Text( ICON_FA_PENCIL );//81\x80"
                ImGui::SameLine();ImGui::Text( ICON_FA_MAP_MARKER );//81\x81"
                ImGui::SameLine();ImGui::Text( ICON_FA_ADJUST );//81\x82"
                ImGui::SameLine();ImGui::Text( ICON_FA_TINT );//81\x83"
                ImGui::SameLine();ImGui::Text( ICON_FA_PENCIL_SQUARE_O );//81\x84"
                ImGui::SameLine();ImGui::Text( ICON_FA_SHARE_SQUARE_O );//81\x85"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHECK_SQUARE_O );//81\x86"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROWS );//81\x87"
                ImGui::SameLine();ImGui::Text( ICON_FA_STEP_BACKWARD );//81\x88"
                ImGui::SameLine();ImGui::Text( ICON_FA_FAST_BACKWARD );//81\x89"
                ImGui::Text( ICON_FA_BACKWARD );//81\x8A"
                ImGui::SameLine();ImGui::Text( ICON_FA_PLAY );//81\x8B"
                ImGui::SameLine();ImGui::Text( ICON_FA_PAUSE );//81\x8C"
                ImGui::SameLine();ImGui::Text( ICON_FA_STOP );//81\x8D"
                ImGui::SameLine();ImGui::Text( ICON_FA_FORWARD );//81\x8E"
                ImGui::SameLine();ImGui::Text( ICON_FA_FAST_FORWARD );//81\x90"
                ImGui::SameLine();ImGui::Text( ICON_FA_STEP_FORWARD );//81\x91"
                ImGui::SameLine();ImGui::Text( ICON_FA_EJECT );//81\x92"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHEVRON_LEFT );//81\x93"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHEVRON_RIGHT );//81\x94"
                ImGui::Text( ICON_FA_PLUS_CIRCLE );//81\x95"
                ImGui::SameLine();ImGui::Text( ICON_FA_MINUS_CIRCLE );//81\x96"
                ImGui::SameLine();ImGui::Text( ICON_FA_TIMES_CIRCLE );//81\x97"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHECK_CIRCLE );//81\x98"
                ImGui::SameLine();ImGui::Text( ICON_FA_QUESTION_CIRCLE );//81\x99"
                ImGui::SameLine();ImGui::Text( ICON_FA_INFO_CIRCLE );//81\x9A"
                ImGui::SameLine();ImGui::Text( ICON_FA_CROSSHAIRS );//81\x9B"
                ImGui::SameLine();ImGui::Text( ICON_FA_TIMES_CIRCLE_O );//81\x9C"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHECK_CIRCLE_O );//81\x9D"
                ImGui::SameLine();ImGui::Text( ICON_FA_BAN );//81\x9E"
                ImGui::Text( ICON_FA_ARROW_LEFT );//81\xA0"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROW_RIGHT );//81\xA1"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROW_UP );//81\xA2"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROW_DOWN );//81\xA3"
                ImGui::SameLine();ImGui::Text( ICON_FA_SHARE );//81\xA4"
                ImGui::SameLine();ImGui::Text( ICON_FA_EXPAND );//81\xA5"
                ImGui::SameLine();ImGui::Text( ICON_FA_COMPRESS );//81\xA6"
                ImGui::SameLine();ImGui::Text( ICON_FA_PLUS );//81\xA7"
                ImGui::SameLine();ImGui::Text( ICON_FA_MINUS );//81\xA8"
                ImGui::SameLine();ImGui::Text( ICON_FA_ASTERISK );//81\xA9"
                ImGui::Text( ICON_FA_EXCLAMATION_CIRCLE );//81\xAA"
                ImGui::SameLine();ImGui::Text( ICON_FA_GIFT );//81\xAB"
                ImGui::SameLine();ImGui::Text( ICON_FA_LEAF );//81\xAC"
                ImGui::SameLine();ImGui::Text( ICON_FA_FIRE );//81\xAD"
                ImGui::SameLine();ImGui::Text( ICON_FA_EYE );//81\xAE"
                ImGui::SameLine();ImGui::Text( ICON_FA_EYE_SLASH );//81\xB0"
                ImGui::SameLine();ImGui::Text( ICON_FA_EXCLAMATION_TRIANGLE );//81\xB1"
                ImGui::SameLine();ImGui::Text( ICON_FA_PLANE );//81\xB2"
                ImGui::SameLine();ImGui::Text( ICON_FA_CALENDAR );//81\xB3"
                ImGui::SameLine();ImGui::Text( ICON_FA_RANDOM );//81\xB4"
                ImGui::Text( ICON_FA_COMMENT );//81\xB5"
                ImGui::SameLine();ImGui::Text( ICON_FA_MAGNET );//81\xB6"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHEVRON_UP );//81\xB7"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHEVRON_DOWN );//81\xB8"
                ImGui::SameLine();ImGui::Text( ICON_FA_RETWEET );//81\xB9"
                ImGui::SameLine();ImGui::Text( ICON_FA_SHOPPING_CART );//81\xBA"
                ImGui::SameLine();ImGui::Text( ICON_FA_FOLDER );//81\xBB"
                ImGui::SameLine();ImGui::Text( ICON_FA_FOLDER_OPEN );//81\xBC"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROWS_V );//81\xBD"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROWS_H );//81\xBE"
                ImGui::Text( ICON_FA_BAR_CHART );//82\x80"
                ImGui::SameLine();ImGui::Text( ICON_FA_TWITTER_SQUARE );//82\x81"
                ImGui::SameLine();ImGui::Text( ICON_FA_FACEBOOK_SQUARE );//82\x82"
                ImGui::SameLine();ImGui::Text( ICON_FA_CAMERA_RETRO );//82\x83"
                ImGui::SameLine();ImGui::Text( ICON_FA_KEY );//82\x84"
                ImGui::SameLine();ImGui::Text( ICON_FA_COGS );//82\x85"
                ImGui::SameLine();ImGui::Text( ICON_FA_COMMENTS );//82\x86"
                ImGui::SameLine();ImGui::Text( ICON_FA_THUMBS_O_UP );//82\x87"
                ImGui::SameLine();ImGui::Text( ICON_FA_THUMBS_O_DOWN );//82\x88"
                ImGui::SameLine();ImGui::Text( ICON_FA_STAR_HALF );//82\x89"
                ImGui::Text( ICON_FA_HEART_O );//82\x8A"
                ImGui::SameLine();ImGui::Text( ICON_FA_SIGN_OUT );//82\x8B"
                ImGui::SameLine();ImGui::Text( ICON_FA_LINKEDIN_SQUARE );//82\x8C"
                ImGui::SameLine();ImGui::Text( ICON_FA_THUMB_TACK );//82\x8D"
                ImGui::SameLine();ImGui::Text( ICON_FA_EXTERNAL_LINK );//82\x8E"
                ImGui::SameLine();ImGui::Text( ICON_FA_SIGN_IN );//82\x90"
                ImGui::SameLine();ImGui::Text( ICON_FA_TROPHY );//82\x91"
                ImGui::SameLine();ImGui::Text( ICON_FA_GITHUB_SQUARE );//82\x92"
                ImGui::SameLine();ImGui::Text( ICON_FA_UPLOAD );//82\x93"
                ImGui::SameLine();ImGui::Text( ICON_FA_LEMON_O );//82\x94"
                ImGui::Text( ICON_FA_PHONE );//82\x95"
                ImGui::SameLine();ImGui::Text( ICON_FA_SQUARE_O );//82\x96"
                ImGui::SameLine();ImGui::Text( ICON_FA_BOOKMARK_O );//82\x97"
                ImGui::SameLine();ImGui::Text( ICON_FA_PHONE_SQUARE );//82\x98"
                ImGui::SameLine();ImGui::Text( ICON_FA_TWITTER );//82\x99"
                ImGui::SameLine();ImGui::Text( ICON_FA_FACEBOOK );//82\x9A"
                ImGui::SameLine();ImGui::Text( ICON_FA_GITHUB );//82\x9B"
                ImGui::SameLine();ImGui::Text( ICON_FA_UNLOCK );//82\x9C"
                ImGui::SameLine();ImGui::Text( ICON_FA_CREDIT_CARD );//82\x9D"
                ImGui::SameLine();ImGui::Text( ICON_FA_RSS );//82\x9E"
                ImGui::Text( ICON_FA_HDD_O );//82\xA0"
                ImGui::SameLine();ImGui::Text( ICON_FA_BULLHORN );//82\xA1"
                ImGui::SameLine();ImGui::Text( ICON_FA_BELL );//83\xB3"
                ImGui::SameLine();ImGui::Text( ICON_FA_CERTIFICATE );//82\xA3"
                ImGui::SameLine();ImGui::Text( ICON_FA_HAND_O_RIGHT );//82\xA4"
                ImGui::SameLine();ImGui::Text( ICON_FA_HAND_O_LEFT );//82\xA5"
                ImGui::SameLine();ImGui::Text( ICON_FA_HAND_O_UP );//82\xA6"
                ImGui::SameLine();ImGui::Text( ICON_FA_HAND_O_DOWN );//82\xA7"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROW_CIRCLE_LEFT );//82\xA8"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROW_CIRCLE_RIGHT );//82\xA9"
                ImGui::Text( ICON_FA_ARROW_CIRCLE_UP );//82\xAA"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROW_CIRCLE_DOWN );//82\xAB"
                ImGui::SameLine();ImGui::Text( ICON_FA_GLOBE );//82\xAC"
                ImGui::SameLine();ImGui::Text( ICON_FA_WRENCH );//82\xAD"
                ImGui::SameLine();ImGui::Text( ICON_FA_TASKS );//82\xAE"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILTER );//82\xB0"
                ImGui::SameLine();ImGui::Text( ICON_FA_BRIEFCASE );//82\xB1"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROWS_ALT );//82\xB2"
                ImGui::SameLine();ImGui::Text( ICON_FA_USERS );//83\x80"
                ImGui::SameLine();ImGui::Text( ICON_FA_LINK );//83\x81"
                ImGui::Text( ICON_FA_CLOUD );//83\x82"
                ImGui::SameLine();ImGui::Text( ICON_FA_FLASK );//83\x83"
                ImGui::SameLine();ImGui::Text( ICON_FA_SCISSORS );//83\x84"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILES_O );//83\x85"
                ImGui::SameLine();ImGui::Text( ICON_FA_PAPERCLIP );//83\x86"
                ImGui::SameLine();ImGui::Text( ICON_FA_FLOPPY_O );//83\x87"
                ImGui::SameLine();ImGui::Text( ICON_FA_SQUARE );//83\x88"
                ImGui::SameLine();ImGui::Text( ICON_FA_BARS );//83\x89"
                ImGui::SameLine();ImGui::Text( ICON_FA_LIST_UL );//83\x8A"
                ImGui::SameLine();ImGui::Text( ICON_FA_LIST_OL );//83\x8B"
                ImGui::Text( ICON_FA_STRIKETHROUGH );//83\x8C"
                ImGui::SameLine();ImGui::Text( ICON_FA_UNDERLINE );//83\x8D"
                ImGui::SameLine();ImGui::Text( ICON_FA_TABLE );//83\x8E"
                ImGui::SameLine();ImGui::Text( ICON_FA_MAGIC );//83\x90"
                ImGui::SameLine();ImGui::Text( ICON_FA_TRUCK );//83\x91"
                ImGui::SameLine();ImGui::Text( ICON_FA_PINTEREST );//83\x92"
                ImGui::SameLine();ImGui::Text( ICON_FA_PINTEREST_SQUARE );//83\x93"
                ImGui::SameLine();ImGui::Text( ICON_FA_GOOGLE_PLUS_SQUARE );//83\x94"
                ImGui::SameLine();ImGui::Text( ICON_FA_GOOGLE_PLUS );//83\x95"
                ImGui::SameLine();ImGui::Text( ICON_FA_MONEY );//83\x96"
                ImGui::Text( ICON_FA_CARET_DOWN );//83\x97"
                ImGui::SameLine();ImGui::Text( ICON_FA_CARET_UP );//83\x98"
                ImGui::SameLine();ImGui::Text( ICON_FA_CARET_LEFT );//83\x99"
                ImGui::SameLine();ImGui::Text( ICON_FA_CARET_RIGHT );//83\x9A"
                ImGui::SameLine();ImGui::Text( ICON_FA_COLUMNS );//83\x9B"
                ImGui::SameLine();ImGui::Text( ICON_FA_SORT );//83\x9C"
                ImGui::SameLine();ImGui::Text( ICON_FA_SORT_DESC );//83\x9D"
                ImGui::SameLine();ImGui::Text( ICON_FA_SORT_ASC );//83\x9E"
                ImGui::SameLine();ImGui::Text( ICON_FA_ENVELOPE );//83\xA0"
                ImGui::SameLine();ImGui::Text( ICON_FA_LINKEDIN );//83\xA1"
                ImGui::Text( ICON_FA_UNDO );//83\xA2"
                ImGui::SameLine();ImGui::Text( ICON_FA_GAVEL );//83\xA3"
                ImGui::SameLine();ImGui::Text( ICON_FA_TACHOMETER );//83\xA4"
                ImGui::SameLine();ImGui::Text( ICON_FA_COMMENT_O );//83\xA5"
                ImGui::SameLine();ImGui::Text( ICON_FA_COMMENTS_O );//83\xA6"
                ImGui::SameLine();ImGui::Text( ICON_FA_BOLT );//83\xA7"
                ImGui::SameLine();ImGui::Text( ICON_FA_SITEMAP );//83\xA8"
                ImGui::SameLine();ImGui::Text( ICON_FA_UMBRELLA );//83\xA9"
                ImGui::SameLine();ImGui::Text( ICON_FA_CLIPBOARD );//83\xAA"
                ImGui::SameLine();ImGui::Text( ICON_FA_LIGHTBULB_O );//83\xAB"
                ImGui::Text( ICON_FA_EXCHANGE );//83\xAC"
                ImGui::SameLine();ImGui::Text( ICON_FA_CLOUD_DOWNLOAD );//83\xAD"
                ImGui::SameLine();ImGui::Text( ICON_FA_CLOUD_UPLOAD );//83\xAE"
                ImGui::SameLine();ImGui::Text( ICON_FA_USER_MD );//83\xB0"
                ImGui::SameLine();ImGui::Text( ICON_FA_STETHOSCOPE );//83\xB1"
                ImGui::SameLine();ImGui::Text( ICON_FA_SUITCASE );//83\xB2"
                ImGui::SameLine();ImGui::Text( ICON_FA_BELL_O );//82\xA2"
                ImGui::SameLine();ImGui::Text( ICON_FA_COFFEE );//83\xB4"
                ImGui::SameLine();ImGui::Text( ICON_FA_CUTLERY );//83\xB5"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE_TEXT_O );//83\xB6"
                ImGui::Text( ICON_FA_BUILDING_O );//83\xB7"
                ImGui::SameLine();ImGui::Text( ICON_FA_HOSPITAL_O );//83\xB8"
                ImGui::SameLine();ImGui::Text( ICON_FA_AMBULANCE );//83\xB9"
                ImGui::SameLine();ImGui::Text( ICON_FA_MEDKIT );//83\xBA"
                ImGui::SameLine();ImGui::Text( ICON_FA_FIGHTER_JET );//83\xBB"
                ImGui::SameLine();ImGui::Text( ICON_FA_BEER );//83\xBC"
                ImGui::SameLine();ImGui::Text( ICON_FA_H_SQUARE );//83\xBD"
                ImGui::SameLine();ImGui::Text( ICON_FA_PLUS_SQUARE );//83\xBE"
                ImGui::SameLine();ImGui::Text( ICON_FA_ANGLE_DOUBLE_LEFT );//84\x80"
                ImGui::SameLine();ImGui::Text( ICON_FA_ANGLE_DOUBLE_RIGHT );//84\x81"
                ImGui::Text( ICON_FA_ANGLE_DOUBLE_UP );//84\x82"
                ImGui::SameLine();ImGui::Text( ICON_FA_ANGLE_DOUBLE_DOWN );//84\x83"
                ImGui::SameLine();ImGui::Text( ICON_FA_ANGLE_LEFT );//84\x84"
                ImGui::SameLine();ImGui::Text( ICON_FA_ANGLE_RIGHT );//84\x85"
                ImGui::SameLine();ImGui::Text( ICON_FA_ANGLE_UP );//84\x86"
                ImGui::SameLine();ImGui::Text( ICON_FA_ANGLE_DOWN );//84\x87"
                ImGui::SameLine();ImGui::Text( ICON_FA_DESKTOP );//84\x88"
                ImGui::SameLine();ImGui::Text( ICON_FA_LAPTOP );//84\x89"
                ImGui::SameLine();ImGui::Text( ICON_FA_TABLET );//84\x8A"
                ImGui::SameLine();ImGui::Text( ICON_FA_MOBILE );//84\x8B"
                ImGui::Text( ICON_FA_CIRCLE_O );//84\x8C"
                ImGui::SameLine();ImGui::Text( ICON_FA_QUOTE_LEFT );//84\x8D"
                ImGui::SameLine();ImGui::Text( ICON_FA_QUOTE_RIGHT );//84\x8E"
                ImGui::SameLine();ImGui::Text( ICON_FA_SPINNER );//84\x90"
                ImGui::SameLine();ImGui::Text( ICON_FA_CIRCLE );//84\x91"
                ImGui::SameLine();ImGui::Text( ICON_FA_REPLY );//84\x92"
                ImGui::SameLine();ImGui::Text( ICON_FA_GITHUB_ALT );//84\x93"
                ImGui::SameLine();ImGui::Text( ICON_FA_FOLDER_O );//84\x94"
                ImGui::SameLine();ImGui::Text( ICON_FA_FOLDER_OPEN_O );//84\x95"
                ImGui::SameLine();ImGui::Text( ICON_FA_SMILE_O );//84\x98"
                ImGui::Text( ICON_FA_FROWN_O );//84\x99"
                ImGui::SameLine();ImGui::Text( ICON_FA_MEH_O );//84\x9A"
                ImGui::SameLine();ImGui::Text( ICON_FA_GAMEPAD );//84\x9B"
                ImGui::SameLine();ImGui::Text( ICON_FA_KEYBOARD_O );//84\x9C"
                ImGui::SameLine();ImGui::Text( ICON_FA_FLAG_O );//84\x9D"
                ImGui::SameLine();ImGui::Text( ICON_FA_FLAG_CHECKERED );//84\x9E"
                ImGui::SameLine();ImGui::Text( ICON_FA_TERMINAL );//84\xA0"
                ImGui::SameLine();ImGui::Text( ICON_FA_CODE );//84\xA1"
                ImGui::SameLine();ImGui::Text( ICON_FA_REPLY_ALL );//84\xA2"
                ImGui::SameLine();ImGui::Text( ICON_FA_STAR_HALF_O );//84\xA3"
                ImGui::Text( ICON_FA_LOCATION_ARROW );//84\xA4"
                ImGui::SameLine();ImGui::Text( ICON_FA_CROP );//84\xA5"
                ImGui::SameLine();ImGui::Text( ICON_FA_CODE_FORK );//84\xA6"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHAIN_BROKEN );//84\xA7"
                ImGui::SameLine();ImGui::Text( ICON_FA_QUESTION );//84\xA8"
                ImGui::SameLine();ImGui::Text( ICON_FA_INFO );//84\xA9"
                ImGui::SameLine();ImGui::Text( ICON_FA_EXCLAMATION );//84\xAA"
                ImGui::SameLine();ImGui::Text( ICON_FA_SUPERSCRIPT );//84\xAB"
                ImGui::SameLine();ImGui::Text( ICON_FA_SUBSCRIPT );//84\xAC"
                ImGui::SameLine();ImGui::Text( ICON_FA_ERASER );//84\xAD"
                ImGui::Text( ICON_FA_PUZZLE_PIECE );//84\xAE"
                ImGui::SameLine();ImGui::Text( ICON_FA_MICROPHONE );//84\xB0"
                ImGui::SameLine();ImGui::Text( ICON_FA_MICROPHONE_SLASH );//84\xB1"
                ImGui::SameLine();ImGui::Text( ICON_FA_SHIELD );//84\xB2"
                ImGui::SameLine();ImGui::Text( ICON_FA_CALENDAR_O );//84\xB3"
                ImGui::SameLine();ImGui::Text( ICON_FA_FIRE_EXTINGUISHER );//84\xB4"
                ImGui::SameLine();ImGui::Text( ICON_FA_ROCKET );//84\xB5"
                ImGui::SameLine();ImGui::Text( ICON_FA_MAXCDN );//84\xB6"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHEVRON_CIRCLE_LEFT );//84\xB7"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHEVRON_CIRCLE_RIGHT );//84\xB8"
                ImGui::Text( ICON_FA_CHEVRON_CIRCLE_UP );//84\xB9"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHEVRON_CIRCLE_DOWN );//84\xBA"
                ImGui::SameLine();ImGui::Text( ICON_FA_HTML5 );//84\xBB"
                ImGui::SameLine();ImGui::Text( ICON_FA_CSS3 );//84\xBC"
                ImGui::SameLine();ImGui::Text( ICON_FA_ANCHOR );//84\xBD"
                ImGui::SameLine();ImGui::Text( ICON_FA_UNLOCK_ALT );//84\xBE"
                ImGui::SameLine();ImGui::Text( ICON_FA_BULLSEYE );//85\x80"
                ImGui::SameLine();ImGui::Text( ICON_FA_ELLIPSIS_H );//85\x81"
                ImGui::SameLine();ImGui::Text( ICON_FA_ELLIPSIS_V );//85\x82"
                ImGui::SameLine();ImGui::Text( ICON_FA_RSS_SQUARE );//85\x83"
                ImGui::Text( ICON_FA_PLAY_CIRCLE );//85\x84"
                ImGui::SameLine();ImGui::Text( ICON_FA_TICKET );//85\x85"
                ImGui::SameLine();ImGui::Text( ICON_FA_MINUS_SQUARE );//85\x86"
                ImGui::SameLine();ImGui::Text( ICON_FA_MINUS_SQUARE_O );//85\x87"
                ImGui::SameLine();ImGui::Text( ICON_FA_LEVEL_UP );//85\x88"
                ImGui::SameLine();ImGui::Text( ICON_FA_LEVEL_DOWN );//85\x89"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHECK_SQUARE );//85\x8A"
                ImGui::SameLine();ImGui::Text( ICON_FA_PENCIL_SQUARE );//85\x8B"
                ImGui::SameLine();ImGui::Text( ICON_FA_EXTERNAL_LINK_SQUARE );//85\x8C"
                ImGui::SameLine();ImGui::Text( ICON_FA_SHARE_SQUARE );//85\x8D"
                ImGui::Text( ICON_FA_COMPASS );//85\x8E"
                ImGui::SameLine();ImGui::Text( ICON_FA_CARET_SQUARE_O_DOWN );//85\x90"
                ImGui::SameLine();ImGui::Text( ICON_FA_CARET_SQUARE_O_UP );//85\x91"
                ImGui::SameLine();ImGui::Text( ICON_FA_CARET_SQUARE_O_RIGHT );//85\x92"
                ImGui::SameLine();ImGui::Text( ICON_FA_EUR );//85\x93"
                ImGui::SameLine();ImGui::Text( ICON_FA_GBP );//85\x94"
                ImGui::SameLine();ImGui::Text( ICON_FA_USD );//85\x95"
                ImGui::SameLine();ImGui::Text( ICON_FA_INR );//85\x96"
                ImGui::SameLine();ImGui::Text( ICON_FA_JPY );//85\x97"
                ImGui::SameLine();ImGui::Text( ICON_FA_RUB );//85\x98"
                ImGui::Text( ICON_FA_KRW );//85\x99"
                ImGui::SameLine();ImGui::Text( ICON_FA_BTC );//85\x9A"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE );//85\x9B"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE_TEXT );//85\x9C"
                ImGui::SameLine();ImGui::Text( ICON_FA_SORT_ALPHA_ASC );//85\x9D"
                ImGui::SameLine();ImGui::Text( ICON_FA_SORT_ALPHA_DESC );//85\x9E"
                ImGui::SameLine();ImGui::Text( ICON_FA_SORT_AMOUNT_ASC );//85\xA0"
                ImGui::SameLine();ImGui::Text( ICON_FA_SORT_AMOUNT_DESC );//85\xA1"
                ImGui::SameLine();ImGui::Text( ICON_FA_SORT_NUMERIC_ASC );//85\xA2"
                ImGui::SameLine();ImGui::Text( ICON_FA_SORT_NUMERIC_DESC );//85\xA3"
                ImGui::Text( ICON_FA_THUMBS_UP );//85\xA4"
                ImGui::SameLine();ImGui::Text( ICON_FA_THUMBS_DOWN );//85\xA5"
                ImGui::SameLine();ImGui::Text( ICON_FA_YOUTUBE_SQUARE );//85\xA6"
                ImGui::SameLine();ImGui::Text( ICON_FA_YOUTUBE );//85\xA7"
                ImGui::SameLine();ImGui::Text( ICON_FA_XING );//85\xA8"
                ImGui::SameLine();ImGui::Text( ICON_FA_XING_SQUARE );//85\xA9"
                ImGui::SameLine();ImGui::Text( ICON_FA_YOUTUBE_PLAY );//85\xAA"
                ImGui::SameLine();ImGui::Text( ICON_FA_DROPBOX );//85\xAB"
                ImGui::SameLine();ImGui::Text( ICON_FA_STACK_OVERFLOW );//85\xAC"
                ImGui::SameLine();ImGui::Text( ICON_FA_INSTAGRAM );//85\xAD"
                ImGui::Text( ICON_FA_FLICKR );//85\xAE"
                ImGui::SameLine();ImGui::Text( ICON_FA_ADN );//85\xB0"
                ImGui::SameLine();ImGui::Text( ICON_FA_BITBUCKET );//85\xB1"
                ImGui::SameLine();ImGui::Text( ICON_FA_BITBUCKET_SQUARE );//85\xB2"
                ImGui::SameLine();ImGui::Text( ICON_FA_TUMBLR );//85\xB3"
                ImGui::SameLine();ImGui::Text( ICON_FA_TUMBLR_SQUARE );//85\xB4"
                ImGui::SameLine();ImGui::Text( ICON_FA_LONG_ARROW_DOWN );//85\xB5"
                ImGui::SameLine();ImGui::Text( ICON_FA_LONG_ARROW_UP );//85\xB6"
                ImGui::SameLine();ImGui::Text( ICON_FA_LONG_ARROW_LEFT );//85\xB7"
                ImGui::SameLine();ImGui::Text( ICON_FA_LONG_ARROW_RIGHT );//85\xB8"
                ImGui::Text( ICON_FA_APPLE );//85\xB9"
                ImGui::SameLine();ImGui::Text( ICON_FA_WINDOWS );//85\xBA"
                ImGui::SameLine();ImGui::Text( ICON_FA_ANDROID );//85\xBB"
                ImGui::SameLine();ImGui::Text( ICON_FA_LINUX );//85\xBC"
                ImGui::SameLine();ImGui::Text( ICON_FA_DRIBBBLE );//85\xBD"
                ImGui::SameLine();ImGui::Text( ICON_FA_SKYPE );//85\xBE"
                ImGui::SameLine();ImGui::Text( ICON_FA_FOURSQUARE );//86\x80"
                ImGui::SameLine();ImGui::Text( ICON_FA_TRELLO );//86\x81"
                ImGui::SameLine();ImGui::Text( ICON_FA_FEMALE );//86\x82"
                ImGui::SameLine();ImGui::Text( ICON_FA_MALE );//86\x83"
                ImGui::Text( ICON_FA_GRATIPAY );//86\x84"
                ImGui::SameLine();ImGui::Text( ICON_FA_SUN_O );//86\x85"
                ImGui::SameLine();ImGui::Text( ICON_FA_MOON_O );//86\x86"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARCHIVE );//86\x87"
                ImGui::SameLine();ImGui::Text( ICON_FA_BUG );//86\x88"
                ImGui::SameLine();ImGui::Text( ICON_FA_VK );//86\x89"
                ImGui::SameLine();ImGui::Text( ICON_FA_WEIBO );//86\x8A"
                ImGui::SameLine();ImGui::Text( ICON_FA_RENREN );//86\x8B"
                ImGui::SameLine();ImGui::Text( ICON_FA_PAGELINES );//86\x8C"
                ImGui::SameLine();ImGui::Text( ICON_FA_STACK_EXCHANGE );//86\x8D"
                ImGui::Text( ICON_FA_ARROW_CIRCLE_O_RIGHT );//86\x8E"
                ImGui::SameLine();ImGui::Text( ICON_FA_ARROW_CIRCLE_O_LEFT );//86\x90"
                ImGui::SameLine();ImGui::Text( ICON_FA_CARET_SQUARE_O_LEFT );//86\x91"
                ImGui::SameLine();ImGui::Text( ICON_FA_DOT_CIRCLE_O );//86\x92"
                ImGui::SameLine();ImGui::Text( ICON_FA_WHEELCHAIR );//86\x93"
                ImGui::SameLine();ImGui::Text( ICON_FA_VIMEO_SQUARE );//86\x94"
                ImGui::SameLine();ImGui::Text( ICON_FA_TRY );//86\x95"
                ImGui::SameLine();ImGui::Text( ICON_FA_PLUS_SQUARE_O );//86\x96"
                ImGui::SameLine();ImGui::Text( ICON_FA_SPACE_SHUTTLE );//86\x97"
                ImGui::SameLine();ImGui::Text( ICON_FA_SLACK );//86\x98"
                ImGui::Text( ICON_FA_ENVELOPE_SQUARE );//86\x99"
                ImGui::SameLine();ImGui::Text( ICON_FA_WORDPRESS );//86\x9A"
                ImGui::SameLine();ImGui::Text( ICON_FA_OPENID );//86\x9B"
                ImGui::SameLine();ImGui::Text( ICON_FA_UNIVERSITY );//86\x9C"
                ImGui::SameLine();ImGui::Text( ICON_FA_GRADUATION_CAP );//86\x9D"
                ImGui::SameLine();ImGui::Text( ICON_FA_YAHOO );//86\x9E"
                ImGui::SameLine();ImGui::Text( ICON_FA_GOOGLE );//86\xA0"
                ImGui::SameLine();ImGui::Text( ICON_FA_REDDIT );//86\xA1"
                ImGui::SameLine();ImGui::Text( ICON_FA_REDDIT_SQUARE );//86\xA2"
                ImGui::SameLine();ImGui::Text( ICON_FA_STUMBLEUPON_CIRCLE );//86\xA3"
                ImGui::Text( ICON_FA_STUMBLEUPON );//86\xA4"
                ImGui::SameLine();ImGui::Text( ICON_FA_DELICIOUS );//86\xA5"
                ImGui::SameLine();ImGui::Text( ICON_FA_DIGG );//86\xA6"
                ImGui::SameLine();ImGui::Text( ICON_FA_PIED_PIPER );//86\xA7"
                ImGui::SameLine();ImGui::Text( ICON_FA_PIED_PIPER_ALT );//86\xA8"
                ImGui::SameLine();ImGui::Text( ICON_FA_DRUPAL );//86\xA9"
                ImGui::SameLine();ImGui::Text( ICON_FA_JOOMLA );//86\xAA"
                ImGui::SameLine();ImGui::Text( ICON_FA_LANGUAGE );//86\xAB"
                ImGui::SameLine();ImGui::Text( ICON_FA_FAX );//86\xAC"
                ImGui::SameLine();ImGui::Text( ICON_FA_BUILDING );//86\xAD"
                ImGui::Text( ICON_FA_CHILD );//86\xAE"
                ImGui::SameLine();ImGui::Text( ICON_FA_PAW );//86\xB0"
                ImGui::SameLine();ImGui::Text( ICON_FA_SPOON );//86\xB1"
                ImGui::SameLine();ImGui::Text( ICON_FA_CUBE );//86\xB2"
                ImGui::SameLine();ImGui::Text( ICON_FA_CUBES );//86\xB3"
                ImGui::SameLine();ImGui::Text( ICON_FA_BEHANCE );//86\xB4"
                ImGui::SameLine();ImGui::Text( ICON_FA_BEHANCE_SQUARE );//86\xB5"
                ImGui::SameLine();ImGui::Text( ICON_FA_STEAM );//86\xB6"
                ImGui::SameLine();ImGui::Text( ICON_FA_STEAM_SQUARE );//86\xB7"
                ImGui::SameLine();ImGui::Text( ICON_FA_RECYCLE );//86\xB8"
                ImGui::Text( ICON_FA_CAR );//86\xB9"
                ImGui::SameLine();ImGui::Text( ICON_FA_TAXI );//86\xBA"
                ImGui::SameLine();ImGui::Text( ICON_FA_TREE );//86\xBB"
                ImGui::SameLine();ImGui::Text( ICON_FA_SPOTIFY );//86\xBC"
                ImGui::SameLine();ImGui::Text( ICON_FA_DEVIANTART );//86\xBD"
                ImGui::SameLine();ImGui::Text( ICON_FA_SOUNDCLOUD );//86\xBE"
                ImGui::SameLine();ImGui::Text( ICON_FA_DATABASE );//87\x80"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE_PDF_O );//87\x81"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE_WORD_O );//87\x82"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE_EXCEL_O );//87\x83"
                ImGui::Text( ICON_FA_FILE_POWERPOINT_O );//87\x84"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE_IMAGE_O );//87\x85"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE_ARCHIVE_O );//87\x86"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE_AUDIO_O );//87\x87"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE_VIDEO_O );//87\x88"
                ImGui::SameLine();ImGui::Text( ICON_FA_FILE_CODE_O );//87\x89"
                ImGui::SameLine();ImGui::Text( ICON_FA_VINE );//87\x8A"
                ImGui::SameLine();ImGui::Text( ICON_FA_CODEPEN );//87\x8B"
                ImGui::SameLine();ImGui::Text( ICON_FA_JSFIDDLE );//87\x8C"
                ImGui::SameLine();ImGui::Text( ICON_FA_LIFE_RING );//87\x8D"
                ImGui::Text( ICON_FA_CIRCLE_O_NOTCH );//87\x8E"
                ImGui::SameLine();ImGui::Text( ICON_FA_REBEL );//87\x90"
                ImGui::SameLine();ImGui::Text( ICON_FA_EMPIRE );//87\x91"
                ImGui::SameLine();ImGui::Text( ICON_FA_GIT_SQUARE );//87\x92"
                ImGui::SameLine();ImGui::Text( ICON_FA_GIT );//87\x93"
                ImGui::SameLine();ImGui::Text( ICON_FA_HACKER_NEWS );//87\x94"
                ImGui::SameLine();ImGui::Text( ICON_FA_TENCENT_WEIBO );//87\x95"
                ImGui::SameLine();ImGui::Text( ICON_FA_QQ );//87\x96"
                ImGui::SameLine();ImGui::Text( ICON_FA_WEIXIN );//87\x97"
                ImGui::SameLine();ImGui::Text( ICON_FA_PAPER_PLANE );//87\x98"
                ImGui::Text( ICON_FA_PAPER_PLANE_O );//87\x99"
                ImGui::SameLine();ImGui::Text( ICON_FA_HISTORY );//87\x9A"
                ImGui::SameLine();ImGui::Text( ICON_FA_CIRCLE_THIN );//87\x9B"
                ImGui::SameLine();ImGui::Text( ICON_FA_HEADER );//87\x9C"
                ImGui::SameLine();ImGui::Text( ICON_FA_PARAGRAPH );//87\x9D"
                ImGui::SameLine();ImGui::Text( ICON_FA_SLIDERS );//87\x9E"
                ImGui::SameLine();ImGui::Text( ICON_FA_SHARE_ALT );//87\xA0"
                ImGui::SameLine();ImGui::Text( ICON_FA_SHARE_ALT_SQUARE );//87\xA1"
                ImGui::SameLine();ImGui::Text( ICON_FA_BOMB );//87\xA2"
                ImGui::SameLine();ImGui::Text( ICON_FA_FUTBOL_O );//87\xA3"
                ImGui::Text( ICON_FA_TTY );//87\xA4"
                ImGui::SameLine();ImGui::Text( ICON_FA_BINOCULARS );//87\xA5"
                ImGui::SameLine();ImGui::Text( ICON_FA_PLUG );//87\xA6"
                ImGui::SameLine();ImGui::Text( ICON_FA_SLIDESHARE );//87\xA7"
                ImGui::SameLine();ImGui::Text( ICON_FA_TWITCH );//87\xA8"
                ImGui::SameLine();ImGui::Text( ICON_FA_YELP );//87\xA9"
                ImGui::SameLine();ImGui::Text( ICON_FA_NEWSPAPER_O );//87\xAA"
                ImGui::SameLine();ImGui::Text( ICON_FA_WIFI );//87\xAB"
                ImGui::SameLine();ImGui::Text( ICON_FA_CALCULATOR );//87\xAC"
                ImGui::SameLine();ImGui::Text( ICON_FA_PAYPAL );//87\xAD"
                ImGui::Text( ICON_FA_GOOGLE_WALLET );//87\xAE"
                ImGui::SameLine();ImGui::Text( ICON_FA_CC_VISA );//87\xB0"
                ImGui::SameLine();ImGui::Text( ICON_FA_CC_MASTERCARD );//87\xB1"
                ImGui::SameLine();ImGui::Text( ICON_FA_CC_DISCOVER );//87\xB2"
                ImGui::SameLine();ImGui::Text( ICON_FA_CC_AMEX );//87\xB3"
                ImGui::SameLine();ImGui::Text( ICON_FA_CC_PAYPAL );//87\xB4"
                ImGui::SameLine();ImGui::Text( ICON_FA_CC_STRIPE );//87\xB5"
                ImGui::SameLine();ImGui::Text( ICON_FA_BELL_SLASH );//87\xB6"
                ImGui::SameLine();ImGui::Text( ICON_FA_BELL_SLASH_O );//87\xB7"
                ImGui::SameLine();ImGui::Text( ICON_FA_TRASH );//87\xB8"
                ImGui::Text( ICON_FA_COPYRIGHT );//87\xB9"
                ImGui::SameLine();ImGui::Text( ICON_FA_AT );//87\xBA"
                ImGui::SameLine();ImGui::Text( ICON_FA_EYEDROPPER );//87\xBB"
                ImGui::SameLine();ImGui::Text( ICON_FA_PAINT_BRUSH );//87\xBC"
                ImGui::SameLine();ImGui::Text( ICON_FA_BIRTHDAY_CAKE );//87\xBD"
                ImGui::SameLine();ImGui::Text( ICON_FA_AREA_CHART );//87\xBE"
                ImGui::SameLine();ImGui::Text( ICON_FA_PIE_CHART );//88\x80"
                ImGui::SameLine();ImGui::Text( ICON_FA_LINE_CHART );//88\x81"
                ImGui::SameLine();ImGui::Text( ICON_FA_LASTFM );//88\x82"
                ImGui::SameLine();ImGui::Text( ICON_FA_LASTFM_SQUARE );//88\x83"
                ImGui::Text( ICON_FA_TOGGLE_OFF );//88\x84"
                ImGui::SameLine();ImGui::Text( ICON_FA_TOGGLE_ON );//88\x85"
                ImGui::SameLine();ImGui::Text( ICON_FA_BICYCLE );//88\x86"
                ImGui::SameLine();ImGui::Text( ICON_FA_BUS );//88\x87"
                ImGui::SameLine();ImGui::Text( ICON_FA_IOXHOST );//88\x88"
                ImGui::SameLine();ImGui::Text( ICON_FA_ANGELLIST );//88\x89"
                ImGui::SameLine();ImGui::Text( ICON_FA_CC );//88\x8A"
                ImGui::SameLine();ImGui::Text( ICON_FA_ILS );//88\x8B"
                ImGui::SameLine();ImGui::Text( ICON_FA_MEANPATH );//88\x8C"
                ImGui::SameLine();ImGui::Text( ICON_FA_BUYSELLADS );//88\x8D"
                ImGui::Text( ICON_FA_CONNECTDEVELOP );//88\x8E"
                ImGui::SameLine();ImGui::Text( ICON_FA_DASHCUBE );//88\x90"
                ImGui::SameLine();ImGui::Text( ICON_FA_FORUMBEE );//88\x91"
                ImGui::SameLine();ImGui::Text( ICON_FA_LEANPUB );//88\x92"
                ImGui::SameLine();ImGui::Text( ICON_FA_SELLSY );//88\x93"
                ImGui::SameLine();ImGui::Text( ICON_FA_SHIRTSINBULK );//88\x94"
                ImGui::SameLine();ImGui::Text( ICON_FA_SIMPLYBUILT );//88\x95"
                ImGui::SameLine();ImGui::Text( ICON_FA_SKYATLAS );//88\x96"
                ImGui::SameLine();ImGui::Text( ICON_FA_CART_PLUS );//88\x97"
                ImGui::SameLine();ImGui::Text( ICON_FA_CART_ARROW_DOWN );//88\x98"
                ImGui::Text( ICON_FA_DIAMOND );//88\x99"
                ImGui::SameLine();ImGui::Text( ICON_FA_SHIP );//88\x9A"
                ImGui::SameLine();ImGui::Text( ICON_FA_USER_SECRET );//88\x9B"
                ImGui::SameLine();ImGui::Text( ICON_FA_MOTORCYCLE );//88\x9C"
                ImGui::SameLine();ImGui::Text( ICON_FA_STREET_VIEW );//88\x9D"
                ImGui::SameLine();ImGui::Text( ICON_FA_HEARTBEAT );//88\x9E"
                ImGui::SameLine();ImGui::Text( ICON_FA_VENUS );//88\xA1"
                ImGui::SameLine();ImGui::Text( ICON_FA_MARS );//88\xA2"
                ImGui::SameLine();ImGui::Text( ICON_FA_MERCURY );//88\xA3"
                ImGui::SameLine();ImGui::Text( ICON_FA_TRANSGENDER );//88\xA4"
                ImGui::Text( ICON_FA_TRANSGENDER_ALT );//88\xA5"
                ImGui::SameLine();ImGui::Text( ICON_FA_VENUS_DOUBLE );//88\xA6"
                ImGui::SameLine();ImGui::Text( ICON_FA_MARS_DOUBLE );//88\xA7"
                ImGui::SameLine();ImGui::Text( ICON_FA_VENUS_MARS );//88\xA8"
                ImGui::SameLine();ImGui::Text( ICON_FA_MARS_STROKE );//88\xA9"
                ImGui::SameLine();ImGui::Text( ICON_FA_MARS_STROKE_V );//88\xAA"
                ImGui::SameLine();ImGui::Text( ICON_FA_MARS_STROKE_H );//88\xAB"
                ImGui::SameLine();ImGui::Text( ICON_FA_NEUTER );//88\xAC"
                ImGui::SameLine();ImGui::Text( ICON_FA_GENDERLESS );//88\xAD"
                ImGui::SameLine();ImGui::Text( ICON_FA_FACEBOOK_OFFICIAL );//88\xB0"
                ImGui::Text( ICON_FA_PINTEREST_P );//88\xB1"
                ImGui::SameLine();ImGui::Text( ICON_FA_WHATSAPP );//88\xB2"
                ImGui::SameLine();ImGui::Text( ICON_FA_SERVER );//88\xB3"
                ImGui::SameLine();ImGui::Text( ICON_FA_USER_PLUS );//88\xB4"
                ImGui::SameLine();ImGui::Text( ICON_FA_USER_TIMES );//88\xB5"
                ImGui::SameLine();ImGui::Text( ICON_FA_BED );//88\xB6"
                ImGui::SameLine();ImGui::Text( ICON_FA_VIACOIN );//88\xB7"
                ImGui::SameLine();ImGui::Text( ICON_FA_TRAIN );//88\xB8"
                ImGui::SameLine();ImGui::Text( ICON_FA_SUBWAY );//88\xB9"
                ImGui::SameLine();ImGui::Text( ICON_FA_MEDIUM );//88\xBA"
                ImGui::Text( ICON_FA_Y_COMBINATOR );//88\xBB"
                ImGui::SameLine();ImGui::Text( ICON_FA_OPTIN_MONSTER );//88\xBC"
                ImGui::SameLine();ImGui::Text( ICON_FA_OPENCART );//88\xBD"
                ImGui::SameLine();ImGui::Text( ICON_FA_EXPEDITEDSSL );//88\xBE"
                ImGui::SameLine();ImGui::Text( ICON_FA_BATTERY_FULL );//89\x80"
                ImGui::SameLine();ImGui::Text( ICON_FA_BATTERY_THREE_QUARTERS );//89\x81"
                ImGui::SameLine();ImGui::Text( ICON_FA_BATTERY_HALF );//89\x82"
                ImGui::SameLine();ImGui::Text( ICON_FA_BATTERY_QUARTER );//89\x83"
                ImGui::SameLine();ImGui::Text( ICON_FA_BATTERY_EMPTY );//89\x84"
                ImGui::SameLine();ImGui::Text( ICON_FA_MOUSE_POINTER );//89\x85"
                ImGui::Text( ICON_FA_I_CURSOR );//89\x86"
                ImGui::SameLine();ImGui::Text( ICON_FA_OBJECT_GROUP );//89\x87"
                ImGui::SameLine();ImGui::Text( ICON_FA_OBJECT_UNGROUP );//89\x88"
                ImGui::SameLine();ImGui::Text( ICON_FA_STICKY_NOTE );//89\x89"
                ImGui::SameLine();ImGui::Text( ICON_FA_STICKY_NOTE_O );//89\x8A"
                ImGui::SameLine();ImGui::Text( ICON_FA_CC_JCB );//89\x8B"
                ImGui::SameLine();ImGui::Text( ICON_FA_CC_DINERS_CLUB );//89\x8C"
                ImGui::SameLine();ImGui::Text( ICON_FA_CLONE );//89\x8D"
                ImGui::SameLine();ImGui::Text( ICON_FA_BALANCE_SCALE );//89\x8E"
                ImGui::SameLine();ImGui::Text( ICON_FA_HOURGLASS_O );//89\x90"
                ImGui::Text( ICON_FA_HOURGLASS_START );//89\x91"
                ImGui::SameLine();ImGui::Text( ICON_FA_HOURGLASS_HALF );//89\x92"
                ImGui::SameLine();ImGui::Text( ICON_FA_HOURGLASS_END );//89\x93"
                ImGui::SameLine();ImGui::Text( ICON_FA_HOURGLASS );//89\x94"
                ImGui::SameLine();ImGui::Text( ICON_FA_HAND_ROCK_O );//89\x95"
                ImGui::SameLine();ImGui::Text( ICON_FA_HAND_PAPER_O );//89\x96"
                ImGui::SameLine();ImGui::Text( ICON_FA_HAND_SCISSORS_O );//89\x97"
                ImGui::SameLine();ImGui::Text( ICON_FA_HAND_LIZARD_O );//89\x98"
                ImGui::SameLine();ImGui::Text( ICON_FA_HAND_SPOCK_O );//89\x99"
                ImGui::SameLine();ImGui::Text( ICON_FA_HAND_POINTER_O );//89\x9A"
                ImGui::Text( ICON_FA_HAND_PEACE_O );//89\x9B"
                ImGui::SameLine();ImGui::Text( ICON_FA_TRADEMARK );//89\x9C"
                ImGui::SameLine();ImGui::Text( ICON_FA_REGISTERED );//89\x9D"
                ImGui::SameLine();ImGui::Text( ICON_FA_CREATIVE_COMMONS );//89\x9E"
                ImGui::SameLine();ImGui::Text( ICON_FA_GG );//89\xA0"
                ImGui::SameLine();ImGui::Text( ICON_FA_GG_CIRCLE );//89\xA1"
                ImGui::SameLine();ImGui::Text( ICON_FA_TRIPADVISOR );//89\xA2"
                ImGui::SameLine();ImGui::Text( ICON_FA_ODNOKLASSNIKI );//89\xA3"
                ImGui::SameLine();ImGui::Text( ICON_FA_ODNOKLASSNIKI_SQUARE );//89\xA4"
                ImGui::SameLine();ImGui::Text( ICON_FA_GET_POCKET );//89\xA5"
                ImGui::Text( ICON_FA_WIKIPEDIA_W );//89\xA6"
                ImGui::SameLine();ImGui::Text( ICON_FA_SAFARI );//89\xA7"
                ImGui::SameLine();ImGui::Text( ICON_FA_CHROME );//89\xA8"
                ImGui::SameLine();ImGui::Text( ICON_FA_FIREFOX );//89\xA9"
                ImGui::SameLine();ImGui::Text( ICON_FA_OPERA );//89\xAA"
                ImGui::SameLine();ImGui::Text( ICON_FA_INTERNET_EXPLORER );//89\xAB"
                ImGui::SameLine();ImGui::Text( ICON_FA_TELEVISION );//89\xAC"
                ImGui::SameLine();ImGui::Text( ICON_FA_CONTAO );//89\xAD"
                ImGui::SameLine();ImGui::Text( ICON_FA_500PX );//89\xAE"
                ImGui::SameLine();ImGui::Text( ICON_FA_AMAZON );//89\xB0"
                ImGui::Text( ICON_FA_CALENDAR_PLUS_O );//89\xB1"
                ImGui::SameLine();ImGui::Text( ICON_FA_CALENDAR_MINUS_O );//89\xB2"
                ImGui::SameLine();ImGui::Text( ICON_FA_CALENDAR_TIMES_O );//89\xB3"
                ImGui::SameLine();ImGui::Text( ICON_FA_CALENDAR_CHECK_O );//89\xB4"
                ImGui::SameLine();ImGui::Text( ICON_FA_INDUSTRY );//89\xB5"
                ImGui::SameLine();ImGui::Text( ICON_FA_MAP_PIN );//89\xB6"
                ImGui::SameLine();ImGui::Text( ICON_FA_MAP_SIGNS );//89\xB7"
                ImGui::SameLine();ImGui::Text( ICON_FA_MAP_O );//89\xB8"
                ImGui::SameLine();ImGui::Text( ICON_FA_MAP );//89\xB9"
                ImGui::SameLine();ImGui::Text( ICON_FA_COMMENTING );//89\xBA"
                ImGui::Text( ICON_FA_COMMENTING_O );//89\xBB"
                ImGui::SameLine();ImGui::Text( ICON_FA_HOUZZ );//89\xBC"
                ImGui::SameLine();ImGui::Text( ICON_FA_VIMEO );//89\xBD"
                ImGui::SameLine();ImGui::Text( ICON_FA_BLACK_TIE );//89\xBE"
                ImGui::SameLine();ImGui::Text( ICON_FA_FONTICONS );//8A\x80"
                ImGui::SameLine();ImGui::Text( ICON_FA_REDDIT_ALIEN );//8A\x81"
                ImGui::SameLine();ImGui::Text( ICON_FA_EDGE );//8A\x82"
                ImGui::SameLine();ImGui::Text( ICON_FA_CREDIT_CARD_ALT );//8A\x83"
                ImGui::SameLine();ImGui::Text( ICON_FA_CODIEPIE );//8A\x84"
                ImGui::SameLine();ImGui::Text( ICON_FA_MODX );//8A\x85"
                ImGui::Text( ICON_FA_FORT_AWESOME );//8A\x86"
                ImGui::SameLine();ImGui::Text( ICON_FA_USB );//8A\x87"
                ImGui::SameLine();ImGui::Text( ICON_FA_PRODUCT_HUNT );//8A\x88"
                ImGui::SameLine();ImGui::Text( ICON_FA_MIXCLOUD );//8A\x89"
                ImGui::SameLine();ImGui::Text( ICON_FA_SCRIBD );//8A\x8A"
                ImGui::SameLine();ImGui::Text( ICON_FA_PAUSE_CIRCLE );//8A\x8B"
                ImGui::SameLine();ImGui::Text( ICON_FA_PAUSE_CIRCLE_O );//8A\x8C"
                ImGui::SameLine();ImGui::Text( ICON_FA_STOP_CIRCLE );//8A\x8D"
                ImGui::SameLine();ImGui::Text( ICON_FA_STOP_CIRCLE_O );//8A\x8E"
                ImGui::SameLine();ImGui::Text( ICON_FA_SHOPPING_BAG );//8A\x90"
                ImGui::Text( ICON_FA_SHOPPING_BASKET );//8A\x91"
                ImGui::SameLine();ImGui::Text( ICON_FA_HASHTAG );//8A\x92"
                ImGui::SameLine();ImGui::Text( ICON_FA_BLUETOOTH );//8A\x93"
                ImGui::SameLine();ImGui::Text( ICON_FA_BLUETOOTH_B );//8A\x94"
                ImGui::SameLine();ImGui::Text( ICON_FA_PERCENT );//8A\x95"
                ImGui::SameLine();ImGui::Text( ICON_FA_GITLAB );//8A\x96"
                ImGui::SameLine();ImGui::Text( ICON_FA_WPBEGINNER );//8A\x97"
                ImGui::SameLine();ImGui::Text( ICON_FA_WPFORMS );//8A\x98"
                ImGui::SameLine();ImGui::Text( ICON_FA_ENVIRA );//8A\x99"
                ImGui::SameLine();ImGui::Text( ICON_FA_UNIVERSAL_ACCESS );//8A\x9A"
                ImGui::Text( ICON_FA_WHEELCHAIR_ALT );//8A\x9B"
                ImGui::SameLine();ImGui::Text( ICON_FA_QUESTION_CIRCLE_O );//8A\x9C"
                ImGui::SameLine();ImGui::Text( ICON_FA_BLIND );//8A\x9D"
                ImGui::SameLine();ImGui::Text( ICON_FA_AUDIO_DESCRIPTION );//8A\x9E"
                ImGui::SameLine();ImGui::Text( ICON_FA_VOLUME_CONTROL_PHONE );//8A\xA0"
                ImGui::SameLine();ImGui::Text( ICON_FA_BRAILLE );//8A\xA1"
                ImGui::SameLine();ImGui::Text( ICON_FA_ASSISTIVE_LISTENING_SYSTEMS );//8A\xA2"
                ImGui::SameLine();ImGui::Text( ICON_FA_AMERICAN_SIGN_LANGUAGE_INTERPRETING );//8A\xA3"
                ImGui::SameLine();ImGui::Text( ICON_FA_DEAF );//8A\xA4"
                ImGui::SameLine();ImGui::Text( ICON_FA_GLIDE );//8A\xA5"
                ImGui::Text( ICON_FA_GLIDE_G );//8A\xA6"
                ImGui::SameLine();ImGui::Text( ICON_FA_SIGN_LANGUAGE );//8A\xA7"
                ImGui::SameLine();ImGui::Text( ICON_FA_LOW_VISION );//8A\xA8"
                ImGui::SameLine();ImGui::Text( ICON_FA_VIADEO );//8A\xA9"
                ImGui::SameLine();ImGui::Text( ICON_FA_VIADEO_SQUARE );//8A\xAA"
                ImGui::SameLine();ImGui::Text( ICON_FA_SNAPCHAT );//8A\xAB"
                ImGui::SameLine();ImGui::Text( ICON_FA_SNAPCHAT_GHOST );//8A\xAC"
                ImGui::SameLine();ImGui::Text( ICON_FA_SNAPCHAT_SQUARE );//8A\xAD"
}
#endif //TEST_ICONS_INSIDE_TTF
