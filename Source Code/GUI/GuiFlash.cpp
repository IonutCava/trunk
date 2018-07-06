#include "Headers/GUIFlash.h"

#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

GUIFlash::GUIFlash(U64 guiID, CEGUI::Window* parent)
        : GUIElement(guiID, parent, GUIType::GUI_FLASH)
{
}

GUIFlash::~GUIFlash()
{
}

void GUIFlash::draw(GFXDevice& context) const {
    ACKNOWLEDGE_UNUSED(context);
}

};
