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
        setState(ResourceState::RES_SPECIAL);
        _boundingBox.set(VECTOR3_UNIT, -VECTOR3_UNIT);

    }

    bool onRender(SceneGraphNode& sgn, RenderStage currentStage) {
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

    bool onRender(RenderStage currentStage) { return true; }

    void postLoad(SceneGraphNode& sgn) { return; }
    bool unload() { return true; }
    bool load(const stringImpl& name) { return true; }
};


typedef std::shared_ptr<SceneGraphNode> SceneGraphNode_ptr;
typedef std::weak_ptr<SceneGraphNode> SceneGraphNode_wptr;
typedef std::shared_ptr<const SceneGraphNode> SceneGraphNode_cptr;
typedef std::weak_ptr<const SceneGraphNode> SceneGraphNode_cwptr;

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
    inline T* getNode() const {
        static_assert(
            std::is_base_of<SceneNode, T>::value,
            "SceneGraphNode::getNode error: Invalid target node type!");
        assert(_node != nullptr);
        return static_cast<T*>(_node);
    }
    /// Add node increments the node's ref counter if the node was already added
    /// to the scene graph
    SceneGraphNode_ptr addNode(SceneNode& node, U32 componentMask, const stringImpl& name = "");
    SceneGraphNode_ptr registerNode(SceneGraphNode_ptr node);
    void removeNode(const stringImpl& nodeName, bool recursive = true);
    inline void removeNode(const SceneGraphNode& node, bool recursive = true) {
        removeNode(node.getName(), recursive);
    }    
    
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

    /*Parent <-> Children*/
    void useDefaultTransform(const bool state);

    /*Node State*/
    void setActive(const bool state);
    inline bool isActive() const { return _active; }

    inline const UsageContext& usageContext() const { return _usageContext; }
    void usageContext(const UsageContext& newContext);

    inline U64 getElapsedTime() const { return _elapsedTime; }

    template <typename T>
    inline T* get() const {
        assert(false && "INVALID COMPONENT");
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
        assert(idx < _childCount);
        updatedChildCount = getChildCount();
        SceneGraphNode_ptr child = _children.at(idx);
        assert(child);
        return *child;
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

    inline bool getFlag(UpdateFlag flag) const {
        return _updateFlags[to_uint(flag)];
    }

    inline void clearUpdateFlag(UpdateFlag flag) {
        _updateFlags[to_uint(flag)] = false;
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
    explicit SceneGraphNode(SceneGraph& sceneGraph,
                            SceneNode& node,
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

    inline I8 getComponentIdx(SGNComponent::ComponentType cmpType) const {
        for (std::pair<I8, SGNComponent::ComponentType> entry : _componentIdx) {
            if (cmpType == entry.second) {
                return entry.first;
            }
        }
        // should be easy to debug
        return -1;
    }

    inline void setComponentIdx(SGNComponent::ComponentType cmpType, I8 newIdx) {
        if (getComponentIdx(cmpType) == -1) {
            _componentIdx.push_back(std::make_pair(newIdx, cmpType));
        } else {
            for (std::pair<U8, SGNComponent::ComponentType> entry : _componentIdx) {
                if (cmpType == entry.second) {
                    entry.first = newIdx;
                    return;
                }
            }
        }
    }

   private:
    // An SGN doesn't exist outside of a scene graph
    SceneGraph& _sceneGraph;

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
    std::array<std::atomic<bool>, to_const_uint(UpdateFlag::COUNT)> _updateFlags;

    bool _isSelectable;
    SelectionFlag _selectionFlag;

    UsageContext _usageContext;

    StateTracker<bool> _trackedBools;

    DELEGATE_CBK_PARAM<SceneGraphNode_cptr> _collisionCbk;

    vectorImpl<std::unique_ptr<SGNComponent>> _components;
    // keeping a separate index list should be faster than checking each component for its type
    vectorImpl<std::pair<I8, SGNComponent::ComponentType>> _componentIdx;
};

template <>
inline AnimationComponent* SceneGraphNode::get() const {
    I8 idx = getComponentIdx(SGNComponent::ComponentType::ANIMATION);
	return idx == -1 ? nullptr : static_cast<AnimationComponent*>(_components[idx].get());
}
template <>
inline IKComponent* SceneGraphNode::get() const {
    I8 idx = getComponentIdx(SGNComponent::ComponentType::INVERSE_KINEMATICS);
    return idx == -1 ? nullptr : static_cast<IKComponent*>(_components[idx].get());
}
template <>
inline RagdollComponent* SceneGraphNode::get() const {
    I8 idx = getComponentIdx(SGNComponent::ComponentType::RAGDOLL);
    return idx == -1 ? nullptr : static_cast<RagdollComponent*>(_components[idx].get());
}
template <>
inline BoundsComponent* SceneGraphNode::get() const {
    I8 idx = getComponentIdx(SGNComponent::ComponentType::BOUNDS);
    return idx == -1 ? nullptr : static_cast<BoundsComponent*>(_components[idx].get());
}
template <>
inline NavigationComponent* SceneGraphNode::get() const {
    I8 idx = getComponentIdx(SGNComponent::ComponentType::NAVIGATION);
    return idx == -1 ? nullptr : static_cast<NavigationComponent*>(_components[idx].get());
}
template <>
inline PhysicsComponent* SceneGraphNode::get() const {
    I8 idx = getComponentIdx(SGNComponent::ComponentType::PHYSICS);
    return idx == -1 ? nullptr : static_cast<PhysicsComponent*>(_components[idx].get());
}
template <>
inline RenderingComponent* SceneGraphNode::get() const {
    I8 idx = getComponentIdx(SGNComponent::ComponentType::RENDERING);
    return idx == -1 ? nullptr : static_cast<RenderingComponent*>(_components[idx].get());
}

};  // namespace Divide
#endif
