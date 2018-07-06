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

namespace Divide {

class SceneNode;
struct RenderSubPassCmd;

FWD_DECLARE_MANAGED_CLASS(Material);
FWD_DECLARE_MANAGED_CLASS(SceneNode);

/// This class manages all of the RenderBins and renders them in the correct order
class RenderQueue {
   public:
    typedef std::array<RenderBin*, RenderBinType::COUNT> RenderBinArray;
    typedef std::array<vector<SceneGraphNode*>, to_base(RenderBinType::COUNT)> SortedQueues;

  public:
    RenderQueue(GFXDevice& context);
    ~RenderQueue();

    void populateRenderQueues(const RenderStagePass& renderStagePass);
    void postRender(const SceneRenderState& renderState, const RenderStagePass& renderStagePass, GFX::CommandBuffer& bufferInOut);
    void sort();
    void refresh();
    void addNodeToQueue(const SceneGraphNode& sgn, const RenderStagePass& stage, const vec3<F32>& eyePos);
    U16 getRenderQueueStackSize() const;

    inline RenderBin* getBin(RenderBinType rbType) {
        return _renderBins[rbType._to_integral()];
    }

    inline RenderBin* getBin(U16 renderBin) {
        return _renderBins[renderBin];
    }

    inline RenderBinArray& getBins() {
        return _renderBins;
    }

    const SortedQueues& getSortedQueues();

  private:
    RenderingOrder::List getSortOrder(RenderBinType rbType);

    RenderBin* getBinForNode(const SceneNode_ptr& nodeType, const Material_ptr& matInstance);

    RenderBin* getOrCreateBin(RenderBinType rbType);

  private:
    GFXDevice& _context;
    RenderBinArray _renderBins;
    SortedQueues   _sortedQueues;
    vector<RenderBin*> _activeBins;
};

};  // namespace Divide

#endif
