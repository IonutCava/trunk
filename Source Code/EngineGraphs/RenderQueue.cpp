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

void RenderQueue::addNodeToQueue(SceneGraphNode* const node){
		
	I32 key = node->getNode()->_sortKey;
	if(key > _highestKey) _highestKey = key;
	if(key < _lowestKey) _lowestKey = key;

	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	_renderQueue.push_back(RenderQueueItem(key,node));
}

const RenderQueueItem& RenderQueue::getItem(I32 index){
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

I32 RenderQueue::setPriority(RenderingPriority::Priority priority){
	F32 key = -1;
	switch(priority){
		case RenderingPriority::FIRST:
			key = _lowestKey - 2;
			break;
		case RenderingPriority::SECOND:
			key = _lowestKey - 1;
			break;
		case RenderingPriority::BEFORE_LAST:
			key = _highestKey + 1;
			break;
		case RenderingPriority::LAST:
			key = _highestKey + 2;
			break;
		default:
		case RenderingPriority::ANY:
			I32 key = (I32)random(_lowestKey+1,_highestKey-1);
			break;
	};
	return key;
}