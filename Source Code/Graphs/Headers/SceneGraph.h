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
#ifndef _SCENE_GRAPH_H_
#define _SCENE_GRAPH_H_

#include "Octree.h"
#include "Scenes/Headers/SceneComponent.h"
#include "Rendering/Headers/FrameListener.h"

namespace ECS {
    class ECSEngine;
};

namespace Divide {
class Ray;
class SceneState;
class ECSManager;

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

    inline const SceneGraphNode& getRoot() const noexcept {
        return *_root;
    }

    inline SceneGraphNode& getRoot() noexcept {
        return *_root;
    }

    inline SceneGraphNode* findNode(const Str128& name, bool sceneNodeName = false) const {
        if (sceneNodeName ? _root->getNode().resourceName().compare(name.c_str()) == 0
                          : _root->name().compare(name) == 0) {
            return _root;
        }

        return _root->findChild(name, sceneNodeName, true);
    }

    inline SceneGraphNode* findNode(I64 guid) const {
        if (_root->getGUID() == guid) {
            return _root;
        }

        return _root->findChild(guid, false, true);
    }

    inline Octree& getOctree() noexcept {
        return *_octree;
    }

    inline const Octree& getOctree() const noexcept {
        return *_octree;
    }

    /// Update all nodes. Called from "updateSceneState" from class Scene
    void sceneUpdate(const U64 deltaTimeUS, SceneState& sceneState);
    void onStartUpdateLoop(const U8 loopNumber);
    void idle();

    // Return true if intersections is not empty. Just shorthand ...
    bool intersect(const Ray& ray, F32 start, F32 end, vectorSTD<SGNRayResult>& intersections) const;

    SceneGraphNode* createSceneGraphNode(SceneGraph& sceneGraph, const SceneGraphNodeDescriptor& descriptor);

    void destroySceneGraphNode(SceneGraphNode*& node, bool inPlace = true);
    void addToDeleteQueue(SceneGraphNode* node, vec_size childIdx);

    // If this function returns true, the node was successfully removed (or queued for removal)
    bool removeNode(I64 guid);
    bool removeNode(SceneGraphNode* node);
    // If this function returns true, nodes of the specified type were successfully removed (or queued for removal)
    bool removeNodesByType(SceneNodeType nodeType);

    const vectorEASTL<SceneGraphNode*>& getNodesByType(SceneNodeType type) const;

    inline void getNodesByType(std::initializer_list<SceneNodeType> types, vectorEASTL<SceneGraphNode*>& nodesOut) const {
        nodesOut.resize(0);
        for (const SceneNodeType type : types) {
            const vectorEASTL<SceneGraphNode*>& nodes = getNodesByType(type);
            nodesOut.insert(eastl::cend(nodesOut), eastl::cbegin(nodes), eastl::cend(nodes));
        }
    }

    inline U32 getTotalNodeCount() const noexcept { return to_U32(_allNodes.size()); }

    void onNetworkSend(U32 frameCount);

    void postLoad();

    void saveToXML(DELEGATE<void, const char*> msgCallback) const;
    void loadFromXML();

    ECSManager& GetECSManager() noexcept { return *_ecsManager; }
    const ECSManager& GetECSManager() const noexcept { return *_ecsManager; }

    ECS::EntityManager* GetEntityManager();
    ECS::EntityManager* GetEntityManager() const;
    ECS::ComponentManager* GetComponentManager();
    ECS::ComponentManager* GetComponentManager() const;

    bool saveCache(ByteBuffer& outputBuffer) const;
    bool loadCache(ByteBuffer& inputBuffer);

    inline ECS::ECSEngine& GetECSEngine() noexcept { return _ecsEngine; }
    inline const ECS::ECSEngine& GetECSEngine() const noexcept { return _ecsEngine; }

   protected:
    void onNodeDestroy(SceneGraphNode& oldNode);
    void onNodeAdd(SceneGraphNode& newNode);
    void onNodeTransform(SceneGraphNode& node);

    bool frameStarted(const FrameEvent& evt) noexcept override;
    bool frameEnded(const FrameEvent& evt) noexcept override;

   private:
    ECS::ECSEngine _ecsEngine;
    std::unique_ptr<ECSManager> _ecsManager;

    bool _loadComplete;
    bool _octreeChanged;
    bool _nodeListChanged;

    SceneGraphNode* _root;
    std::shared_ptr<Octree> _octree;
    std::atomic_bool _octreeUpdating;
    vectorEASTL<SceneGraphNode*> _allNodes;
    vectorEASTL<SceneGraphNode*> _orderedNodeList;

    std::array<vectorEASTL<SceneGraphNode*>, to_base(SceneNodeType::COUNT)> _nodesByType;

    mutable SharedMutex _pendingDeletionLock;
    hashMap<SceneGraphNode*, vectorSTD<vec_size>> _pendingDeletion;

    mutable std::mutex _nodeCreateMutex;

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

    static void onNodeTransform(SceneGraph& sceneGraph, SceneGraphNode& node)
    {
        sceneGraph.onNodeTransform(node);
    }
             
    friend class Divide::SceneGraphNode;
};

};  // namespace Attorney

};  // namespace Divide
#endif