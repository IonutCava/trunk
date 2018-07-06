#include "stdafx.h"

#include "Headers/PropertyWindow.h"

namespace Divide {
    PropertyWindow::PropertyWindow(PanelManager& parent)
        : DockedWindow(parent, "Properties")
    {

    }

    PropertyWindow::~PropertyWindow()
    {

    }

    void PropertyWindow::draw() {
        ImGui::Text("%s\n",_name.c_str());
#ifdef TEST_ICONS_INSIDE_TTF
        ImGui::Spacing();ImGui::Separator();
        ImGui::TextDisabled("Testing icons inside FontAwesome");
        ImGui::Separator();ImGui::Spacing();

        ImGui::Button(ICON_FA_FILE "  File"); // use string literal concatenation, ouputs a file icon and File
#   ifndef NO_IMGUIFILESYSTEM // Testing icons inside ImGuiFs::Dialog
        ImGui::AlignFirstTextHeightToWidgets();ImGui::Text("File:");ImGui::SameLine();
        static ImGuiFs::Dialog dlg;
        ImGui::InputText("###fsdlg", (char*)dlg.getChosenPath(), ImGuiFs::MAX_PATH_BYTES, ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        const bool browseButtonPressed = ImGui::Button("...##fsdlg");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "file chooser dialog\nwith FontAwesome icons");
        dlg.chooseFileDialog(browseButtonPressed, dlg.getLastDirectory());
#   endif //NO_IMGUIFILESYSTEM
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
#endif //TEST_ICONS_INSIDE_TTF
    }
};