#include "stdafx.h"

#include "Headers/MenuBar.h"

#include "Editor/Headers/Editor.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/PlatformContext.h"

#include <imgui/addons/imguifilesystem/imguifilesystem.h>

namespace Divide {
MenuBar::MenuBar(PlatformContext& context, bool mainMenu)
    : PlatformContextComponent(context),
      _isMainMenu(mainMenu)
{

}

MenuBar::~MenuBar()
{

}

void MenuBar::draw() {
    if ((ImGui::BeginMenuBar()))
    {
        drawFileMenu();
        drawEditMenu();
        drawProjectMenu();
        drawObjectMenu();
        drawToolsMenu();
        drawWindowsMenu();
        drawHelpMenu();

        ImGui::EndMenuBar();
    }
}

void MenuBar::drawFileMenu() {
    bool showFileOpenDialog = false;
    bool showFileSaveDialog = false;

    bool hasUnsavedElements = Attorney::EditorPanelManager::hasUnsavedElements(_context.editor());

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New")) {}
        showFileOpenDialog = ImGui::MenuItem("Open", "Ctrl+O");
        if (ImGui::BeginMenu("Open Recent"))
        {
            ImGui::MenuItem("_PLACEHOLDER_A");
            ImGui::MenuItem("_PLACEHOLDER_B");
            if (ImGui::BeginMenu("More.."))
            {
                ImGui::MenuItem("_PLACEHOLDER_C");
                ImGui::MenuItem("_PLACEHOLDER_D");
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        showFileSaveDialog = ImGui::MenuItem(hasUnsavedElements ? "Save*" : "Save", "Ctrl+S");
        
        if (ImGui::MenuItem(hasUnsavedElements ? "Save All*" : "Save All")) {
            Attorney::EditorPanelManager::saveElement(_context.editor(), -1);
        }

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
        if (ImGui::MenuItem("Quit", "Alt+F4")) {
            context().app().RequestShutdown();
        }

        ImGui::EndMenu();
    }

    static ImGuiFs::Dialog s_fileOpenDialog(true, true);
    static ImGuiFs::Dialog s_fileSaveDialog(true, true);
    s_fileOpenDialog.chooseFileDialog(showFileOpenDialog);
    s_fileSaveDialog.chooseFileDialog(showFileSaveDialog);

    if (strlen(s_fileOpenDialog.getChosenPath()) > 0) {
        ImGui::Text("Chosen file: \"%s\"", s_fileOpenDialog.getChosenPath());
    }

    if (strlen(s_fileSaveDialog.getChosenPath()) > 0) {
        ImGui::Text("Chosen file: \"%s\"", s_fileSaveDialog.getChosenPath());
        Attorney::EditorPanelManager::saveElement(_context.editor(), -1);
    }
}

void MenuBar::drawEditMenu() {
    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
        if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
        ImGui::Separator();
        if (ImGui::MenuItem("Cut", "CTRL+X")) {}
        if (ImGui::MenuItem("Copy", "CTRL+C")) {}
        if (ImGui::MenuItem("Paste", "CTRL+V")) {}
        ImGui::Separator();
        ImGui::EndMenu();
    }
}
void MenuBar::drawProjectMenu() {
    if (ImGui::BeginMenu("Project"))
    {
        ImGui::EndMenu();
    }
}
void MenuBar::drawObjectMenu() {
    if (ImGui::BeginMenu("Object"))
    {
        ImGui::EndMenu();
    }

}
void MenuBar::drawToolsMenu() {
    if (ImGui::BeginMenu("Tools"))
    {
        ImGui::EndMenu();
    }

}
void MenuBar::drawWindowsMenu() {
    if (ImGui::BeginMenu("Window"))
    {
        bool showDebugWindow = Attorney::EditorPanelManager::showDebugWindow(_context.editor());
        if (ImGui::MenuItem("Debug Window", NULL, &showDebugWindow)) {
            Attorney::EditorPanelManager::showDebugWindow(_context.editor(), showDebugWindow);
        }

        bool showSampleWindow = Attorney::EditorPanelManager::showSampleWindow(_context.editor());
        if (ImGui::MenuItem("Sample Window", NULL, &showSampleWindow)) {
            Attorney::EditorPanelManager::showSampleWindow(_context.editor(), showSampleWindow);
        }

        ImGui::EndMenu();
    }

}
void MenuBar::drawHelpMenu() {
    if (ImGui::BeginMenu("Help"))
    {
        ImGui::Text("Copyright(c) 2018 DIVIDE - Studio");
        ImGui::Text("Copyright(c) 2009 Ionut Cava");
        ImGui::EndMenu();
    }
}
}; //namespace Divide