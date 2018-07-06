/*“Copyright 2009-2013 DIVIDE-Studio”*/
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

#include "core.h"
class SceneGraphNode;

struct RenderQueueItem{

	SceneGraphNode  *_node;
	P32              _sortKey;
	U32              _stateHash;

	RenderQueueItem() : _node(NULL){}
	RenderQueueItem(P32 sortKey, SceneGraphNode *node );
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
	void addNodeToQueue(SceneGraphNode* const sgn);
	U32  getRenderQueueStackSize();
	const RenderQueueItem& RenderQueue::getItem(I32 index);

private:
	RenderQueue() {
		_renderQueue.reserve(250);
		_order = RenderingOrder::BACK_TO_FRONT;
		_lowestKey = 0;
		_highestKey = 0;
	}
	
	mutable Lock _renderQueueGetMutex; 

	RenderQueueStack _opaqueStack;
	RenderQueueStack _translucentStack;
	RenderQueueStack _renderQueue;
	RenderingOrder::List _order;
	I32 _lowestKey;
	I32 _highestKey;
END_SINGLETON


#endif