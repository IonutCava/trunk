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

#ifndef _SCENE_GRAPH_NODE_H_
#define _SCENE_GRAPH_NODE_H_

#include "SceneNode.h"
#include "SGNRelationshipCache.h"
#include "Utility/Headers/StateTracker.h"
#include "ECS/Components/Headers/IKComponent.h"
#include "ECS/Components/Headers/UnitComponent.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/RagdollComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "ECS/Components/Headers/AnimationComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/RigidBodyComponent.h"
#include "ECS/Components/Headers/NavigationComponent.h"
#include "ECS/Components/Headers/NetworkingComponent.h"

namespace Divide {

class SceneGraph;
class SceneState;
struct TransformDirty;
struct TransformClean;

// This is the scene root node. All scene node's are added to it as child nodes
class SceneRoot : public SceneNode {
   public:
    SceneRoot(ResourceCache& parentCache, size_t descriptorHash)
        : SceneNode(parentCache, descriptorHash, "root", SceneNodeType::TYPE_ROOT)
    {
        _renderState.useDefaultMaterial(false);
        setState(ResourceState::RES_LOADED);
        _boundingBox.set(VECTOR3_UNIT, -VECTOR3_UNIT);

    }
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(SceneRoot);
// Add as many SceneTransform nodes are needed as parent nodes for any scenenode
// to create complex transforms in the scene
class SceneTransform : public SceneNode {
   public:
    SceneTransform(ResourceCache& parentCache, size_t descriptorHash, vec3<F32> extents)
        : SceneNode(parentCache, descriptorHash, "TransformNode", SceneNodeType::TYPE_TRANSFORM),
          _extents(extents)
    {
        _renderState.useDefaultMaterial(false);
        setState(ResourceState::RES_LOADED);
    }

    inline void updateBoundsInternal(SceneGraphNode& sgn) override {
        _boundingBox.setMin(-_extents * 0.5f);
        _boundingBox.setMax( _extents * 0.5f);
    }

  protected:
    vec3<F32> _extents;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(SceneTransform);

class SceneGraphNode : public ECS::Entity<SceneGraphNode>,
                       protected ECS::Event::IEventListener,
                       public GUIDWrapper,
                       private NonCopyable
{
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
        THREADED_LOAD = 1,
        COUNT
    };

    /// Called from SceneGraph "sceneUpdate"
    void sceneUpdate(const U64 deltaTimeUS, SceneState& sceneState);
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

    template<class... ARGS>
    static SceneGraphNode* CreateSceneGraphNode(ARGS&&... args) {
        ECS::EntityId nodeID = GetEntityManager()->CreateEntity<SceneGraphNode>(std::forward<ARGS>(args)...);
        return static_cast<SceneGraphNode*>(GetEntityManager()->GetEntity(nodeID));
    }

    static void DestroySceneGraphNode(SceneGraphNode*& node, bool inPlace = true) {
        if (node) {
            if (inPlace) {
                GetEntityManager()->DestroyAndRemoveEntity(node->GetEntityID());
            } else {
                GetEntityManager()->DestroyEntity(node->GetEntityID());
            }
            node = nullptr;
        }
    }

    inline static ECS::EntityManager* GetEntityManager() {
        return ECS::ECS_Engine->GetEntityManager();
    }

    inline static ECS::ComponentManager* GetComponentManager() {
        return ECS::ECS_Engine->GetComponentManager();
    }
    
    template<class E, class... ARGS>
    static void SendEvent(ARGS&&... eventArgs)
    {
        ECS::ECS_Engine->SendEvent<E>(std::forward<ARGS>(eventArgs)...);
    }
    /// Add node increments the node's ref counter if the node was already added
    /// to the scene graph
    SceneGraphNode* addNode(const SceneNode_ptr& node, U32 componentMask, PhysicsGroup physicsGroup, const stringImpl& name = "");
    SceneGraphNode* registerNode(SceneGraphNode* node);

    /// If this function returns true, the node will no longer be part of the scene hierarchy.
    /// If the node is not a child of the calling node, we will recursively look in all of its children for a match
    bool removeNode(const SceneGraphNode& node);
    /// If this function returns true, at least one node of the specified type was removed.
    bool removeNodesByType(SceneNodeType nodeType);

    /// Find a child Node using the given name (either SGN name or SceneNode name)
    SceneGraphNode* findChild(const stringImpl& name, bool sceneNodeName = false, bool recursive = false) const;
    /// Find a child using the give GUID
    SceneGraphNode* findChild(I64 GUID, bool recursive = false) const;

    /// Find the graph nodes whom's bounding boxes intersects the given ray
    void intersect(const Ray& ray, F32 start, F32 end,
                   vectorImpl<I64>& selectionHits,
                   bool recursive = true) const;

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
    inline SceneGraphNode* getParent() const {
        return _parent;
    }

    void setParent(SceneGraphNode& parent);

    /*Node State*/
    void setActive(const bool state);
    inline bool isActive() const { return _active; }

    inline const UsageContext& usageContext() const { return _usageContext; }
    void usageContext(const UsageContext& newContext);

    inline U64 getElapsedTimeUS() const { return _elapsedTimeUS; }

    template <typename T>
    inline T* get() const {
        // ToDo: Optimise this -Ionut
        return GetComponentManager()->GetComponent<T>(GetEntityID());
    }

    inline StateTracker<bool>& getTrackedBools() { return _trackedBools; }

    bool isChildOfType(U32 typeMask, bool ignoreRoot) const;
    bool isRelated(const SceneGraphNode& target) const;
    bool isChild(const SceneGraphNode& target, bool recursive) const;

    void forEachChild(const DELEGATE_CBK<void, SceneGraphNode&>& callback);
    void forEachChild(const DELEGATE_CBK<void, const SceneGraphNode&>& callback) const;

    //Returns false if the loop was interrupted
    bool forEachChildInterruptible(const DELEGATE_CBK<bool, SceneGraphNode&>& callback);
    //Returns false if the loop was interrupted
    bool forEachChildInterruptible(const DELEGATE_CBK<bool, const SceneGraphNode&>& callback) const;

    void forEachChild(const DELEGATE_CBK<void, SceneGraphNode&, I32>& callback, U32 start, U32 end);
    void forEachChild(const DELEGATE_CBK<void, const SceneGraphNode&, I32>& callback, U32 start, U32 end) const;
    //Returns false if the loop was interrupted
    bool forEachChildInterruptible(const DELEGATE_CBK<bool, SceneGraphNode&, I32>& callback, U32 start, U32 end);
    //Returns false if the loop was interrupted
    bool forEachChildInterruptible(const DELEGATE_CBK<bool, const SceneGraphNode&, I32>& callback, U32 start, U32 end) const;

    inline bool hasChildren() const {
        return getChildCount() > 0;
    }

    inline SceneGraphNode& getChild(U32 idx) {
        ReadLock r_lock(_childLock);
        assert(idx <  getChildCountLocked());
        return *_children.at(idx);
    }

    inline const SceneGraphNode& getChild(U32 idx) const {
        ReadLock r_lock(_childLock);
        assert(idx <  getChildCountLocked());
        return *_children.at(idx);
    }

    inline U32 getChildCount() const {
        ReadLock r_lock(_childLock);
        return getChildCountLocked();
    }

    inline U32 getChildCountLocked() const {
        return to_U32(_children.size());
    }

    inline bool getFlag(UpdateFlag flag) const {
        return _updateFlags[to_U32(flag)];
    }

    inline void clearUpdateFlag(UpdateFlag flag) {
        _updateFlags[to_U32(flag)] = false;
    }

    bool operator==(SceneGraphNode* other) const {
        return this->getGUID() == other->getGUID();
    }

    bool operator!=(SceneGraphNode* other) const {
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
    bool prepareRender(const SceneRenderState& sceneRenderState,
                     const RenderStagePass& renderStagePass);

   protected:
    friend class SceneGraph;
    void onCameraUpdate(const I64 cameraGUID,
                        const vec3<F32>& posOffset,
                        const mat4<F32>& rotationOffset);

    void onCameraChange(const Camera& cam);

    void onNetworkSend(U32 frameCount);

    inline void setUpdateFlag(UpdateFlag flag) {
        _updateFlags[to_U32(flag)] = true;
    }

    void getOrderedNodeList(vectorImpl<SceneGraphNode*>& nodeList);

    void processDeleteQueue();

   private:
    void addToDeleteQueue(U32 idx);

    inline void setName(const stringImpl& name) { 
        _name = name;
    }

    void RegisterEventCallbacks();
    void OnTransformDirty(const TransformDirty* event);
    void OnTransformClean(const TransformClean* event);

   private:
    friend class SGNRelationshipCache;
    inline const SGNRelationshipCache& relationshipCache() const {
        return _relationshipCache;
    }
    void invalidateRelationshipCache();

   private:
    // An SGN doesn't exist outside of a scene graph
    SceneGraph& _sceneGraph;

    mutable I8 _frustPlaneCache;
    U64 _elapsedTimeUS;
    U32 _componentMask;
    stringImpl _name;
    SceneNode_ptr _node;
    SceneGraphNode* _parent;
    vectorImpl<SceneGraphNode*> _children;
    mutable SharedLock _childLock;
    std::atomic<bool> _active;
    std::atomic<bool> _visibilityLocked;
    std::array<std::atomic<bool>, to_base(UpdateFlag::COUNT)> _updateFlags;

    bool _isSelectable;
    SelectionFlag _selectionFlag;

    UsageContext _usageContext;

    StateTracker<bool> _trackedBools;

    SGNRelationshipCache _relationshipCache;

    mutable SharedLock _childrenDeletionLock;
    vectorImpl<vectorAlg::vecSize> _childrenPendingDeletion;
};

};  // namespace Divide
#endif
