#include "stdafx.h"

#include "Headers/DockedWindow.h"

namespace Divide {

    DockedWindow::DockedWindow(Editor& parent, const stringImpl& name)
        : _parent(parent),
          _focused(false),
          _isHovered(false),
          _name(name)
    {
    }

    DockedWindow::~DockedWindow()
    {
    }

    void DockedWindow::draw() {
        _focused = ImGui::IsWindowFocused();
        _isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    }

}; //namespace Divide