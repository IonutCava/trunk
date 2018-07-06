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
    typedef std::array<RenderBin*, RenderBinType::_size_constant> RenderBinArray;
    typedef std::array<vectorEASTL<SceneGraphNode*>, RenderBinType::_size_constant> SortedQueues;

  public:
    RenderQueue(Kernel& parent);
    ~RenderQueue();

    void populateRenderQueues(RenderStagePass stagePass, RenderBinType rbType, vectorEASTL<RenderPackage*>& queueInOut);
    void postRender(const SceneRenderState& renderState, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut);
    void sort(RenderStagePass stagePass);
    void refresh(RenderStage stage);
    void addNodeToQueue(const SceneGraphNode& sgn, RenderStagePass stage, const vec3<F32>& eyePos);
    U16 getRenderQueueStackSize(RenderStage stage) const;

    inline RenderBin* getBin(RenderBinType rbType) {
        return _renderBins[rbType._to_integral()];
    }

    inline RenderBin* getBin(U16 renderBin) {
        return _renderBins[renderBin];
    }

    inline RenderBinArray& getBins() {
        return _renderBins;
    }

    SortedQueues getSortedQueues(RenderStage stage) const;

  private:
    RenderingOrder::List getSortOrder(RenderStagePass stagePass, RenderBinType rbType);

    RenderBin* getBinForNode(const SceneNode_ptr& nodeType, const Material_ptr& matInstance);

    RenderBin* getOrCreateBin(RenderBinType rbType);

  private:
    RenderBinArray _renderBins;
    vector<RenderBin*> _activeBins;
};

};  // namespace Divide

#endif
