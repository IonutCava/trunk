/*
   Copyright (c) 2016 DIVIDE-Studio
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

class Material;
class SceneNode;

/// This class manages all of the RenderBins and renders them in the correct order
class RenderQueue {
    typedef std::array<RenderBin*, RenderBinType::COUNT> RenderBinArray;

  public:
    RenderQueue();
    ~RenderQueue();

    void populateRenderQueues(RenderStage renderStage);
    void postRender(SceneRenderState& renderState, RenderStage renderStage);
    void sort(RenderStage renderStage);
    void refresh();
    void addNodeToQueue(const SceneGraphNode& sgn, RenderStage stage, const vec3<F32>& eyePos);
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

  private:
    RenderBin* getBinForNode(const std::shared_ptr<SceneNode>& nodeType,
                             const std::shared_ptr<Material>& matInstance);

    RenderBin* getOrCreateBin(RenderBinType rbType);

  private:
    RenderBinArray _renderBins;
    vectorImpl<RenderBin*> _activeBins;
};

};  // namespace Divide

#endif
