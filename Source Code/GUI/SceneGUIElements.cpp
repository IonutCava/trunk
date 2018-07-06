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
    for (const GUIMap::value_type& guiStackIterator : _guiElements) {
        GUIElement& element = *guiStackIterator.second.first;
        // Skip hidden elements
        if (element.isVisible()) {
            // Cache text elements
            if (element.getType() == GUIType::GUI_TEXT) {
                GUIText& textElement = static_cast<GUIText&>(element);
                if (!textElement.text().empty()) {
                    textBatch.emplace_back(&textElement, textElement.getPosition(), textElement.getStateBlockHash());
                }
            }
            else {
                element.draw(context);
            }
        }
    }
    if (!textBatch.empty()) {
        Attorney::GFXDeviceGUI::drawText(context, textBatch);
    }
}

void SceneGUIElements::onEnable() {
    for (const GUIMap::value_type& guiStackIterator : _guiElements) {
        guiStackIterator.second.first->setVisible(guiStackIterator.second.second);
    }
}

void SceneGUIElements::onDisable() {
    for (const GUIMap::value_type& guiStackIterator : _guiElements) {
        guiStackIterator.second.first->setVisible(false);
    }
}

}; // namespace Divide