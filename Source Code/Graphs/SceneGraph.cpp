#include "Headers/SceneGraph.h"
#include "Hardware/Video/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"
#include "Utility/Headers/Event.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

SceneGraph::SceneGraph() : _scene(NULL){
	_root = New SceneGraphNode(New SceneRoot);
	_updateRunning = false;
}

void SceneGraph::update(){
	_root->checkBoundingBoxes();
	_root->updateTransforms();
	_root->updateVisualInformation();
}

void SceneGraph::idle(){
	typedef unordered_map<std::string, SceneGraphNode*> NodeChildren;
	for_each(SceneGraphNode* it, _pendingDeletionNodes){
		it->unload();
		it->getParent()->removeNode(it);
		SAFE_DELETE(it);
	}
}

//Update, cull and draw
void SceneGraph::render(){

	if(!_scene){
		_scene = GET_ACTIVE_SCENE();
	}
	ParamHandler& par = ParamHandler::getInstance();
	update(); ///< Call the very first update and the last one to take place before "processRenderQueue"
	///Render current frame
	GFX_DEVICE.processRenderQueue();

	///Update visual information while GPU is bussy
	//update();
}

void SceneGraph::print(){
	PRINT_FN("SceneGraph: ");
	Console::getInstance().toggleTimeStamps(false);
	boost::unique_lock< boost::mutex > lock_access_here(_rootAccessMutex);
	_root->print();
	Console::getInstance().toggleTimeStamps(true);
}

void SceneGraph::startUpdateThread(){
}

void SceneGraph::sceneUpdate(D32 sceneTime){
	_root->sceneUpdate(sceneTime);
}

SceneGraphNode* SceneGraph::Intersect(const Ray& ray, F32 start, F32 end){
	return _root->Intersect(ray,start,end);
}