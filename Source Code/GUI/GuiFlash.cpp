#include "Headers/GuiFlash.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {

GUIFlash::GUIFlash(CEGUI::Window* parent)
        : GUIElement(parent, GUIType::GUI_FLASH)
{
}

GUIFlash::~GUIFlash()
{
}

void GUIFlash::draw() const {
}

};