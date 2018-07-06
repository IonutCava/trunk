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

bool GUIEditorLightInterface::tick(U32 deltaMsTime){
	return true;
}