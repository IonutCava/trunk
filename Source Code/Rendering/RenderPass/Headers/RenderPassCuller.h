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
    const Camera* _currentCamera = nullptr;
    F32 _cullMaxDistanceSq = 0.0f;
    I32 _minLoD = false;
    RenderStage _stage = RenderStage::COUNT;;
    bool _threaded = false;
};

class RenderPassCuller {
   public:
    //Should return true if the node is not inside the frustum
    typedef std::function<bool(const SceneGraphNode&, const SceneNode&)> CullingFunction;

    // draw order, node pointer
    struct VisibleNode {
        F32 _distanceToCameraSq = 0.0f;
        const SceneGraphNode* _node = nullptr;
    };

    typedef vectorEASTL<VisibleNode> VisibleNodeList;

    struct CullParams {
        CullingFunction _cullFunction;
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
    RenderPassCuller();
    ~RenderPassCuller();

    // flush all caches
    void clear();

    VisibleNodeList& getNodeCache(RenderStage stage);
    const VisibleNodeList& getNodeCache(RenderStage stage) const;

    RenderPassCuller::VisibleNodeList& frustumCull(const CullParams& params);

    RenderPassCuller::VisibleNodeList frustumCull(const NodeCullParams& params, const vectorEASTL<SceneGraphNode*>& nodes) const;
    RenderPassCuller::VisibleNodeList toVisibleNodes(const Camera& camera, const vectorEASTL<SceneGraphNode*>& nodes) const;

    bool wasNodeInView(I64 GUID, RenderStage stage) const;

   protected:

    // return true if the node is not currently visible
    void frustumCullNode(const Task& parentTask,
                         const SceneGraphNode& node,
                         const NodeCullParams& params,
                         bool clearList,
                         VisibleNodeList& nodes) const;

    void addAllChildren(const SceneGraphNode& currentNode,
                        const NodeCullParams& params,
                        VisibleNodeList& nodes) const;

   protected:
    std::array<CullingFunction, to_base(RenderStage::COUNT)> _cullingFunction;
    std::array<VisibleNodeList, to_base(RenderStage::COUNT)> _visibleNodes;
};

};  // namespace Divide
#endif
