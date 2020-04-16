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

#ifndef _RENDER_PASS_CULLER_H_
#define _RENDER_PASS_CULLER_H_

#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Core/Math/Headers/MathVectors.h"

/// This class performs all the necessary visibility checks on the scene's
/// scenegraph to decide what get's rendered and what not
namespace Divide {
class SceneState;
class SceneRenderState;

struct Task;
class Camera;
class SceneNode;
class SceneGraph;
class SceneGraphNode;
class PlatformContext;
enum class RenderStage : U8;

struct NodeCullParams {
    vec4<U16> _lodThresholds = {1000u};
    vec3<F32> _minExtents = { 0.0f };
    std::pair<I64*, size_t> _ignoredGUIDS;
    const Camera* _currentCamera = nullptr;
    F32 _cullMaxDistanceSq = 0.0f;
    I32 _minLoD = -1;
    RenderStage _stage = RenderStage::COUNT;
};

struct VisibleNode {
    SceneGraphNode* _node = nullptr;
    F32 _distanceToCameraSq = 0.0f;
};

using VisibleNodeList = eastl::fixed_vector<VisibleNode, Config::MAX_VISIBLE_NODES>;
using NodeListContainer = vectorEASTL<VisibleNodeList>;

class RenderPassCuller {
    public:
        RenderPassCuller() = default;
        ~RenderPassCuller() = default;

        static bool onStartup();
        static bool onShutdown();

        void clear() noexcept;

        VisibleNodeList& frustumCull(const NodeCullParams& params, const SceneGraph& sceneGraph, const SceneState& sceneState, PlatformContext& context);

        VisibleNodeList frustumCull(const NodeCullParams& params, const vectorEASTL<SceneGraphNode*>& nodes) const;
        VisibleNodeList toVisibleNodes(const Camera& camera, const vectorEASTL<SceneGraphNode*>& nodes) const;

        inline VisibleNodeList& getNodeCache(RenderStage stage) noexcept { return _visibleNodes[to_U32(stage)]; }
        inline const VisibleNodeList& getNodeCache(RenderStage stage) const noexcept { return _visibleNodes[to_U32(stage)]; }

    protected:
        void frustumCullNode(const Task* parentTask, SceneGraphNode& node, const NodeCullParams& params, U8 recursionLevel, VisibleNodeList& nodes) const;
        void addAllChildren(const SceneGraphNode& currentNode, const NodeCullParams& params,  VisibleNodeList& nodes) const;

    protected:
        std::array<VisibleNodeList, to_base(RenderStage::COUNT)> _visibleNodes;
};

};  // namespace Divide
#endif
