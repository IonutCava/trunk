/*“Copyright 2009-2011 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _RENDER_QUEUE_H_
#define _RENDER_QUEUE_H_

#include "resource.h"
class SceneGraphNode;

struct RenderQueueItem{

	SceneGraphNode  *_node;
	I32         _sortKey;

	RenderQueueItem() : _node(NULL){}
	RenderQueueItem(I32 sortKey, SceneGraphNode *node ) : _node( node ), _sortKey( sortKey ) {}
};

struct RenderingOrder{
	enum List{
		NONE = 0,
		FRONT_TO_BACK = 1,
		BACK_TO_FRONT = 2,
		BY_STATE = 3
	};
};

struct RenderingPriority{
	enum Priority{
		FIRST = 0,
		SECOND = 1,
		ANY = 2,
		BEFORE_LAST = 3,
		LAST = 4
	};
};
DEFINE_SINGLETON( RenderQueue )
	typedef std::vector< RenderQueueItem > RenderQueueStack;
public:
	void sort();
	void refresh();
	void addNodeToQueue(SceneGraphNode* const node);
	U32  getRenderQueueStackSize();
	I32  setPriority(RenderingPriority::Priority priority);
	const RenderQueueItem& RenderQueue::getItem(I32 index);

private:
	RenderQueue() {
		_renderQueue.reserve(750);
		_order = RenderingOrder::BACK_TO_FRONT;
		_lowestKey = 0;
		_highestKey = 0;
	}
	boost::mutex _renderQueueMutex; 
	RenderQueueStack _renderQueue;
	RenderingOrder::List _order;
	I32 _lowestKey;
	I32 _highestKey;
END_SINGLETON


#endif