/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

/*The system is similar to the one used in Torque3D (RenderPassMgr /
  RenderBinManager)
  as it was used as an inspiration.
  All credit goes to GarageGames for the idea:
  - http://garagegames.com/
  - https://github.com/GarageGames/Torque3D
  */

#ifndef _RENDER_BIN_H_
#define _RENDER_BIN_H_

#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

#include <BetterEnums/include/enum.h>

namespace Divide {

struct Task;
struct RenderStagePass;

class GFXDevice;
class SceneGraphNode;
class RenderingComponent;

namespace GFX {
    class CommandBuffer;
};

struct RenderBinItem {
    RenderingComponent* _renderable;
    I32 _sortKeyA;
    I32 _sortKeyB;
    size_t _stateHash;
    F32 _distanceToCameraSq;

    RenderBinItem() : _sortKeyA(-1), _sortKeyB(-1), _stateHash(0), _distanceToCameraSq(-1.0f), _renderable(nullptr) {}
    RenderBinItem(RenderStagePass currentStage, I32 sortKeyA, I32 sortKeyB, F32 distToCamSq, RenderingComponent& renderable);
};

struct RenderingOrder {
    enum class List : U8 {
        NONE = 0,
        FRONT_TO_BACK = 1,
        BACK_TO_FRONT = 2,
        BY_STATE = 3,
        COUNT
    };
};

//Bins can hold certain node types. This is also the order in which nodes will be rendered!
BETTER_ENUM(RenderBinType, U32,
    RBT_TERRAIN = 0, //< Terrains should occupy most of the screen and be balanced fill/geometry cost
    RBT_OPAQUE,      //< Opaque geometry will be occluded by terrain but will often occlude most of the sky (e.g.: indoors)
    RBT_SKY,         //< Sky needs to be drawn after ALL opque geometry to save on fillrate
    RBT_TRANSLUCENT, //< Translucent items use a [0.0...1.0] alpha values supplied via an opacity map or the albedo's alpha channel
    RBT_DECAL,       //< Decals are drawn over everything
    RBT_IMPOSTOR,    //< Impostors should be overlayed over everything since they are a debugging tool
    RBT_COUNT);

class RenderPackage;
class SceneRenderState;
class RenderPassManager;

struct RenderStagePass;
/// This class contains a list of "RenderBinItem"'s and stores them sorted
/// depending on designation
class RenderBin {
    typedef vector<RenderBinItem> RenderBinStack;

   public:

    friend class RenderQueue;

    explicit RenderBin(RenderBinType rbType);
    ~RenderBin();

    void sort(RenderStagePass stagePass, RenderingOrder::List renderOrder);
    void sort(RenderStagePass stagePass, RenderingOrder::List renderOrder, const Task& parentTask);
    void populateRenderQueue(RenderStagePass stagePass, vectorEASTL<RenderPackage*>& queueInOut) const;
    void postRender(const SceneRenderState& renderState, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut);
    void refresh(RenderStagePass stagePass);

    void addNodeToBin(const SceneGraphNode& sgn,
                      RenderStagePass renderStagePass,
                      const vec3<F32>& eyePos);

    const RenderBinItem& getItem(RenderStagePass stagePass, U16 index) const;

    void getSortedNodes(RenderStagePass stagePass, vectorEASTL<SceneGraphNode*>& nodes) const;

    U16 getBinSize(RenderStagePass stagePass) const;

    bool empty(RenderStagePass stagePass) const;

    inline RenderBinType getType() const { return _rbType; }

   private:
    RenderBinType _rbType;

    std::array<RenderBinStack, to_base(RenderStage::COUNT)> _renderBinStack;
};

};  // namespace Divide
#endif