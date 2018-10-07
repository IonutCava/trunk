#include "stdafx.h"

#include "Headers/DockedWindow.h"

namespace Divide {

    DockedWindow::DockedWindow(Editor& parent, const Descriptor& descriptor)
        : _parent(parent),
          _descriptor(descriptor),
          _focused(false),
          _isHovered(false)
    {
    }

    DockedWindow::~DockedWindow()
    {
    }

    void DockedWindow::draw() {
        ImGuiWindowFlags windowFlags = 0;
        //window_flags |= ImGuiWindowFlags_NoTitleBar;
        //window_flags |= ImGuiWindowFlags_NoScrollbar;
        //window_flags |= ImGuiWindowFlags_MenuBar;
        //window_flags |= ImGuiWindowFlags_NoMove;
        //window_flags |= ImGuiWindowFlags_NoResize;
        //window_flags |= ImGuiWindowFlags_NoCollapse;
        //window_flags |= ImGuiWindowFlags_NoNav;

        ImGui::SetNextWindowPos(_descriptor.position, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(_descriptor.size, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(_descriptor.minSize, _descriptor.maxSize);
        if (ImGui::Begin(_descriptor.name.c_str(), NULL, windowFlags | _descriptor.flags)) {
            _focused = ImGui::IsWindowFocused();
            _isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
            drawInternal();
        }
        ImGui::End();
    }

}; //namespace Divide