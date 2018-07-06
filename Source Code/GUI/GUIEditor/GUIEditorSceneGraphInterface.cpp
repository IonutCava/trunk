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

bool GUIEditorSceneGraphInterface::update(const D32 deltaTime){
	return true;
}