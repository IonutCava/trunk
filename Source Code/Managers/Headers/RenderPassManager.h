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

#ifndef _MANAGERS_RENDER_PASS_MANAGER_H_
#define _MANAGERS_RENDER_PASS_MANAGER_H_

#include "Core/Headers/KernelComponent.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Threading/Headers/Task.h"

namespace Divide {

class Camera;
class SceneGraph;
class RenderTarget;
class RTDrawDescriptor;
enum class RenderStage : U32;

class RenderPassManager : public KernelComponent {
public:
    struct PassParams {
        PassParams();

        RenderTargetID target;
        RTDrawDescriptor* drawPolicy = nullptr;
        RenderStage stage = RenderStage::COUNT;
        Camera* camera = nullptr;
        bool occlusionCull = false;
        bool doPrePass = true;
        bool bindTargets = true;
        U32 pass = 0;
        FrustumClipPlanes clippingPlanes;
    };
public:
    explicit RenderPassManager(Kernel& parent, GFXDevice& context);
    ~RenderPassManager();

    bool init();
    void destroy();

    /// Call every renderqueue's render function in order
    void render(SceneRenderState& sceneRenderState);
    /// Add a new pass that will run once for each of the RenderStages specified
    RenderPass& addRenderPass(const stringImpl& renderPassName,
                              U8 orderKey,
                              RenderStage renderStage);
    /// Find a renderpass by name and remove it from the manager
    void removeRenderPass(const stringImpl& name);
    U16  getLastTotalBinSize(RenderStage renderStage) const;

    inline RenderQueue& getQueue() {
        return *_renderQueue;
    }

    RenderPass::BufferData& getBufferData(RenderStage renderStage, I32 bufferIndex);
    const RenderPass::BufferData& getBufferData(RenderStage renderStage, I32 bufferIndex) const;

    void doCustomPass(PassParams& params, GFX::CommandBuffer& bufferInOut);

private:

    void prePass(const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut);
    void mainPass(const PassParams& params, RenderTarget& target, GFX::CommandBuffer& bufferInOut);
    void woitPass(const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut);

    RenderPass* getPassForStage(RenderStage renderStage) const;

private:
    GFXDevice& _context;
    // Some vector implementations are not move-aware so use STL in this case
    vectorImpl<RenderPass*> _renderPasses;
    RenderQueue* _renderQueue;
    ShaderProgram_ptr _OITCompositionShader;
};

};  // namespace Divide

#endif
