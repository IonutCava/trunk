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

bool GUIEditorAIInterface::init() {
	return true;
}

bool GUIEditorAIInterface::tick(U32 deltaMsTime){
	bool state = true;
	if(_createNavMeshQueued){
		Navigation::NavigationMesh* temp = New Navigation::NavigationMesh();
		temp->setFileName(GET_ACTIVE_SCENE()->getName());
		if(!temp->load(NULL)){ //<Start from root for now
			temp->build(NULL,false);
		}
		temp->save();
		state = AIManager::getInstance().addNavMesh(temp);
		_createNavMeshQueued = false;
	}
	return state;
}

bool GUIEditorAIInterface::Handle_CreateNavMesh(const CEGUI::EventArgs &e){
	GUI::getInstance().getConsole()->setVisible(true);
	_createNavMeshQueued = true;
	return true;
}