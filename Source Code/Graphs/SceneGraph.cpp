#include "Headers/SceneGraph.h"
#include "Headers/RenderQueue.h"
#include "Hardware/Video/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"
#include "Utility/Headers/Event.h"

SceneGraph::SceneGraph() : _scene(NULL){
	_root = New SceneGraphNode(New SceneRoot);
	_updateRunning = false;
}

void SceneGraph::update(){
	RenderQueue::getInstance().refresh();
	boost::unique_lock< boost::mutex > lock_access_here(_rootAccessMutex);
	_root->updateTransformsAndBounds();
	_root->updateVisualInformation();
}

//Update, cull and draw
void SceneGraph::render(){
	GFXDevice& gfx = GFXDevice::getInstance();

	if(!_scene){
		_scene = SceneManager::getInstance().getActiveScene();
	}
	ParamHandler& par = ParamHandler::getInstance();

	if(!_updateRunning){
		startUpdateThread();
	}

	gfx.processRenderQueue();
}

void SceneGraph::print(){
	Console::getInstance().printfn("SceneGraph: ");
	Console::getInstance().toggleTimeStamps(false);
	boost::unique_lock< boost::mutex > lock_access_here(_rootAccessMutex);
	_root->print();
	Console::getInstance().toggleTimeStamps(true);
}

void SceneGraph::startUpdateThread(){
	//New Event(1,true,false,boost::bind(&SceneGraph::update,this));
	//_updateRunning = true;
	update();
}