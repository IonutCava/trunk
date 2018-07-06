#include "Headers/RenderQueue.h"
#include "Headers/SceneGraphNode.h"
#include "Rendering/Headers/Frustum.h"

struct RenderQueueKeyCompare{
	bool operator()( const RenderQueueItem &a, const RenderQueueItem &b ) const
		{ return a._sortKey.i < b._sortKey.i; }
};

struct RenderQueueDistanceBacktoFront{
	bool operator()( const RenderQueueItem &a, const RenderQueueItem &b) const {
		const vec3& eye = Frustum::getInstance().getEyePos();
		F32 dist_a = a._node->getBoundingBox().nearestDistanceFromPoint(eye);
		F32 dist_b = b._node->getBoundingBox().nearestDistanceFromPoint(eye);
		return dist_a < dist_b;
	}
};

struct RenderQueueDistanceFrontToBack{
	bool operator()( const RenderQueueItem &a, const RenderQueueItem &b) const {
		const vec3& eye = Frustum::getInstance().getEyePos();
		F32 dist_a = a._node->getBoundingBox().nearestDistanceFromPoint(eye);
		F32 dist_b = b._node->getBoundingBox().nearestDistanceFromPoint(eye);
		return dist_a > dist_b;
	}
};

void RenderQueue::sort(){
	const vec3& eye = Frustum::getInstance().getEyePos();
	std::sort( _translucentStack.begin(), _translucentStack.end(), RenderQueueDistanceBacktoFront());
	std::sort( _opaqueStack.begin(), _opaqueStack.end(), RenderQueueKeyCompare()  );
	//Render opaque items first
	for_each(RenderQueueStack::value_type& s, _opaqueStack){
		_renderQueue.push_back(s);
	}
	//Then translucent
	for_each(RenderQueueStack::value_type& s, _translucentStack){
		_renderQueue.push_back(s);
	}
}

void RenderQueue::addNodeToQueue(SceneGraphNode* const node){
	assert(node != NULL);
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	if(node->getNode()){
		P32 key;
		Material* nodeMaterial = node->getNode()->getMaterial();
		if(nodeMaterial){
			key = nodeMaterial->getMaterialId();
			if(node->getNode()->getMaterial()->isTranslucent()){
				_translucentStack.push_back(RenderQueueItem(key,node));
			}else{
				_opaqueStack.push_back(RenderQueueItem(key,node));
			}
		}else{
			key.i = _renderQueue.size()+1;
			_renderQueue.push_back(RenderQueueItem(key,node));
		}
	}
}

const RenderQueueItem& RenderQueue::getItem(I32 index){
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	return _renderQueue[index];
}
 
void RenderQueue::refresh(){
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	_renderQueue.resize(0);
	_translucentStack.resize(0);
	_opaqueStack.resize(0);
}

U32 RenderQueue::getRenderQueueStackSize(){
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	return _renderQueue.size();
}
