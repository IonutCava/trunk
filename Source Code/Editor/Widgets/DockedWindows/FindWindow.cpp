#include "stdafx.h"

#include "Headers/FindWindow.h"

namespace Divide {
    EditorFindWindow::EditorFindWindow(PanelManager& parent)
        : DockedWindow(parent, "Application Output")
    {

    }

    EditorFindWindow::~EditorFindWindow()
    {

    }

    void EditorFindWindow::draw() {
        // Draw Find Window
        ImGui::Text("%s\n", _name.c_str());
    }
};