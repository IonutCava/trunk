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

#ifndef _SCENE_GRAPH_NODE_H_
#define _SCENE_GRAPH_NODE_H_

#include "SceneNode.h"
#include "Utility/Headers/StateTracker.h"
#include "Graphs/Components/Headers/SGNComponent.h"
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
        setState(ResourceState::RES_SPECIAL);
        _boundingBox.first.reset();
        _boundingBox.second = false;
    }

    bool onDraw(SceneGraphNode& sgn, RenderStage currentStage) {
        return true;
    }

    bool unload() { return true; }
    bool load(const stringImpl& name) {
        return true; 
    }

   protected:
    friend class SceneGraph;
    void postLoad(SceneGraphNode& sgn) { SceneNode::postLoad(sgn); }
};

// Add as many SceneTransform nodes are needed as parent nodes for any scenenode
// to create complex transforms in the scene
class SceneTransform : public SceneNode {
   public:
    SceneTransform() : SceneNode(SceneNodeType::TYPE_TRANSFORM) {
        _renderState.useDefaultMaterial(false);
        setState(ResourceState::RES_SPECIAL);
    }

    bool onDraw(RenderStage currentStage) { return true; }

    void postLoad(SceneGraphNode& sgn) { return; }
    bool unload() { return true; }
    bool load(const stringImpl& name) { return true; }
};


typedef std::shared_ptr<SceneGraphNode> SceneGraphNode_ptr;
typedef std::weak_ptr<SceneGraphNode> SceneGraphNode_wptr;

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

    /// Called from SceneGraph "sceneUpdate"
    void sceneUpdate(const U64 deltaTime, SceneState& sceneState);
    /*Node Management*/
    /// Always use the level of redirection needed to reduce virtual function
    /// overhead
    /// Use getNode<SceneNode> if you need material properties for ex. or
    /// getNode<SubMesh> for animation transforms
    template <typename T = SceneNode>
    inline T* getNode() const {
        static_assert(
            std::is_base_of<SceneNode, T>::value,
            "SceneGraphNode::getNode error: Invalid target node type!");
        assert(_node != nullptr);
        return static_cast<T*>(_node);
    }
    /// Add node increments the node's ref counter if the node was already added
    /// to the scene graph
    SceneGraphNode_ptr addNode(SceneNode& node, const stringImpl& name = "");
    SceneGraphNode_ptr addNode(SceneGraphNode_ptr node);
    void removeNode(const stringImpl& nodeName, bool recursive = true);
    inline void removeNode(const SceneGraphNode& node, bool recursive = true) {
        removeNode(node.getName(), recursive);
    }    
    
    /// Find a node in the graph based on the SceneGraphNode's name
    /// If sceneNodeName = true, find a node in the graph based on the
    /// SceneNode's name
    SceneGraphNode_wptr findNode(const stringImpl& name, bool sceneNodeName = false);
    /// Find a child Node using the given name (either SGN name or SceneNode name)
    SceneGraphNode_wptr findChild(const stringImpl& name, bool sceneNodeName = false);
    /// Find the graph nodes whom's bounding boxes intersects the given ray
    void intersect(const Ray& ray, F32 start, F32 end,
                   vectorImpl<SceneGraphNode_wptr>& selectionHits, bool recursive = true);

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

    /*Parent <-> Children*/

    /*Bounding Box Management*/
    inline BoundingBox& getBoundingBox() { return _boundingBox; }
    inline const BoundingBox& getBoundingBoxConst() const { return _boundingBox;  }
    inline BoundingSphere& getBoundingSphere() { return _boundingSphere; }
    inline const BoundingSphere& getBoundingSphereConst() const { return _boundingSphere;  }
    void computeBoundingBox();
    /*Bounding Box Management*/

    void useDefaultTransform(const bool state);

    /*Node State*/
    void setActive(const bool state);
    inline bool isActive() const { return _active; }

    inline const UsageContext& usageContext() const { return _usageContext; }
    void usageContext(const UsageContext& newContext);

    inline bool lockBBTransforms() const { return _lockBBTransforms; }

    inline void lockBBTransforms(const bool state) { _lockBBTransforms = state; }

    inline U64 getElapsedTime() const { return _elapsedTime; }

    inline void setComponent(SGNComponent::ComponentType type,
                             SGNComponent* component) {
        _components[to_uint(type)].reset(component);
    }

    template <typename T>
    inline T* getComponent() const {
        assert(false && "INVALID COMPONENT");
        return nullptr;
    }

    inline StateTracker<bool>& getTrackedBools() { return _trackedBools; }

    inline bool hasChildren() const {
        return getChildCount() > 0;
    }

    inline SceneGraphNode& getChild(U32 idx, U32& updatedChildCount) {
        assert(idx < _childCount);
        updatedChildCount = getChildCount();
        assert(_children.at(idx));
        return *_children.at(idx);
    }

    inline const SceneGraphNode& getChild(U32 idx, U32& updatedChildCount) const {
        assert(idx < _childCount);
        updatedChildCount = getChildCount();
        assert(_children.at(idx));
        return *_children.at(idx);
    }

    inline U32 getChildCount() const {
        return _childCount;
    }

    bool operator==(const SceneGraphNode_ptr other) const {
        return this->getGUID() == other->getGUID();
    }

    bool operator!=(const SceneGraphNode_ptr other) const {
        return this->getGUID() != other->getGUID();
    }

   /*protected:
    SET_DELETE_FRIEND
    SET_SAFE_UPDATE_FRIEND
    SET_DELETE_VECTOR_FRIEND
    SET_DELETE_HASHMAP_FRIEND

    friend class SceneGraph;
    friend class std::shared_ptr<SceneGraphNode> ;*/
    explicit SceneGraphNode(SceneNode& node, const stringImpl& name);
    ~SceneGraphNode();

    bool operator==(const SceneGraphNode& rhs) const {
        return getGUID() == rhs.getGUID();
    }

    bool operator!=(const SceneGraphNode& rhs) const {
        return getGUID() != rhs.getGUID();
    }

   protected:
    friend class RenderPassCuller;
    // Returns true if the node should be culled (is not visible for the current stage)
    bool cullNode(const SceneRenderState& sceneRenderState,
                  Frustum::FrustCollision& collisionType,
                  RenderStage currentStage) const;

   protected:
    friend class RenderingComponent;
    bool prepareDraw(const SceneRenderState& sceneRenderState,
                     RenderStage renderStage);
   protected:
    friend class SceneGraph;
    void frameEnded();

   private:
    inline void setName(const stringImpl& name) { _name = name; }

   private:
    D32 _updateTimer;
    U64 _elapsedTime;
    stringImpl _name;
    SceneNode* _node;
    SceneGraphNode_wptr _parent;
    vectorImpl<SceneGraphNode_ptr> _children;
    std::atomic_uint _childCount;
    mutable SharedLock _childLock;
    std::atomic<bool> _active;
    std::atomic<bool> _visibilityLocked;

    bool _isSelectable;
    SelectionFlag _selectionFlag;

    std::atomic<bool> _boundingBoxDirty;
    bool _lockBBTransforms;
    BoundingBox _boundingBox;
    BoundingBox _boundingBoxTransformedCache;
    BoundingSphere _boundingSphere;

    UsageContext _usageContext;
    std::array<std::unique_ptr<SGNComponent>, 
               to_const_uint(SGNComponent::ComponentType::COUNT)> _components;

    StateTracker<bool> _trackedBools;
};

template <>
inline AnimationComponent* SceneGraphNode::getComponent() const {
	return static_cast<AnimationComponent*>(
		_components[to_uint(SGNComponent::ComponentType::ANIMATION)].get());
}
template <>
inline NavigationComponent* SceneGraphNode::getComponent() const {
	return static_cast<NavigationComponent*>(
		_components[to_uint(SGNComponent::ComponentType::NAVIGATION)].get());
}
template <>
inline PhysicsComponent* SceneGraphNode::getComponent() const {
	return static_cast<PhysicsComponent*>(
		_components[to_uint(SGNComponent::ComponentType::PHYSICS)].get());
}
template <>
inline RenderingComponent* SceneGraphNode::getComponent() const {
	return static_cast<RenderingComponent*>(
		_components[to_uint(SGNComponent::ComponentType::RENDERING)].get());
}
};  // namespace Divide
#endif
