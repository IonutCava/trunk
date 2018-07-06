#include "stdafx.h"
#include "Headers/TabbedWindow.h"

#include "Platform/File/Headers/FileManagement.h"

namespace Divide {

stringImpl TabbedWindow::s_savePath;

TabbedWindow::TabbedWindow(const stringImpl& name)
    : _name(name)
{
    if (s_savePath.empty()) {
        s_savePath = Paths::g_saveLocation + Paths::Editor::g_saveLocation + Paths::Editor::g_tabLayoutFile;
    }
}

TabbedWindow::~TabbedWindow()
{

}

ImGui::TabWindow& TabbedWindow::impl() {
    return _impl;
}

const ImGui::TabWindow& TabbedWindow::impl() const {
    return _impl;
}

bool TabbedWindow::loadFromFile(ImGui::TabWindow* tabWindows, size_t count) {
    //Temp
    if (s_savePath.empty()) {
        s_savePath = Paths::g_saveLocation + Paths::Editor::g_saveLocation + Paths::Editor::g_tabLayoutFile;
    }
    return ImGui::TabWindow::Load(s_savePath.c_str(), tabWindows, (int)count);
}

bool TabbedWindow::saveToFile(const ImGui::TabWindow* tabWindows, size_t count) {
    if (s_savePath.empty()) {
        s_savePath = Paths::g_saveLocation + Paths::Editor::g_saveLocation + Paths::Editor::g_tabLayoutFile;
    }
    return ImGui::TabWindow::Save(s_savePath.c_str(), tabWindows, (int)count);
}
}; //namespace Divide
