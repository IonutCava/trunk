/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _RENDER_PASS_CULLER_H_
#define _RENDER_PASS_CULLER_H_

#include "Utility/Headers/Vector.h"

/// This class performs all the necessary visibility checks on the scene's scenegraph to decide what get's rendered and what not
/// All node's that should be rendered, will be added to the RenderQueue
class SceneState;
class SceneRenderState;
class SceneGraphNode;

class RenderPassCuller {

public:
    RenderPassCuller();
    ~RenderPassCuller();
    /// This method performs the visibility check on the given node and all of it's children and adds them to the RenderQueue
    void cullSceneGraph(SceneGraphNode* const currentNode, SceneState& sceneState);
    void refresh();

protected:
    /// Perform CPU-based culling (Frustrum - AABB, distance check, etc)
    void cullSceneGraphCPU(SceneGraphNode* const currentNode, SceneRenderState& sceneRenderState);
    /// Perform GPU-based culling (e.g. Occlusion queries)
    void cullSceneGraphGPU(SceneState& sceneState);
    /// Internal cleanup
    void refreshNodeList();

protected:
    vectorImpl<SceneGraphNode* > _visibleNodes;

};

#endif