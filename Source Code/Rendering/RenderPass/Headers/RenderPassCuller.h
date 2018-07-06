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
#include "Platform/Headers/PlatformDefines.h"

/// This class performs all the necessary visibility checks on the scene's
/// scenegraph to decide what get's rendered and what not
namespace Divide {
class SceneState;
class SceneRenderState;

struct Task;
class Camera;
class SceneGraph;
class SceneGraphNode;
class PlatformContext;
enum class RenderStage : U8;

template<typename T>
class vec3;

class RenderPassCuller {
   public:
    //Should return true if the node is not inside the frustum
    typedef std::function<bool(const SceneGraphNode&)> CullingFunction;

    // draw order, node pointer
    struct VisibleNode {
        F32 _distanceToCameraSq = 0.0f;
        const SceneGraphNode* _node = nullptr;
    };

    typedef vectorEASTL<VisibleNode> VisibleNodeList;

    struct CullParams {
        PlatformContext* _context = nullptr;
        SceneGraph* _sceneGraph = nullptr;
        const Camera* _camera = nullptr;
        SceneState* _sceneState = nullptr;
        RenderStage _stage = RenderStage::COUNT;
        F32 _visibilityDistanceSq = 0.0f;
        CullingFunction _cullFunction;
    };

   public:
    RenderPassCuller();
    ~RenderPassCuller();

    // flush all caches
    void clear();

    VisibleNodeList& getNodeCache(RenderStage stage);
    const VisibleNodeList& getNodeCache(RenderStage stage) const;

    RenderPassCuller::VisibleNodeList& frustumCull(const CullParams& params);

    RenderPassCuller::VisibleNodeList frustumCull(const Camera& camera, F32 maxDistanceSq, RenderStage stage, const vectorEASTL<SceneGraphNode*>& nodes) const;
    RenderPassCuller::VisibleNodeList toVisibleNodes(const Camera& camera, const vectorEASTL<SceneGraphNode*>& nodes) const;

    bool wasNodeInView(I64 GUID, RenderStage stage) const;

   protected:

    // return true if the node is not currently visible
    void frustumCullNode(const Task& parentTask,
                         const SceneGraphNode& node,
                         const Camera& currentCamera,
                         RenderStage stage,
                         F32 cullMaxDistanceSq,
                         VisibleNodeList& nodes,
                         bool clearList) const;

    void addAllChildren(const SceneGraphNode& currentNode,
                        RenderStage stage,
                        const vec3<F32>& cameraEye,
                        VisibleNodeList& nodes) const;

   protected:
    std::array<CullingFunction, to_base(RenderStage::COUNT)> _cullingFunction;
    std::array<VisibleNodeList, to_base(RenderStage::COUNT)> _visibleNodes;
    std::array<vectorEASTL<VisibleNodeList>, to_base(RenderStage::COUNT)> _perThreadNodeList;
};

};  // namespace Divide
#endif
