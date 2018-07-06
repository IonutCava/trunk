#include "stdafx.h"

#include "Headers/GUIFlash.h"

#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

GUIFlash::GUIFlash(U64 guiID, const stringImpl& name, CEGUI::Window* parent)
        : GUIElement(guiID, name, parent, GUIType::GUI_FLASH)
{
}

GUIFlash::~GUIFlash()
{
}

};
