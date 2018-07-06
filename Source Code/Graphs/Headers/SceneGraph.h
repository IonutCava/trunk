/*
   Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _SCENE_GRAPH_H_
#define _SCENE_GRAPH_H_

#include "Octree.h"
#include "Scenes/Headers/SceneComponent.h"
#include "Rendering/Headers/FrameListener.h"

namespace Divide {
class Ray;
class SceneState;

namespace Attorney {
    class SceneGraphSGN;
};

class SceneGraph : private NonCopyable,
                   public FrameListener,
                   public SceneComponent
{

    friend class Attorney::SceneGraphSGN;
   public:
    explicit SceneGraph(Scene& parentScene);
    ~SceneGraph();

    void unload();

    inline const SceneGraphNode& getRoot() const {
        return *_root;
    }

    inline SceneGraphNode& getRoot() {
        return *_root;
    }

    inline SceneGraphNode_wptr findNode(const stringImpl& name, bool sceneNodeName = false) const {
        if (sceneNodeName ? _root->getNode()->getName().compare(name) == 0
                          : _root->getName().compare(name) == 0) {
            return _root;
        }

        return _root->findChild(name, sceneNodeName, true);
    }

    inline SceneGraphNode_wptr findNode(I64 guid) const {
        if (_root->getGUID() == guid) {
            return _root;
        }

        return _root->findChild(guid, true);
    }

    inline Octree& getOctree() {
        return *_octree;
    }

    inline const Octree& getOctree() const {
        return *_octree;
    }

    /// Update all nodes. Called from "updateSceneState" from class Scene
    void sceneUpdate(const U64 deltaTimeUS, SceneState& sceneState);

    void idle();

    void intersect(const Ray& ray, F32 start, F32 end, vectorImpl<I64>& selectionHits) const;

    // If deferrRemoval is true, removal will happen on the next "idle" call.
    // If this function returns true, the node was successfully removed (or queued for removal)
    bool removeNode(SceneGraphNode_wptr node, bool deferrRemoval = true);
    // If deferrRemoval is true, removal will happen on the next "idle" call.
    // If this function returns true, nodes of the specified type were successfully removed (or queued for removal)
    bool removeNodesByType(SceneNodeType nodeType, bool deferrRemoval = true);

    void onCameraUpdate(const Camera& camera);
    void onCameraChange(const Camera& camera);
    void onNetworkSend(U32 frameCount);

    void postLoad();

    const vectorImpl<SceneGraphNode_wptr>& getNodesByType(SceneNodeType type) const;

   protected:
    void onNodeDestroy(SceneGraphNode& oldNode);
    void onNodeAdd(SceneGraphNode& newNode);
    void onNodeTransform(SceneGraphNode& node);

    void unregisterNode(I64 guid, SceneGraphNode::UsageContext usage);

    bool frameStarted(const FrameEvent& evt);
    bool frameEnded(const FrameEvent& evt);

   private:
    bool _loadComplete;
    bool _octreeChanged;
    SceneRoot_ptr _rootNode;
    SceneGraphNode_ptr _root;
    std::shared_ptr<Octree> _octree;
    std::atomic_bool _octreeUpdating;
    vectorImpl<SceneGraphNode_wptr> _allNodes;
    vectorImpl<SceneNodeType> _pendingRemovalNodeTypes;
    vectorImpl<SceneGraphNode_wptr> _pendingRemovalNodes;
    vectorImpl<SceneGraphNode*> _orderedNodeList;

    std::array<vectorImpl<SceneGraphNode_wptr>, to_base(SceneNodeType::COUNT)> _nodesByType;
};

namespace Attorney {
class SceneGraphSGN {
   private:
    static void onNodeAdd(SceneGraph& sceneGraph, SceneGraphNode& newNode) {
        sceneGraph.onNodeAdd(newNode);
    }

    static void onNodeDestroy(SceneGraph& sceneGraph, SceneGraphNode& oldNode) {
        sceneGraph.onNodeDestroy(oldNode);
    }

    static void onNodeUsageChange(SceneGraph& sceneGraph, 
                                  SceneGraphNode& node,
                                  SceneGraphNode::UsageContext oldUsage,
                                  SceneGraphNode::UsageContext newUsage)
    {
        sceneGraph.unregisterNode(node.getGUID(), node.usageContext());

    }

    static void onNodeTransform(SceneGraph& sceneGraph, SceneGraphNode& node)
    {
        sceneGraph.onNodeTransform(node);
    }
             
    friend class Divide::SceneGraphNode;
};

};  // namespace Attorney

};  // namespace Divide
#endif