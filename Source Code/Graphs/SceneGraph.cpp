#include "Headers/SceneGraph.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

SceneGraph::SceneGraph(){
	_root = New SceneGraphNode(New SceneRoot);
	_updateRunning = false;
}

void SceneGraph::update(){
	_root->checkBoundingBoxes();
	_root->updateTransforms();
	_root->updateVisualInformation();
}

void SceneGraph::idle(){
	typedef Unordered_map<std::string, SceneGraphNode*> NodeChildren;
	for_each(SceneGraphNode* it, _pendingDeletionNodes){
		it->unload();
		it->getParent()->removeNode(it);
		SAFE_DELETE(it);
	}
}

void SceneGraph::print(){
	PRINT_FN(Locale::get("SCENEGRAPH_TITLE"));
	Console::getInstance().toggleTimeStamps(false);
	boost::unique_lock< boost::mutex > lock_access_here(_rootAccessMutex);
	_root->print();
	Console::getInstance().toggleTimeStamps(true);
}

void SceneGraph::startUpdateThread(){
}

void SceneGraph::sceneUpdate(U32 sceneTime){
	_root->sceneUpdate(sceneTime);
}

SceneGraphNode* SceneGraph::Intersect(const Ray& ray, F32 start, F32 end){
	return _root->Intersect(ray,start,end);
}