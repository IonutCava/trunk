#include "RenderQueue.h"
#include "SceneGraphNode.h"

struct RenderQueueKeyCompare{
	bool operator()( const RenderQueueItem &a, const RenderQueueItem &b ) const
		{ return a._sortKey < b._sortKey; }
};


void RenderQueue::sort(){
	switch(_order){
		case RenderingOrder::NONE:
			return;
		case RenderingOrder::BACK_TO_FRONT:
		case RenderingOrder::FRONT_TO_BACK:
		case RenderingOrder::BY_STATE:
		default:
			std::sort( _renderQueue.begin(), _renderQueue.end(), RenderQueueKeyCompare() );
			break;
	}
}

void RenderQueue::addNodeToQueue(SceneGraphNode* node){
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	_renderQueue.push_back(RenderQueueItem(node->getNode()->_sortKey,node));
}

RenderQueueItem& RenderQueue::getItem(I32 index) {
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	return _renderQueue[index];
}
 
void RenderQueue::refresh(){
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	_renderQueue.resize(0);
}

U32 RenderQueue::getRenderQueueStackSize(){
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	return _renderQueue.size();
}