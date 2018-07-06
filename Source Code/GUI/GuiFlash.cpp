#include "Headers/GuiFlash.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {

GUIFlash::GUIFlash(CEGUI::Window* parent)
        : GUIElement(parent, GUIType::GUI_FLASH, vec2<I32>(0, 0))
{
}

GUIFlash::~GUIFlash()
{
}

void GUIFlash::draw() const {
}

};