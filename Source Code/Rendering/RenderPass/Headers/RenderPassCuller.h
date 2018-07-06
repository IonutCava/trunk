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

#include "Utility/Headers/Vector.h"
#include <functional>

/// This class performs all the necessary visibility checks on the scene's
/// scenegraph
/// to decide what get's rendered and what not
/// All node's that should be rendered, will be added to the RenderQueue
namespace Divide {
class SceneState;
class SceneRenderState;
class SceneGraphNode;

class RenderPassCuller {
   public:
    struct RenderableNode {
        SceneGraphNode* _visibleNode;
        bool _isDrawReady;

        explicit RenderableNode()
            : _visibleNode(nullptr), _isDrawReady(false) {}
        explicit RenderableNode(SceneGraphNode& node)
            : _visibleNode(&node), _isDrawReady(false) {}
    };

   public:
    RenderPassCuller();
    ~RenderPassCuller();
    /// This method performs the visibility check on the given node and all of
    /// it's children and
    /// adds them to the RenderQueue
    void cullSceneGraph(
        SceneGraphNode& currentNode,
        SceneState& sceneState,
        const std::function<bool(SceneGraphNode*)>& cullingFunction);
    void refresh();

   protected:
    /// Perform CPU-based culling (Frustrum - AABB, distance check, etc)
    void cullSceneGraphCPU(
        SceneGraphNode& currentNode,
        SceneRenderState& sceneRenderState,
        const std::function<bool(SceneGraphNode*)>& cullingFunction);
    /// Perform GPU-based culling (e.g. Occlusion queries)
    void cullSceneGraphGPU(
        SceneState& sceneState,
        const std::function<bool(SceneGraphNode*)>& cullingFunction);
    /// Internal cleanup
    void refreshNodeList();
    /// Sort visible nodes based on their render bin order
    void sortVisibleNodes();

   protected:
    vectorImpl<RenderableNode> _visibleNodes;
    bool _visibleNodesSorted;
};

};  // namespace Divide
#endif
