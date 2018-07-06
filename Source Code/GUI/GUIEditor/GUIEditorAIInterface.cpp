#include <CEGUI/CEGUI.h>
#include "Headers/GUIEditorAIInterface.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIConsole.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h" ///< For NavMesh creation

GUIEditorAIInterface::GUIEditorAIInterface() : GUIEditorInterface(), _createNavMeshQueued(false)
{

}

GUIEditorAIInterface::~GUIEditorAIInterface()
{
}

bool GUIEditorAIInterface::init(CEGUI::Window *parent) {
	_debugDrawCheckbox = static_cast<CEGUI::ToggleButton*>(parent->getChild("AIEditor/DebugDraw"));
	_debugDrawCheckbox->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged,
									   CEGUI::Event::Subscriber(&GUIEditorAIInterface::Handle_ToggleDebugDraw,&GUIEditorAIInterface::getInstance()));
	return GUIEditorInterface::init(parent);
}

bool GUIEditorAIInterface::tick(U32 deltaMsTime){
	bool state = true;
	if(_createNavMeshQueued){
		AIManager::getInstance().toggleNavMeshDebugDraw(_debugDrawCheckbox->isSelected());
		Navigation::NavigationMesh* temp = New Navigation::NavigationMesh();
		temp->setFileName(GET_ACTIVE_SCENE()->getName());
		bool loaded = temp->load(NULL);//<Start from root for now

		if(!loaded){ 
			loaded = temp->build(NULL,false);
			temp->save();
		}

		if(loaded){
			state = AIManager::getInstance().addNavMesh(temp);
		}

		state = loaded;

		_createNavMeshQueued = false;
	}
	return state;
}

bool GUIEditorAIInterface::Handle_CreateNavMesh(const CEGUI::EventArgs &e){
	GUI::getInstance().getConsole()->setVisible(true);
	_createNavMeshQueued = true;
	return true;
}

bool GUIEditorAIInterface::Handle_ToggleDebugDraw(const CEGUI::EventArgs &e){
	AIManager::getInstance().toggleNavMeshDebugDraw(_debugDrawCheckbox->isSelected());
	return true;
}