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

#include "RenderBin.h"
#include "Core/Headers/Singleton.h"
#include "Utility/Headers/UnorderedMap.h"

class SceneNode;

///This class manages all of the RenderBins and renders them in the correct order
DEFINE_SINGLETON( RenderQueue )
	typedef Unordered_map<RenderBin::RenderBinType, RenderBin* > RenderBinMap;
	typedef Unordered_map<U16, RenderBin::RenderBinType > RenderBinIDType;

public:
	///
	void sort();
	void refresh();
	void addNodeToQueue(SceneGraphNode* const sgn);
	U16 getRenderQueueStackSize();
	inline U16 getRenderQueueBinSize() {return _sortedRenderBins.size();}
	SceneGraphNode* getItem(U16 renderBin, U16 index);
	RenderBin*      getBinSorted(U16 renderBin);
	RenderBin*      getBin(U16 renderBin);
	RenderBin*      getBin(RenderBin::RenderBinType rbType);

private:
	~RenderQueue();
	RenderBin* getBinForNode(SceneNode* const nodeType);
	RenderBin* getOrCreateBin(const RenderBin::RenderBinType& rbType);

private:
	RenderBinMap _renderBins;
	RenderBinIDType _renderBinId;
	vectorImpl<RenderBin* > _sortedRenderBins;

END_SINGLETON

#endif