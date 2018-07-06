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

/*The system is similar to the one used in Torque3D (RenderPassMgr / RenderBinManager) as it was used as an inspiration.
  All credit goes to GarageGames for the idea:
  - http://garagegames.com/ 
  - https://github.com/GarageGames/Torque3D
  */

#ifndef _RENDER_BIN_H_
#define _RENDER_BIN_H_
class SceneGraphNode;
#include "core.h"

struct RenderBinItem{

	SceneGraphNode  *_node;
	P32              _sortKey;
	U32              _stateHash;

	RenderBinItem() : _node(NULL){}
	RenderBinItem(P32 sortKey, SceneGraphNode *node );
};

struct RenderingOrder{
	enum List{
		NONE = 0,
		FRONT_TO_BACK = 1,
		BACK_TO_FRONT = 2,
		BY_STATE = 3,
		ORDER_PLACEHOLDER = 4
	};
};

///This class contains a list of "RenderBinItem"'s and stores them sorted depending on the desired designation

class RenderBin {
	typedef vectorImpl< RenderBinItem > RenderBinStack;
public:
	enum RenderBinType {
		RBT_MESH = 0,
		RBT_DELEGATE,
		RBT_TRANSLUCENT,
		RBT_SKY,
		RBT_WATER,
		RBT_TERRAIN,
        RBT_IMPOSTOR,
		RBT_FOLIAGE,
		RBT_PARTICLES,
		RBT_DECALS,
		RBT_SHADOWS,
		RBT_PLACEHOLDER
	};
	friend class RenderQueue;

	RenderBin(const RenderBinType& rbType = RBT_PLACEHOLDER, const RenderingOrder::List& renderOrder = RenderingOrder::ORDER_PLACEHOLDER,D32 drawKey = -1);
	virtual ~RenderBin() {}

	virtual void sort();
	virtual void render();
	virtual void refresh();
	virtual void addNodeToBin(SceneGraphNode* const sgn);
	const RenderBinItem& getItem(U16 index);
	std::string rBinTypeToString(const RenderBinType& rbt);

	inline U16 getBinSize() {return _renderBinStack.size();}
	inline D32 getSortOrder() {return _drawKey;}

	inline const RenderBinType&  getType() {return _rbType;}
private:
	mutable SharedLock _renderBinGetMutex; 
	D32 _drawKey;
	RenderBinType _rbType;
	RenderBinStack _renderBinStack;
	RenderingOrder::List _renderOrder;

};

#endif