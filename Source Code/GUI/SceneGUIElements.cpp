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
    GFX::DrawTextCommand drawTextCommand;
    drawTextCommand._batch;

    const GUIMap& map = _guiElements[to_base(GUIType::GUI_TEXT)];
    drawTextCommand._batch._data.reserve(map.size());

    for (const GUIMap::value_type& guiStackIterator : _guiElements[to_base(GUIType::GUI_TEXT)]) {
        GUIText& textElement = static_cast<GUIText&>(*guiStackIterator.second.first);
        if (textElement.visible() && !textElement.text().empty()) {
            drawTextCommand._batch._data.emplace_back(textElement);
        }
    }

    if (!drawTextCommand._batch().empty()) {
        Attorney::GFXDeviceGUI::drawText(context, drawTextCommand, bufferInOut);
    }
}

void SceneGUIElements::onEnable() {
    for (U8 i = 0; i < to_base(GUIType::COUNT); ++i) {
        for (const GUIMap::value_type& guiStackIterator : _guiElements[i]) {
            guiStackIterator.second.first->visible(guiStackIterator.second.second);
        }
    }
}

void SceneGUIElements::onDisable() {
    for (U8 i = 0; i < to_base(GUIType::COUNT); ++i) {
        for (const GUIMap::value_type& guiStackIterator : _guiElements[i]) {
            guiStackIterator.second.first->visible(false);
        }
    }
}

}; // namespace Divide