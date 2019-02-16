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

#pragma once
#ifndef _MANAGERS_RENDER_PASS_MANAGER_H_
#define _MANAGERS_RENDER_PASS_MANAGER_H_

#include "Core/Headers/KernelComponent.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderPackage.h"

namespace Divide {

class Camera;
class SceneGraph;
class RenderTarget;
class RTDrawDescriptor;
enum class RenderStage : U8;

class RenderPassManager : public KernelComponent {
public:
    struct PassParams {
        // source node is used to determine if the current pass is triggered by a specific node:
        // e.g. a light node for shadow mapping, a reflector for reflection (or refraction), etc
        // safe to be set to null
        const SceneGraphNode* _sourceNode = nullptr;
        Camera* _camera = nullptr;
        const RTDrawDescriptor* _drawPolicy = nullptr;
        FrustumClipPlanes _clippingPlanes = {};
        RenderTargetID _target = {};
        I32 _minLoD = -1; //-1 = all
        union {
            U32 _passIndex = 0;
            struct {
                U16 _indexA;
                U16 _indexB;
            };
        };
        RenderStage _stage = RenderStage::COUNT;
        RenderPassType _pass = RenderPassType::COUNT;
        U8  _passVariant = 0;
        bool _occlusionCull = false;
        bool _bindTargets = true;
    };

public:
    explicit RenderPassManager(Kernel& parent, GFXDevice& context);
    ~RenderPassManager();

    /// Call every renderqueue's render function in order
    void render(SceneRenderState& sceneRenderState, Time::ProfileTimer* parentTimer = nullptr);
    /// Add a new pass that will run once for each of the RenderStages specified
    RenderPass& addRenderPass(const stringImpl& renderPassName,
                              U8 orderKey,
                              RenderStage renderStage);
    /// Find a renderpass by name and remove it from the manager
    void removeRenderPass(const stringImpl& name);
    U16  getLastTotalBinSize(RenderStage renderStage) const;

    inline RenderQueue& getQueue() { return _renderQueue; }

    RenderPass::BufferData getBufferData(RenderStagePass stagePass) const;

    void doCustomPass(PassParams& params, GFX::CommandBuffer& bufferInOut);

private:
    // Returns false if we skipped the pre-pass step
    bool prePass(const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut);
    void mainPass(const PassParams& params, RenderTarget& target, GFX::CommandBuffer& bufferInOut, bool prePassExecuted);
    void woitPass(const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut);

    RenderPass& getPassForStage(RenderStage renderStage);
    const RenderPass& getPassForStage(RenderStage renderStage) const;
    void prepareRenderQueues(RenderStagePass stagePass, const PassParams& params, bool refreshNodeData, GFX::CommandBuffer& bufferInOut);
    void buildDrawCommands(RenderStagePass stagePass, const PassParams& params, bool refreshNodeData, GFX::CommandBuffer& bufferInOut);
    void refreshNodeData(RenderStagePass stagePass, const SceneRenderState& renderState, const mat4<F32>& viewMatrix, const RenderQueue::SortedQueues& sortedQueues, GFX::CommandBuffer& bufferInOut);
    GFXDevice::NodeData processVisibleNode(SceneGraphNode* node, RenderStagePass stagePass, bool isOcclusionCullable, bool playAnimations, const mat4<F32>& viewMatrix) const;

private: //TEMP
    friend class RenderBin;
    U32  renderQueueSize(RenderStagePass stagePass, RenderPackage::MinQuality qualityRequirement = RenderPackage::MinQuality::COUNT) const;
    void renderQueueToSubPasses(RenderStagePass stagePass, GFX::CommandBuffer& commandsInOut, RenderPackage::MinQuality qualityRequirement = RenderPackage::MinQuality::COUNT) const;

private:
    GFXDevice& _context;
    RenderQueue _renderQueue;

    vectorEASTL<std::shared_ptr<RenderPass>> _renderPasses;
    vectorEASTL<GFX::CommandBuffer*> _renderPassCommandBuffer;
    GFX::CommandBuffer* _mainCommandBuffer;

    ShaderProgram_ptr _OITCompositionShader;
    Time::ProfileTimer* _renderPassTimer;
    Time::ProfileTimer* _buildCommandBufferTimer;
    Time::ProfileTimer* _flushCommandBufferTimer;
    Time::ProfileTimer* _postFxRenderTimer;
    std::array<vectorEASTLFast<RenderPackage*>, to_base(RenderStage::COUNT)> _renderQueues;
};

};  // namespace Divide

#endif
