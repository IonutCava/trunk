#include "stdafx.h"

#include "Headers/ApplicationOutputWindow.h"

namespace Divide {
    ApplicationOutputWindow::ApplicationOutputWindow(PanelManager& parent)
        : DockedWindow(parent, "Application Output")
    {

    }

    ApplicationOutputWindow::~ApplicationOutputWindow()
    {

    }

    void ApplicationOutputWindow::draw() {
        // Draw Application Window
        ImGui::Text("%s\n", _name.c_str());
    }
};