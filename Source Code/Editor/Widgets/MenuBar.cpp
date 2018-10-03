#include "stdafx.h"

#include "Headers/MenuBar.h"

#include "Editor/Headers/Editor.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/PlatformContext.h"

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
    if ((_isMainMenu ? ImGui::BeginMainMenuBar() : ImGui::BeginMenuBar()))
    {
        drawFileMenu();
        drawEditMenu();
        drawProjectMenu();
        drawObjectMenu();
        drawToolsMenu();
        drawWindowsMenu();
        drawHelpMenu();

        _isMainMenu ? ImGui::EndMainMenuBar() : ImGui::EndMenuBar();
    }
}

void MenuBar::drawFileMenu() {
    if (ImGui::BeginMenu("File"))
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
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
        }
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
        if (ImGui::MenuItem("Quit", "Alt+F4")) {
            context().app().RequestShutdown();
        }

        ImGui::EndMenu();
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
        if (ImGui::MenuItem("Save Panel Layout", "CTRL+P+S")) {
            Attorney::EditorPanelManager::savePanelLayout(_context.editor());
        }
        if (ImGui::MenuItem("Load Panel Layout", "CTRL+P+S")) {
            Attorney::EditorPanelManager::loadPanelLayout(_context.editor());
        }
        if (ImGui::MenuItem("Save Tabs Layout", "CTRL+T+S")) {
            Attorney::EditorPanelManager::saveTabLayout(_context.editor());
        }
        if (ImGui::MenuItem("Load Tabs Layout", "CTRL+T+S")) {
            Attorney::EditorPanelManager::loadTabLayout(_context.editor());
        }
        if (ImGui::MenuItem("Save all layouts", "CTRL+A+S")) {
            Attorney::EditorPanelManager::savePanelLayout(_context.editor());
            Attorney::EditorPanelManager::saveTabLayout(_context.editor());
        }
        if (ImGui::MenuItem("Load all layouts", "CTRL+A+S")) {
            Attorney::EditorPanelManager::loadPanelLayout(_context.editor());
            Attorney::EditorPanelManager::loadTabLayout(_context.editor());
        }
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
        ImGui::EndMenu();
    }
}
}; //namespace Divide