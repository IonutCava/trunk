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
class SceneEnvironmentProbePool;

enum class RenderStage : U8;

struct RenderPassParams
{
    FrustumClipPlanes _clippingPlanes = {};
    vec3<F32> _minExtents = { 0.0f };
    // source node is used to determine if the current pass is triggered by a specific node:
    // e.g. a light node for shadow mapping, a reflector for reflection (or refraction), etc
    // safe to be set to null
    FeedBackContainer* _feedBackContainer = nullptr;
    const SceneGraphNode* _sourceNode = nullptr;
    Camera* _camera = nullptr;
    Str64 _passName = "";
    I32 _minLoD = -1; //-1 = all
    RenderTargetID _target = {};

    RTDrawDescriptor _targetDescriptorPrePass = {};
    RTDrawDescriptor _targetDescriptorMainPass = {};

    RenderTargetID _targetHIZ = {};
    RenderTargetID _targetOIT = {};
    RenderStagePass _stagePass = {};

    RenderTarget::DrawLayerParams _layerParams = {};

    //Ughhhh
    bool _shadowMappingEnabled = true;
};

class RenderPassManager final : public KernelComponent {

public:
    struct RenderParams {
        SceneRenderState* _sceneRenderState = nullptr;
        Rect<I32> _targetViewport = {};
        Time::ProfileTimer* _parentTimer = nullptr;
        bool _editorRunning = false;
    };

    explicit RenderPassManager(Kernel& parent, GFXDevice& context);
    ~RenderPassManager();

    /// Call every render queue's render function in order
    void render(const RenderParams& params);
    /// Add a new pass that will run once for each of the RenderStages specified
    RenderPass& addRenderPass(const Str64& renderPassName,
                              U8 orderKey,
                              RenderStage renderStage,
                              const vectorEASTL<U8>& dependencies = {},
                              bool usePerformanceCounters = false);
    /// Find a render pass by name and remove it from the manager
    void removeRenderPass(const Str64& name);
    [[nodiscard]] U32  getLastTotalBinSize(RenderStage renderStage) const;
    [[nodiscard]] I32 drawCallCount(const RenderStage stage) const noexcept { return _drawCallCount[to_base(stage)]; }

    [[nodiscard]] RenderQueue& getQueue() noexcept { return _renderQueue; }

    [[nodiscard]] RenderPass::BufferData getBufferData(const RenderStagePass& stagePass) const;

    void doCustomPass(RenderPassParams params, GFX::CommandBuffer& bufferInOut);
    void postInit();

private:
    // Returns false if we skipped the pre-pass step
    bool prePass(const VisibleNodeList<>& nodes,
                 const RenderPassParams& params,
                 const RenderTarget& target,
                 U32& visibleNodeCount,
                 GFX::CommandBuffer& bufferInOut);
    bool occlusionPass(const VisibleNodeList<>& nodes,
                       U32 visibleNodeCount,
                       const RenderStagePass& stagePass,
                       const Camera& camera,
                       const RenderTargetID& sourceDepthBuffer,
                       const RenderTargetID& targetDepthBuffer,
                       GFX::CommandBuffer& bufferInOut) const;
    void mainPass(const VisibleNodeList<>& nodes,
                  const RenderPassParams& params,
                  RenderTarget& target,
                  bool prePassExecuted,
                  bool hasHiZ,
                  GFX::CommandBuffer& bufferInOut);

    void transparencyPass(const VisibleNodeList<>& nodes,
                          const RenderPassParams& params,
                          GFX::CommandBuffer& bufferInOut);

    
    void woitPass(const VisibleNodeList<>& nodes,
                  const RenderPassParams& params,
                  GFX::CommandBuffer& bufferInOut);

    [[nodiscard]] RenderPass& getPassForStage(RenderStage renderStage);
    [[nodiscard]] const RenderPass& getPassForStage(RenderStage renderStage) const;
    void prepareRenderQueues(const RenderPassParams& params, const VisibleNodeList<>& nodes, bool refreshNodeData, bool transparencyPass, RenderingOrder renderOrder = RenderingOrder::COUNT);
    // Returns the number of processed nodes that will get rendered (the number of draw packages uploaded to the GPU
    U32 buildDrawCommands(const RenderPassParams& params, bool refreshNodeData, GFX::CommandBuffer& bufferInOut);
    // Returns the number of processed nodes that will get rendered (the number of draw packages uploaded to the GPU
    U32  buildBufferData(const RenderStagePass& stagePass, const SceneRenderState& renderState, const RenderPassParams& passParams, bool fullRefresh, GFX::CommandBuffer& bufferInOut);
    void processVisibleNode(const RenderingComponent& rComp, const RenderStagePass& stagePass, bool playAnimations, D64 interpolationFactor, bool needsInterp, GFXDevice::NodeData& dataOut, GFXDevice::CollisionData& collisionDataOut) const;

private: //TEMP
    friend class RenderBin;
    [[nodiscard]] U32  renderQueueSize(RenderStage stage, RenderPackage::MinQuality qualityRequirement = RenderPackage::MinQuality::COUNT) const;
    void renderQueueToSubPasses(RenderStage stage, GFX::CommandBuffer& commandsInOut, RenderPackage::MinQuality qualityRequirement = RenderPackage::MinQuality::COUNT) const;

private:
    GFXDevice& _context;
    RenderQueue _renderQueue;

    vectorEASTL<Task*> _renderTasks;
    vectorEASTL<RenderPass*> _renderPasses;
    vectorEASTL<GFX::CommandBuffer*> _renderPassCommandBuffer;
    GFX::CommandBuffer* _postFXCommandBuffer = nullptr;
    GFX::CommandBuffer* _postRenderBuffer = nullptr;

    Pipeline* _OITCompositionPipeline = nullptr;

    ShaderProgram_ptr _OITCompositionShader = nullptr;
    Time::ProfileTimer* _renderPassTimer = nullptr;
    Time::ProfileTimer* _buildCommandBufferTimer = nullptr;
    Time::ProfileTimer* _flushCommandBufferTimer = nullptr;
        Time::ProfileTimer* _postFxRenderTimer = nullptr;
    Time::ProfileTimer* _blitToDisplayTimer = nullptr;

    std::array<RenderQueuePackages, to_base(RenderStage::COUNT)> _renderQueues;
    std::array<I32, to_base(RenderStage::COUNT)> _drawCallCount;
};

} // namespace Divide

#endif
