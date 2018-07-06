#include "stdafx.h"

#include "Headers/PanelManagerPane.h"
#include "Headers/PanelManager.h"

namespace Divide {
PanelManagerPane::PanelManagerPane(PanelManager& parent, const stringImpl& name, ImGui::PanelManager::Position position)
    : _parent(parent),
      _name(name)
{
    ImGui::PanelManager& mgr = Attorney::PanelManagerWidgets::internalManager(parent);
    _pane = mgr.addPane(position, name.c_str());
}

PanelManagerPane::~PanelManagerPane()
{

}

void PanelManagerPane::addTabWindowIfSupported(const char* tabWindowNames[4]) {
    const ImTextureID texId = ImGui::TabWindow::DockPanelIconTextureID;
    IM_ASSERT(_pane && texId);

    ImVec2 buttonSizeTemp(PanelManager::ButtonHeight, PanelManager::ButtonHeight);

    const I32 index = static_cast<int>(_pane->pos);

    if (index < 2) {
        buttonSizeTemp.x = PanelManager::ButtonWidth;
    }

    const I32 uvIndex = (index == 0) ? 3 : (index == 2) ? 0 : (index == 3) ? 2 : index;

    ImVec2 uv0(0.75f, (float)uvIndex*0.25f), uv1(uv0.x + 0.25f, uv0.y + 0.25f);

    _pane->addButtonAndWindow(ImGui::Toolbutton(tabWindowNames[index],
                                                texId,
                                                uv0,
                                                uv1,
                                                buttonSizeTemp),  // the 1st arg of Toolbutton is only used as a text for the tooltip.
                              ImGui::PanelManagerPaneAssociatedWindow(tabWindowNames[index],
                                                                      -1,
                                                                      &Attorney::PanelManagerWidgets::drawDockedTabWindows,
                                                                      &_parent,
                                                                      ImGuiWindowFlags_NoScrollbar));    //  the 1st arg of PanelManagerPaneAssociatedWindow is the name of the window
}

ImGui::PanelManager::Pane& PanelManagerPane::impl() {
    return *_pane;
}

const ImGui::PanelManager::Pane& PanelManagerPane::impl() const {
    return *_pane;
}


}; //namespace Divide