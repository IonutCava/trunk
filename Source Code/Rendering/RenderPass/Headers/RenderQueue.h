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

#ifndef _RENDER_QUEUE_H_
#define _RENDER_QUEUE_H_

#include "RenderBin.h"
#include "Core/Headers/KernelComponent.h"

namespace Divide {

class SceneNode;
struct RenderSubPassCmd;

FWD_DECLARE_MANAGED_CLASS(Material);
FWD_DECLARE_MANAGED_CLASS(SceneNode);

/// This class manages all of the RenderBins and renders them in the correct order
class RenderQueue : public KernelComponent {
   public:
    using RenderBinArray = std::array<RenderBin*, RenderBinType::RBT_COUNT>;

  public:
    RenderQueue(Kernel& parent);
    ~RenderQueue();

    //binAndFlag: if true, populate from bin, if false, populate from everything except bin
    void populateRenderQueues(RenderStagePass stagePass, std::pair<RenderBinType, bool> binAndFlag, vectorEASTLFast<RenderPackage*>& queueInOut);

    void postRender(const SceneRenderState& renderState, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut);
    void sort(RenderStagePass stagePass, RenderBinType targetBinType = RenderBinType::RBT_COUNT, RenderingOrder renderOrder = RenderingOrder::COUNT);
    void refresh(RenderStage stage, RenderBinType targetBinType = RenderBinType::RBT_COUNT);
    void addNodeToQueue(const SceneGraphNode& sgn, RenderStagePass stage, F32 minDistToCameraSq, RenderBinType targetBinType = RenderBinType::RBT_COUNT);
    U16 getRenderQueueStackSize(RenderStage stage) const;

    inline RenderBin* getBin(RenderBinType rbType) noexcept {
        return _renderBins[rbType._to_integral()];
    }

    inline RenderBin* getBin(U16 renderBin) noexcept {
        return _renderBins[renderBin];
    }

    inline RenderBinArray& getBins() noexcept {
        return _renderBins;
    }

    void getSortedQueues(RenderStage stage, bool isPrePass, RenderBin::SortedQueues& queuesOut, U16& countOut) const;

  private:

    RenderingOrder getSortOrder(RenderStagePass stagePass, RenderBinType rbType);

    RenderBin* getBinForNode(const SceneGraphNode& nodeType, const Material_ptr& matInstance);

  private:
    RenderBinArray _renderBins;
};

};  // namespace Divide

#endif
