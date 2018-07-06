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

#ifndef _MANAGERS_RENDER_PASS_MANAGER_H_
#define _MANAGERS_RENDER_PASS_MANAGER_H_

#include "Core/Headers/NonCopyable.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"

namespace Divide {

class SceneGraph;
class RenderQueue;
class SceneRenderState;
enum class RenderStage : U32;
DEFINE_SINGLETON(RenderPassManager)

  public:
    /// Call every renderqueue's render function in order
    void render(SceneRenderState& sceneRenderState, bool anaglyph = false);
    /// Add a new pass that will run once for each of the RenderStages specified
    RenderPass& addRenderPass(const stringImpl& renderPassName, 
                              U8 orderKey,
                              std::initializer_list<RenderStage> renderStages);
    /// Find a renderpass by name and remove it from the manager
    void removeRenderPass(const stringImpl& name);
    U16  getLastTotalBinSize(RenderStage displayStage) const;

    inline RenderQueue& getQueue() {
        return *_renderQueue.get();
    }

  private:
    RenderPassManager();
    ~RenderPassManager();

  private:
    // Some vector implementations are not move-awarem so use STL in this case
    vectorImpl<RenderPass> _renderPasses;
    std::unique_ptr<RenderQueue> _renderQueue;

END_SINGLETON

};  // namespace Divide

#endif
