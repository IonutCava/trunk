#include "Headers/SceneGUIElements.h"

#include "Headers/GUI.h"
#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"

#ifndef CEGUI_STATIC
#define CEGUI_STATIC
#include <CEGUI/CEGUI.h>
#endif //CEGUI_STATIC

namespace Divide {

SceneGUIElements::SceneGUIElements(Scene& parentScene)
    : GUIInterface(GUI::instance().getDisplayResolution()),
      SceneComponent(parentScene)
{
}

SceneGUIElements::~SceneGUIElements()
{
}

void SceneGUIElements::draw() {
    for (const GUIMap::value_type& guiStackIterator : _guiElements) {
        GUIElement& element = *guiStackIterator.second.first;
        // Skip hidden elements
        if (element.isVisible()) {
            element.draw();
        }
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