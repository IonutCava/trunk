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
    vec4<U16> _lodThresholds;
    vec3<F32> _minExtents = { 0.0f };
    const Camera* _currentCamera = nullptr;
    F32 _cullMaxDistanceSq = 0.0f;
    I32 _minLoD = -1;
    RenderStage _stage = RenderStage::COUNT;;
    bool _threaded = false;
};

struct VisibleNode {
    F32 _distanceToCameraSq = 0.0f;
    SceneGraphNode* _node = nullptr;
};

using VisibleNodeList = vectorEASTL<VisibleNode>;
using NodeListContainer = vectorEASTL<VisibleNodeList>;
constexpr size_t NodeListContainerSizeInBytes = prevPOW2(Config::MAX_VISIBLE_NODES * sizeof(NodeListContainer) * to_base(RenderStage::COUNT));

class RenderPassCuller {
    public:
        struct CullParams {
            vec3<F32> _minExtents = { 0.0f };
            PlatformContext* _context = nullptr;
            const SceneGraph* _sceneGraph = nullptr;
            const Camera* _camera = nullptr;
            const SceneState* _sceneState = nullptr;
            F32 _visibilityDistanceSq = std::numeric_limits<F32>::max();
            I32 _minLoD = -1;
            RenderStage _stage = RenderStage::COUNT;
            bool _threaded = true;

        };

    public:
        RenderPassCuller() = default;
        ~RenderPassCuller() = default;

        void clear();

        VisibleNodeList& frustumCull(const CullParams& params);

        VisibleNodeList frustumCull(const NodeCullParams& params, const vectorEASTL<SceneGraphNode*>& nodes) const;
        VisibleNodeList toVisibleNodes(const Camera& camera, const vectorEASTL<SceneGraphNode*>& nodes) const;

        inline VisibleNodeList& getNodeCache(RenderStage stage) { return _visibleNodes[to_U32(stage)]; }
        inline const VisibleNodeList& getNodeCache(RenderStage stage) const { return _visibleNodes[to_U32(stage)]; }

    protected:
        void frustumCullNode(const Task& parentTask, SceneGraphNode& node, const NodeCullParams& params, VisibleNodeList& nodes) const;
        void addAllChildren(SceneGraphNode& currentNode, const NodeCullParams& params,  VisibleNodeList& nodes) const;

    protected:
        std::array<VisibleNodeList, to_base(RenderStage::COUNT)> _visibleNodes;

        static std::mutex s_nodeListContainerMutex;
        static MemoryPool<NodeListContainer, NodeListContainerSizeInBytes> s_nodeListContainer;
};

};  // namespace Divide
#endif
