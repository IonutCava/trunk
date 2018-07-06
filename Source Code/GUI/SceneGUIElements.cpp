#include "Headers/SceneGUIElements.h"

#include "Headers/GUI.h"
#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"

#include "Platform/Video/Headers/GFXDevice.h"

#ifndef CEGUI_STATIC
#define CEGUI_STATIC
#include <CEGUI/CEGUI.h>
#endif //CEGUI_STATIC

namespace Divide {

SceneGUIElements::SceneGUIElements(Scene& parentScene, GUI& context)
    : GUIInterface(context, context.getDisplayResolution()),
      SceneComponent(parentScene)
{
}

SceneGUIElements::~SceneGUIElements()
{
}

void SceneGUIElements::draw(GFXDevice& context) {
    static vectorImpl<GUITextBatchEntry> textBatch;
    textBatch.resize(0);

    for (U8 i = 0; i < to_base(GUIType::COUNT); ++i) {
        if (i != to_base(GUIType::GUI_TEXT)) {
            for (const GUIMap::value_type& guiStackIterator : _guiElements[i]) {
                GUIElement& element = *guiStackIterator.second.first;
                // Skip hidden elements
                if (element.isVisible()) {
                    element.draw(context);
                }
            }
        }
    }

    for (const GUIMap::value_type& guiStackIterator : _guiElements[to_base(GUIType::GUI_TEXT)]) {
        GUIText& textElement = static_cast<GUIText&>(*guiStackIterator.second.first);
        if (!textElement.text().empty()) {
            textBatch.emplace_back(&textElement, textElement.getPosition(), textElement.getStateBlockHash());
        }
    }

    if (!textBatch.empty()) {
        Attorney::GFXDeviceGUI::drawText(context, textBatch);
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