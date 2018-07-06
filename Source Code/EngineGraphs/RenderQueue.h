#ifndef _RENDER_QUEUE_H_
#define _RENDER_QUEUE_H_

#include "resource.h"
class SceneGraphNode;

struct RenderQueueItem{

	SceneGraphNode  *_node;
	F32         _sortKey;

	RenderQueueItem() {}
	RenderQueueItem(F32 sortKey, SceneGraphNode *node ) : _node( node ), _sortKey( sortKey ) {}
};

struct RenderingOrder{
	enum List{
		NONE = 0,
		FRONT_TO_BACK = 1,
		BACK_TO_FRONT = 2,
		BY_STATE = 3
	};
};

DEFINE_SINGLETON( RenderQueue )
	typedef std::vector< RenderQueueItem > RenderQueueStack;
public:
	void sort();
	void refresh();
	void addNodeToQueue(SceneGraphNode* node);
	U32  getRenderQueueStackSize();
	RenderQueueItem& RenderQueue::getItem(I32 index);
private:
	RenderQueue() {
		_renderQueue.reserve(750);
		_order = RenderingOrder::BACK_TO_FRONT;
	}
	boost::mutex _renderQueueMutex; 
	RenderQueueStack _renderQueue;
	RenderingOrder::List _order;
END_SINGLETON


#endif