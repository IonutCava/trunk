#include "Headers/GUIEditorLightInterface.h"

GUIEditorLightInterface::GUIEditorLightInterface() : GUIEditorInterface()
{
}

GUIEditorLightInterface::~GUIEditorLightInterface()
{
}

bool GUIEditorLightInterface::init(CEGUI::Window *parent) {
	return GUIEditorInterface::init(parent);
}

bool GUIEditorLightInterface::update(const D32 deltaTime){
	return true;
}