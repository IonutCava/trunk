/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

/*The system is similar to the one used in Torque3D (RenderPassMgr / RenderBinManager) as it was used as an inspiration.
  All credit goes to GarageGames for the idea:
  - http://garagegames.com/
  - https://github.com/GarageGames/Torque3D
  */

#ifndef _RENDER_BIN_H_
#define _RENDER_BIN_H_

#include "Utility/Headers/Vector.h"
#include "Core/Math/Headers/MathClasses.h"

namespace Divide {
enum  RenderStage;
class SceneGraphNode;

struct RenderBinItem{
    SceneGraphNode  *_node;
    I32              _sortKeyA; 
    I32              _sortKeyB;
    size_t           _stateHash;
    F32              _distanceToCameraSq;

    RenderBinItem() : _node(nullptr){}
    RenderBinItem(I32 sortKeyA, I32 sortKeyB, F32 distToCamSq, SceneGraphNode *node);
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

class SceneRenderState;
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
        RBT_PARTICLES,
        RBT_VEGETATION_GRASS,
        RBT_VEGETATION_TREES,
        RBT_DECALS,
        RBT_SHADOWS,
        RBT_PLACEHOLDER
    };

    std::string renderBinTypeToNameMap[RBT_PLACEHOLDER+1];

    friend class RenderQueue;

    RenderBin(const RenderBinType& rbType = RBT_PLACEHOLDER,
              const RenderingOrder::List& renderOrder = RenderingOrder::ORDER_PLACEHOLDER,
              D32 drawKey = -1);

    virtual ~RenderBin()
    {
    }

    virtual void sort(const RenderStage& currentRenderStage);
    virtual void preRender(const RenderStage& currentRenderStage);
    virtual void render(const SceneRenderState& renderState, const RenderStage& currentRenderStage);
    virtual void postRender(const RenderStage& currentRenderStage);
    virtual void refresh();

    virtual void addNodeToBin(SceneGraphNode* const sgn, const vec3<F32>& eyePos);
    inline  const RenderBinItem& getItem(U16 index) const {assert(index < _renderBinStack.size());	return _renderBinStack[index]; }

    inline U16 getBinSize()   const {return (U16)_renderBinStack.size();}
    inline D32 getSortOrder() const {return _drawKey;}

    inline const RenderBinType&  getType() const {return _rbType;}

    bool operator< (const RenderBin& rhs){
        return (_drawKey < rhs._drawKey); 
    };

private:
    //mutable SharedLock _renderBinGetMutex;
    D32 _drawKey;
    RenderBinType _rbType;
    RenderBinStack _renderBinStack;
    RenderingOrder::List _renderOrder;
};

}; //namespace Divide
#endif