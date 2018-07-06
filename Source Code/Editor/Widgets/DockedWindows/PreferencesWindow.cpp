#include "stdafx.h"

#include "Headers/PreferencesWindow.h"
#include "Widgets/Headers/PanelManager.h"

namespace Divide {
    PreferencesWindow::PreferencesWindow(PanelManager& parent)
        : DockedWindow(parent, "Preferences")
    {

    }

    PreferencesWindow::~PreferencesWindow()
    {

    }

    void PreferencesWindow::draw() {
        ImGui::PanelManager& mgr = _parent.ImGuiPanelManager();

        ImGui::DragFloat("Window Alpha##WA1", &mgr.getDockedWindowsAlpha(), 0.005f, -0.01f, 1.0f, mgr.getDockedWindowsAlpha() < 0.0f ? "(default)" : "%.3f");

        bool noTitleBar = mgr.getDockedWindowsNoTitleBar();
        if (ImGui::Checkbox("No Window TitleBars", &noTitleBar)) mgr.setDockedWindowsNoTitleBar(noTitleBar);
        if (Attorney::PanelManagerDockedWindows::showCentralWindow(_parent)) {
            ImGui::SameLine();
            ImGui::Checkbox("Show Central Wndow", (bool*)Attorney::PanelManagerDockedWindows::showCentralWindow(_parent));
        }

        // Here we test saving/loading the ImGui::PanelManager layout (= the sizes of the 4 docked windows and the buttons that are selected on the 4 toolbars)
        // Please note that the API should allow loading/saving different items into a single file and loading/saving from/to memory too, but we don't show it now.

        ImGui::Separator();
        static const char pmTooltip[] = "the ImGui::PanelManager layout\n(the sizes of the 4 docked windows and\nthe buttons that are selected\non the 4 toolbars)";
        if (ImGui::Button("Save Panel Manager Layout")) {
            _parent.saveToFile();
        }

        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save %s", pmTooltip);
        ImGui::SameLine();
        if (ImGui::Button("Load Panel Manager Layout")) {
            if (_parent.loadFromFile())
                _parent.setPanelManagerBoundsToIncludeMainMenuIfPresent();	// That's because we must adjust gpShowMainMenuBar state here (we have used a manual toggle button for it)
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load %s", pmTooltip);


        ImGui::Separator();
        static const char twTooltip[] = "the layout of the 5 ImGui::TabWindows\n(this option is also available by right-clicking\non an empty space in the Tab Header)";
        if (ImGui::Button("Save TabWindows Layout")) {
            _parent.saveTabsToFile();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save %s", twTooltip);
        ImGui::SameLine();
        if (ImGui::Button("Load TabWindows Layout")) {
            _parent.loadTabsFromFile();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load %s", twTooltip);

        //ImGui::Spacing();
        //if (ImGui::Button("Reset Central Window Tabs")) ResetTabWindow(tabWindow);
    }
};