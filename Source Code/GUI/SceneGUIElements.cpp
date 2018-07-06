#include "stdafx.h"

#include "Headers/SceneGUIElements.h"

#include "Headers/GUI.h"
#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"

#include "Platform/Video/Headers/GFXDevice.h"

#include <CEGUI/CEGUI.h>

namespace Divide {

SceneGUIElements::SceneGUIElements(Scene& parentScene, GUI& context)
    : GUIInterface(context),
      SceneComponent(parentScene)
{
}

SceneGUIElements::~SceneGUIElements()
{
}

void SceneGUIElements::draw(GFXDevice& context, GFX::CommandBuffer& bufferInOut) {
    TextElementBatch batch;
    for (const GUIMap::value_type& guiStackIterator : _guiElements[to_base(GUIType::GUI_TEXT)]) {
        GUIText& textElement = static_cast<GUIText&>(*guiStackIterator.second.first);
        if (textElement.isVisible() && !textElement.text().empty()) {
            batch._data.emplace_back(textElement);
        }
    }

    if (!batch().empty()) {
        Attorney::GFXDeviceGUI::drawText(context, batch, bufferInOut);
    }
}

void SceneGUIElements::onEnable() {
    for (U8 i = 0; i < to_base(GUIType::COUNT); ++i) {
        for (const GUIMap::value_type& guiStackIterator : _guiElements[i]) {
            guiStackIterator.second.first->setVisible(guiStackIterator.second.second);
        }
    }
}

void SceneGUIElements::onDisable() {
    for (U8 i = 0; i < to_base(GUIType::COUNT); ++i) {
        for (const GUIMap::value_type& guiStackIterator : _guiElements[i]) {
            guiStackIterator.second.first->setVisible(false);
        }
    }
}

}; // namespace Divide