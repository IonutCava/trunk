#include "Headers/GUIEditorSceneGraphInterface.h"

GUIEditorSceneGraphInterface::GUIEditorSceneGraphInterface() : GUIEditorInterface()
{
}

GUIEditorSceneGraphInterface::~GUIEditorSceneGraphInterface()
{
}

bool GUIEditorSceneGraphInterface::init(CEGUI::Window *parent) {
	return GUIEditorInterface::init(parent);
}

bool GUIEditorSceneGraphInterface::tick(U32 deltaMsTime){
	return true;
}