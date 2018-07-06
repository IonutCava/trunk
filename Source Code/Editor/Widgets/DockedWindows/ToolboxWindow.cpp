#include "stdafx.h"

#include "Headers/ToolboxWindow.h"

namespace Divide {
    ToolboxWindow::ToolboxWindow(PanelManager& parent)
        : DockedWindow(parent, "Toolbox")
    {

    }

    ToolboxWindow::~ToolboxWindow()
    {

    }

    void ToolboxWindow::draw() {
        // Draw Toolbox
        ImGui::Text("%s\n", _name.c_str());
    }
};