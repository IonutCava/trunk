/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _SCENE_GRAPH_NODE_H_
#define _SCENE_GRAPH_NODE_H_

#include "SceneNode.h"
#include "SGNRelationshipCache.h"
#include "Utility/Headers/StateTracker.h"
#include "Graphs/Components/Headers/IKComponent.h"
#include "Graphs/Components/Headers/BoundsComponent.h"
#include "Graphs/Components/Headers/RagdollComponent.h"
#include "Graphs/Components/Headers/PhysicsComponent.h"
#include "Graphs/Components/Headers/AnimationComponent.h"
#include "Graphs/Components/Headers/NavigationComponent.h"
#include "Graphs/Components/Headers/RenderingComponent.h"

namespace Divide {
class Transform;
class SceneGraph;
class SceneState;

// This is the scene root node. All scene node's are added to it as child nodes
class SceneRoot : public SceneNode {
   public:
    SceneRoot() : SceneNode("root", SceneNodeType::TYPE_ROOT)
    {
        _renderState.useDefaultMaterial(false);
        setState(ResourceState::RES_LOADED);
        _boundingBox.set(VECTOR3_UNIT, -VECTOR3_UNIT);

    }

    bool onRender(SceneGraphNode& sgn, RenderStage currentStage) {
        return true;
    }

    bool unload() { return true; }
    bool load() override {
        return true; 
    }

   protected:
    friend class SceneGraph;
    void postLoad(SceneGraphNode& sgn) { SceneNode::postLoad(sgn); }
};
TYPEDEF_SMART_POINTERS_FOR_CLASS(SceneRoot);
// Add as many SceneTransform nodes are needed as parent nodes for any scenenode
// to create complex transforms in the scene
class SceneTransform : public SceneNode {
   public:
    SceneTransform() : SceneNode("TransformNode", SceneNodeType::TYPE_TRANSFORM) {
        _renderState.useDefaultMaterial(false);
        setState(ResourceState::RES_LOADED);
    }

    bool onRender(RenderStage currentStage) { return true; }

    void postLoad(SceneGraphNode& sgn) { return; }
    bool unload() { return true; }
    bool load() override { return true; }
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(SceneTransform);
TYPEDEF_SMART_POINTERS_FOR_CLASS(SceneGraphNode);

class SceneGraphNode : public GUIDWrapper,
                       private NonCopyable,
                       public std::enable_shared_from_this<SceneGraphNode> {
    static const size_t INITIAL_CHILD_COUNT = 128;
   public:
    /// Usage context affects lighting, navigation, physics, etc
    enum class UsageContext : U32 {
        NODE_DYNAMIC = 0,
        NODE_STATIC 
    };

    enum class SelectionFlag : U32 {
        SELECTION_NONE = 0,
        SELECTION_HOVER,
        SELECTION_SELECTED,
        SELECTION_COUNT
    };

    enum class UpdateFlag : U32 {
        SPATIAL_PARTITION = 0,
        COUNT
    };

    /// Called from SceneGraph "sceneUpdate"
    void sceneUpdate(const U64 deltaTime, SceneState& sceneState);
    /*Node Management*/
    /// Always use the level of redirection needed to reduce virtual function
    /// overhead
    /// Use getNode<SceneNode> if you need material properties for ex. or
    /// getNode<SubMesh> for animation transforms
    template <typename T = SceneNode>
    inline std::shared_ptr<T> getNode() const {
        static_assert(std::is_base_of<SceneNode, T>::value,
                      "SceneGraphNode::getNode error: Invalid target node type!");
        return std::static_pointer_cast<T>(_node);
    }
    /// Add node increments the node's ref counter if the node was already added
    /// to the scene graph
    SceneGraphNode_ptr addNode(const SceneNode_ptr& node, U32 componentMask, PhysicsGroup physicsGroup, const stringImpl& name = "");
    SceneGraphNode_ptr registerNode(SceneGraphNode_ptr node);
    /// If recursive is true, this stops on the first node match
    bool removeNode(SceneGraphNode& node, bool recursive = true);
    
    /// Find a node in the graph based on the SceneGraphNode's name
    /// If sceneNodeName = true, find a node in the graph based on the
    /// SceneNode's name
    SceneGraphNode_wptr findNode(const stringImpl& name, bool sceneNodeName = false);
    /// Find a child Node using the given name (either SGN name or SceneNode name)
    SceneGraphNode_wptr findChild(const stringImpl& name, bool sceneNodeName = false, bool recursive = false);
    /// Find a child using the give GUID
    SceneGraphNode_wptr findChild(I64 GUID, bool recursive = false);
    /// Find the graph nodes whom's bounding boxes intersects the given ray
    void intersect(const Ray& ray, F32 start, F32 end,
                   vectorImpl<SceneGraphNode_cwptr>& selectionHits,
                   bool recursive = true) const;

    inline void onCollisionCbk(const DELEGATE_CBK_PARAM<SceneGraphNode_cptr>& cbk) {
        _collisionCbk = cbk;
    }

    /// Selection helper functions
    void setSelectionFlag(SelectionFlag flag);
    inline SelectionFlag getSelectionFlag() const { return _selectionFlag; }

    void setSelectable(const bool state);
    inline bool isSelectable() const { return _isSelectable; }

    void lockVisibility(const bool state);
    inline bool visibilityLocked() const { return _visibilityLocked; }

    const stringImpl& getName() const { return _name; }
    /*Node Management*/

    /*Parent <-> Children*/
    inline SceneGraphNode_wptr getParent() const {
        return _parent;
    }

    void setParent(SceneGraphNode& parent);

    /*Node State*/
    void setActive(const bool state);
    inline bool isActive() const { return _active; }

    inline const UsageContext& usageContext() const { return _usageContext; }
    void usageContext(const UsageContext& newContext);

    inline U64 getElapsedTime() const { return _elapsedTime; }

    template <typename T>
    inline T* get() const {
        static_assert(false, "INVALID COMPONENT");
        return nullptr;
    }

    inline StateTracker<bool>& getTrackedBools() { return _trackedBools; }

    bool isChildOfType(U32 typeMask, bool ignoreRoot) const;
    bool isRelated(const SceneGraphNode& target) const;
    bool isChild(const SceneGraphNode& target, bool recursive) const;

    inline bool hasChildren() const {
        return getChildCount() > 0;
    }

    inline SceneGraphNode& getChild(U32 idx, U32& updatedChildCount) {
        ReadLock r_lock(_childLock);
        updatedChildCount = getChildCount();
        assert(idx < updatedChildCount);
        const SceneGraphNode_ptr& child = _children.at(idx);
        assert(child);
        return *child;
    }

    inline const SceneGraphNode& getChild(U32 idx, U32& updatedChildCount) const {
        ReadLock r_lock(_childLock);
        updatedChildCount = getChildCount();
        assert(idx < updatedChildCount);
        const SceneGraphNode_ptr& child = _children.at(idx);
        assert(child);
        return *child;
    }

    inline U32 getChildCount() const {
        return _childCount;
    }

    inline bool getFlag(UpdateFlag flag) const {
        return _updateFlags[to_uint(flag)];
    }

    inline void clearUpdateFlag(UpdateFlag flag) {
        _updateFlags[to_uint(flag)] = false;
    }

    bool operator==(const SceneGraphNode_ptr& other) const {
        return this->getGUID() == other->getGUID();
    }

    bool operator!=(const SceneGraphNode_ptr& other) const {
        return this->getGUID() != other->getGUID();
    }

   /*protected:
    SET_DELETE_FRIEND
    SET_SAFE_UPDATE_FRIEND
    SET_DELETE_VECTOR_FRIEND
    SET_DELETE_HASHMAP_FRIEND

    friend class SceneGraph;
    friend class std::shared_ptr<SceneGraphNode> ;*/
    explicit SceneGraphNode(SceneGraph& sceneGraph,
                            PhysicsGroup physicsGroup,
                            const SceneNode_ptr& node,
                            const stringImpl& name,
                            U32 componentMask);
    ~SceneGraphNode();

    bool operator==(const SceneGraphNode& rhs) const {
        return getGUID() == rhs.getGUID();
    }

    bool operator!=(const SceneGraphNode& rhs) const {
        return getGUID() != rhs.getGUID();
    }

    void postLoad();

    inline SceneGraph& parentGraph() {
        return _sceneGraph;
    }

    inline const SceneGraph& parentGraph() const {
        return _sceneGraph;
    }

   protected:
    friend class RenderPassCuller;
    // Returns true if the node should be culled (is not visible for the current stage)
    bool cullNode(const Camera& currentCamera,
                  F32 maxDistanceFromCamera,
                  RenderStage currentStage,
                  Frustum::FrustCollision& collisionTypeOut) const;

   protected:
    friend class RenderingComponent;
    bool prepareDraw(const SceneRenderState& sceneRenderState,
                     RenderStage renderStage);

   protected:
    friend class SceneGraph;
    void frameEnded();
    void onCameraUpdate(const I64 cameraGUID,
                        const vec3<F32>& posOffset,
                        const mat4<F32>& rotationOffset);

    inline void setUpdateFlag(UpdateFlag flag) {
        _updateFlags[to_uint(flag)] = true;
    }

    void sgnUpdate(const U64 deltaTime, SceneState& sceneState);

    void getOrderedNodeList(vectorImpl<SceneGraphNode*>& nodeList);

   protected:
    friend class Octree;
    void onCollision(SceneGraphNode_cwptr collider);

   private:
    void onTransform();
    bool filterCollission(const SceneGraphNode& node);
    inline void setName(const stringImpl& name) { 
        _name = name;
    }

    void setComponent(SGNComponent::ComponentType type, SGNComponent* component);

    inline U32 getComponentIdx(SGNComponent::ComponentType type) const {
        return powerOfTwo(to_uint(type)) - 1;
    }

    inline SGNComponent* getComponent(SGNComponent::ComponentType type) const {
        return _components[getComponentIdx(type)];
    }
   private:
    friend class SGNRelationshipCache;
    inline const SGNRelationshipCache& relationshipCache() const {
        return _relationshipCache;
    }
    void invalidateRelationshipCache();

    inline bool isHelperNode(const SceneNode& node) const {
        return node.getType() == SceneNodeType::TYPE_ROOT ||
               node.getType() == SceneNodeType::TYPE_TRANSFORM;
    }

   private:
    // An SGN doesn't exist outside of a scene graph
    SceneGraph& _sceneGraph;

    D64 _updateTimer;
    U64 _elapsedTime;
    stringImpl _name;
    SceneNode_ptr _node;
    SceneGraphNode_wptr _parent;
    vectorImpl<SceneGraphNode_ptr> _children;
    std::atomic_uint _childCount;
    mutable SharedLock _childLock;
    std::atomic<bool> _active;
    std::atomic<bool> _visibilityLocked;
    std::array<std::atomic<bool>, to_const_uint(UpdateFlag::COUNT)> _updateFlags;

    bool _isSelectable;
    SelectionFlag _selectionFlag;

    UsageContext _usageContext;

    StateTracker<bool> _trackedBools;

    DELEGATE_CBK_PARAM<SceneGraphNode_cptr> _collisionCbk;

    std::array<SGNComponent*, to_const_uint(SGNComponent::ComponentType::COUNT)> _components;
    SGNRelationshipCache _relationshipCache;
};

template <>
inline AnimationComponent* SceneGraphNode::get() const {
    return static_cast<AnimationComponent*>(getComponent(SGNComponent::ComponentType::ANIMATION));
}

template <>
inline IKComponent* SceneGraphNode::get() const {
    return static_cast<IKComponent*>(getComponent(SGNComponent::ComponentType::INVERSE_KINEMATICS));
}

template <>
inline RagdollComponent* SceneGraphNode::get() const {
    return static_cast<RagdollComponent*>(getComponent(SGNComponent::ComponentType::RAGDOLL));
}

template <>
inline BoundsComponent* SceneGraphNode::get() const {
    return static_cast<BoundsComponent*>(getComponent(SGNComponent::ComponentType::BOUNDS));
}

template <>
inline NavigationComponent* SceneGraphNode::get() const {
    return static_cast<NavigationComponent*>(getComponent(SGNComponent::ComponentType::NAVIGATION));
}

template <>
inline PhysicsComponent* SceneGraphNode::get() const {
    return static_cast<PhysicsComponent*>(getComponent(SGNComponent::ComponentType::PHYSICS));
}

template <>
inline RenderingComponent* SceneGraphNode::get() const {
    return static_cast<RenderingComponent*>(getComponent(SGNComponent::ComponentType::RENDERING));
}

};  // namespace Divide
#endif
