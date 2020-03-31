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
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

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
        FrustumClipPlanes _clippingPlanes = {};
        vec3<F32> _minExtents = { 0.0f };
        // source node is used to determine if the current pass is triggered by a specific node:
        // e.g. a light node for shadow mapping, a reflector for reflection (or refraction), etc
        // safe to be set to null
        const SceneGraphNode* _sourceNode = nullptr;
        const RTDrawDescriptor* _drawPolicy = nullptr;
        const RTClearDescriptor* _clearDescriptor = nullptr;
        Camera* _camera = nullptr;
        Str64 _passName = "";
        I32 _minLoD = -1; //-1 = all
        RenderTargetID _target = {};
        RenderTargetID _targetHIZ = {};
        RenderStagePass _stagePass = {};
        bool _bindTargets = true;
    };

public:
    explicit RenderPassManager(Kernel& parent, GFXDevice& context);
    ~RenderPassManager();

    /// Call every renderqueue's render function in order
    void render(SceneRenderState& sceneRenderState, Time::ProfileTimer* parentTimer = nullptr);
    /// Draw all of the 2D game elements in the target viewport
    void createFrameBuffer(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut);
    /// Add a new pass that will run once for each of the RenderStages specified
    RenderPass& addRenderPass(const Str64& renderPassName,
                              U8 orderKey,
                              RenderStage renderStage,
                              std::vector<U8> dependencies = {},
                              bool usePerformanceCounters = false);
    /// Find a renderpass by name and remove it from the manager
    void removeRenderPass(const Str64& name);
    U32  getLastTotalBinSize(RenderStage renderStage) const;

    inline RenderQueue& getQueue() { return _renderQueue; }

    RenderPass::BufferData getBufferData(const RenderStagePass& stagePass) const;

    void doCustomPass(PassParams params, GFX::CommandBuffer& bufferInOut);
    void postInit();

private:
    // Returns false if we skipped the pre-pass step
    bool prePass(const VisibleNodeList& nodes,
                 const PassParams& params,
                 const RenderTarget& target,
                 GFX::CommandBuffer& bufferInOut);
    bool occlusionPass(const VisibleNodeList& nodes,
                       const PassParams& params,
                       vec2<bool> extraTargets, 
                       const RenderTarget& target,
                       bool prePassExecuted,
                       GFX::CommandBuffer& bufferInOut);
    void mainPass(const VisibleNodeList& nodes,
                  const PassParams& params,
                  vec2<bool> extraTargets,
                  RenderTarget& target,
                  bool prePassExecuted,
                  bool hasHiZ,
                  GFX::CommandBuffer& bufferInOut);

    void woitPass(const VisibleNodeList& nodes,
                  const PassParams& params,
                  vec2<bool> extraTargets,
                  const RenderTarget& target,
                  GFX::CommandBuffer& bufferInOut);

    RenderPass& getPassForStage(RenderStage renderStage);
    const RenderPass& getPassForStage(RenderStage renderStage) const;
    void prepareRenderQueues(const PassParams& params, const VisibleNodeList& nodes, bool refreshNodeData, GFX::CommandBuffer& bufferInOut);
    void buildDrawCommands(const PassParams& params, bool refreshNodeData, GFX::CommandBuffer& bufferInOut);
    void buildBufferData(const RenderStagePass& stagePass, const SceneRenderState& renderState, const Camera& camera, const RenderBin::SortedQueues& sortedQueues, bool fullRefresh, GFX::CommandBuffer& bufferInOut);
    void processVisibleNode(const RenderingComponent& rComp, const RenderStagePass& stagePass, bool playAnimations, const mat4<F32>& viewMatrix, const D64 interpolationFactor, bool needsInterp, GFXDevice::NodeData& dataOut) const;

private: //TEMP
    friend class RenderBin;
    U32  renderQueueSize(RenderStage stage, RenderPackage::MinQuality qualityRequirement = RenderPackage::MinQuality::COUNT) const;
    void renderQueueToSubPasses(RenderStage stage, GFX::CommandBuffer& commandsInOut, RenderPackage::MinQuality qualityRequirement = RenderPackage::MinQuality::COUNT) const;

private:
    GFXDevice& _context;
    RenderQueue _renderQueue;

    vectorEASTL<Task*> _renderTasks;
    vectorEASTL<bool> _completedPasses;
    vectorEASTL<RenderPass*> _renderPasses;
    vectorEASTL<GFX::CommandBuffer*> _renderPassCommandBuffer;
    GFX::CommandBuffer* _postFXCommandBuffer;

    Pipeline* _OITCompositionPipeline = nullptr;

    ShaderProgram_ptr _OITCompositionShader;
    Time::ProfileTimer* _renderPassTimer;
    Time::ProfileTimer* _buildCommandBufferTimer;
    Time::ProfileTimer* _flushCommandBufferTimer;
    Time::ProfileTimer* _postFxRenderTimer;
    std::array<vectorEASTLFast<RenderPackage*>, to_base(RenderStage::COUNT)> _renderQueues;
};

};  // namespace Divide

#endif
