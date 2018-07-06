#include "Headers/GUIFlash.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {

GUIFlash::GUIFlash(ULL ID, CEGUI::Window* parent)
        : GUIElement(ID, parent, GUIType::GUI_FLASH)
{
}

GUIFlash::~GUIFlash()
{
}

void GUIFlash::draw() const {
}

};
