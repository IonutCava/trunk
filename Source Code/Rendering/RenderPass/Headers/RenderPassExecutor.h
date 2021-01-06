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
#ifndef _RENDER_PASS_EXECUTOR_H_
#define _RENDER_PASS_EXECUTOR_H_

#include "NodeBufferedData.h"
#include "RenderPass.h"
#include "RenderQueue.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "Platform/Video/Headers/GenericDrawCommand.h"
#include "Platform/Video/Headers/RenderPackage.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

namespace Divide {
struct PerPassData;
struct RenderTargetID;
struct RenderStagePass;
struct RenderPassParams;

class RenderTarget;
class RenderPassManager;
class RenderingComponent;

namespace GFX {
    class CommandBuffer;
}

class RenderPassExecutor
{
public:
    struct MaterialUpdateRange
    {
        U16 _firstIDX = std::numeric_limits<U16>::max();
        U16 _lastIDX = 0u;

        [[nodiscard]] U16 range() const noexcept { return _lastIDX >= _firstIDX ? _lastIDX - _firstIDX + 1u : 0u; }

        void reset() noexcept {
            _firstIDX = std::numeric_limits<U16>::max();
            _lastIDX = 0u;
        }
    };

public:
    explicit RenderPassExecutor(RenderPassManager& parent, GFXDevice& context, RenderStage stage);
    ~RenderPassExecutor() = default;

    void doCustomPass(RenderPassParams params, GFX::CommandBuffer& bufferInOut);
    void postInit(const ShaderProgram_ptr& OITCompositionShader) const;

private:
    // Returns false if we skipped the pre-pass step
    void prePass(const VisibleNodeList<>& nodes,
                 const RenderPassParams& params,
                 GFX::CommandBuffer& bufferInOut);

    void occlusionPass(const VisibleNodeList<>& nodes,
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


    void prepareRenderQueues(const RenderPassParams& params,
                             const VisibleNodeList<>& nodes,
                             bool transparencyPass,
                             RenderingOrder renderOrder = RenderingOrder::COUNT);

    
    NodeDataIdx processVisibleNode(const RenderingComponent& rComp,
                                   RenderStage stage,
                                   D64 interpolationFactor,
                                   U32 materialElementOffset,
                                   U16 nodeIndex);

    void buildDrawCommands(const RenderPassParams& params, bool doPrePass, bool doOITPass, GFX::CommandBuffer& bufferInOut);
    U16 prepareNodeData(VisibleNodeList<>& nodes, const RenderPassParams& params, bool hasInvalidNodes, const bool doPrePass, const bool doOITPass, GFX::CommandBuffer& bufferInOut);

    void renderQueueToSubPasses(GFX::CommandBuffer& commandsInOut,
                                RenderPackage::MinQuality qualityRequirement = RenderPackage::MinQuality::COUNT) const;

    [[nodiscard]] U32 renderQueueSize(RenderPackage::MinQuality qualityRequirement = RenderPackage::MinQuality::COUNT) const;

    void addTexturesAt(size_t idx, const NodeMaterialTextures& tempTextures);

private:
    struct PerRingEntryMaterialData {
        MaterialUpdateRange _matUpdateRange{};
        vectorEASTL<NodeMaterialData> _nodeMaterialData{};
        vectorEASTL<std::pair<size_t, U16>> _nodeMaterialLookupInfo{};
    };

    RenderPassManager& _parent;
    GFXDevice& _context;
    const RenderStage _stage;
    RenderQueue _renderQueue;
    RenderBin::SortedQueues _sortedQueues{};
    DrawCommandContainer _drawCommands{};
    RenderQueuePackages _renderQueuePackages{};

    U32 _materialBufferIndex = 0u;

    std::array<NodeTransformData, Config::MAX_VISIBLE_NODES> _nodeTransformData{};

    std::array<PerRingEntryMaterialData, RenderPass::DataBufferRingSize> _materialData{};

    eastl::set<SamplerAddress> _uniqueTextureAddresses{};

    static Pipeline* s_OITCompositionPipeline;
};
} //namespace Divide

#endif //_RENDER_PASS_EXECUTOR_H_