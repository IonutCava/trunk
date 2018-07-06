/*
   Copyright (c) 2015 DIVIDE-Studio
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
#include <future>

/// This class performs all the necessary visibility checks on the scene's
/// scenegraph to decide what get's rendered and what not
namespace Divide {
class SceneState;
class SceneRenderState;

class SceneGraph;
class SceneGraphNode;
typedef std::weak_ptr<SceneGraphNode> SceneGraphNode_wptr;
class RenderPassCuller {
   public:
    typedef vectorImpl<SceneGraphNode_wptr> VisibleNodeList;

    //Should return true if the node is not inside the frustum
    typedef std::function<bool(const SceneGraphNode&)> CullingFunction;

   public:
    RenderPassCuller();
    ~RenderPassCuller();

    VisibleNodeList& getNodeCache(RenderStage stage);
    void frustumCull(SceneGraph& sceneGraph,
                     SceneState& sceneState,
                     RenderStage stage,
                     bool async,
                     const CullingFunction& cullingFunction);
   protected:

    // return true if the node is not currently visible
    void frustumCullNode(SceneGraphNode& node,
                         RenderStage currentStage,
                         SceneRenderState& sceneRenderState,
                         VisibleNodeList& nodes);

   protected:
    CullingFunction _cullingFunction;
    vectorImpl<std::future<void>> _cullingTasks;
    vectorImpl<VisibleNodeList> _perThreadNodeList;
    std::array<VisibleNodeList, to_const_uint(RenderStage::COUNT)> _visibleNodes;
};

};  // namespace Divide
#endif
