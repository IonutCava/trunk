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

#define SCENE_GRAPH_PROCESS(pointer) \
    DELEGATE_BIND(&SceneGraph::process(), pointer)

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
    SceneRoot() : SceneNode("root", SceneNodeType::TYPE_ROOT) {
        _renderState.useDefaultMaterial(false);
        setState(ResourceState::RES_LOADED);
    }

    bool onDraw(SceneGraphNode& sgn, const RenderStage& currentStage) {
        return true;
    }
    bool unload() { return true; }
    bool load(const stringImpl& name) { return true; }
    bool computeBoundingBox(SceneGraphNode& sgn);

   protected:
    void render(SceneGraphNode& sgn,
                const SceneRenderState& sceneRenderState,
                const RenderStage& currentRenderStage) {}
    void postLoad(SceneGraphNode& sgn) { SceneNode::postLoad(sgn); }
    void getDrawCommands(SceneGraphNode& sgn,
                         const RenderStage& renderStage,
                         SceneRenderState& sceneRenderState,
                         vectorImpl<GenericDrawCommand>& drawCommandsOut) {}
};

// Add as many SceneTransform nodes are needed as parent nodes for any scenenode
// to create complex transforms in the scene
class SceneTransform : public SceneNode {
   public:
    SceneTransform() : SceneNode(SceneNodeType::TYPE_TRANSFORM) {
        _renderState.useDefaultMaterial(false);
        setState(ResourceState::RES_LOADED);
    }

    void render(SceneGraphNode& sgn,
                const SceneRenderState& sceneRenderState,
                const RenderStage& currentRenderStage) {}
    bool onDraw(const RenderStage& currentStage) { return true; }

    void postLoad(SceneGraphNode& sgn) { return; }
    bool unload() { return true; }
    bool load(const stringImpl& name) { return true; }
    bool computeBoundingBox(SceneGraphNode& sgn) { return true; }
};

class IMPrimitive;
class SceneGraphNode : public GUIDWrapper, private NonCopyable {
   public:
    typedef hashMapImpl<stringImpl, SceneGraphNode*> NodeChildren;

    /// Usage context affects lighting, navigation, physics, etc
    enum class UsageContext : U32 {
        NODE_DYNAMIC = 0,
        NODE_STATIC 
    };

    bool unload();
    /// Update bounding boxes
    void checkBoundingBoxes();
    /// Apply current transform to the node's BB. Return true if the bounding
    /// extents changed
    bool updateBoundingBoxTransform(const mat4<F32>& transform);
    /// Called from SceneGraph "sceneUpdate"
    void sceneUpdate(const U64 deltaTime, SceneState& sceneState);
    /// Called when the camera updates the view matrix and/or the projection
    /// matrix
    void onCameraChange();
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
    /// Create node never increments the node's ref counter (used for scene
    /// loading)
    SceneGraphNode& createNode(SceneNode* const node,
                               const stringImpl& name = "");
    /// Add node increments the node's ref counter if the node was already added
    /// to the scene graph
    SceneGraphNode& addNode(SceneNode* const node, const stringImpl& name = "");
    void removeNode(SceneGraphNode* node);
    inline void deleteNode(SceneGraphNode*& node) {
        removeNode(node);
        MemoryManager::DELETE(node);
    }
    /// Find a node in the graph based on the SceneGraphNode's name
    /// If sceneNodeName = true, find a node in the graph based on the
    /// SceneNode's name
    SceneGraphNode* findNode(const stringImpl& name,
                             bool sceneNodeName = false);
    /// Find the graph nodes whom's bounding boxes intersects the given ray
    void intersect(const Ray& ray, F32 start, F32 end,
                   vectorImpl<SceneGraphNode*>& selectionHits);

    /// Selection helper functions
    void setSelected(const bool state);
    inline bool isSelected() const { return _selected; }
    inline bool isSelectable() const { return _isSelectable; }
    inline void setSelectable(const bool state) { _isSelectable = state; }

    const stringImpl& getName() const { return _name; }
    /*Node Management*/

    /*Parent <-> Children*/
    inline SceneGraphNode* getParent() const { return _parent; }
    inline NodeChildren& getChildren() { return _children; }
    void setParent(SceneGraphNode& parent);

    /*Parent <-> Children*/

    /*Bounding Box Management*/
    void setInitialBoundingBox(const BoundingBox& initialBoundingBox);

    inline BoundingBox& getBoundingBox() { return _boundingBox; }
    inline const BoundingBox& getBoundingBoxConst() const {
        return _boundingBox;
    }
    inline BoundingSphere& getBoundingSphere() { return _boundingSphere; }
    inline const BoundingSphere& getBoundingSphereConst() const {
        return _boundingSphere;
    }
    inline const BoundingBox& getInitialBoundingBox() const {
        return _initialBoundingBox;
    }

    void getBBoxes(vectorImpl<BoundingBox>& boxes) const;
    /*Bounding Box Management*/

    inline void silentDispose(const bool state) { _silentDispose = state; }
    inline void useDefaultTransform(const bool state) {
        getComponent<PhysicsComponent>()->useDefaultTransform(!state);
    }

    /*Node State*/
    inline void setActive(const bool state) {
        _wasActive = _active;
        _active = state;
    }
    inline void restoreActive() { _active = _wasActive; }
    inline void scheduleDeletion() { _shouldDelete = true; }

    inline bool inView() const { return _inView; }
    inline bool isActive() const { return _active; }

    inline U32 getInstanceID() const { return _instanceID; }
    inline U32 getChildQueue() const { return _childQueue; }
    inline void incChildQueue() { _childQueue++; }
    inline void decChildQueue() { _childQueue--; }

    inline const UsageContext& usageContext() const { return _usageContext; }
    inline void usageContext(const UsageContext& newContext) {
        _usageContext = newContext;
    }

    void addBoundingBox(const BoundingBox& bb, const SceneNodeType& type);
    void setBBExclusionMask(U32 bbExclusionMask) {
        _bbAddExclusionList = bbExclusionMask;
    }

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
    template <>
    inline AnimationComponent* getComponent() const {
        return static_cast<AnimationComponent*>(
            _components[to_uint(SGNComponent::ComponentType::SGN_COMP_ANIMATION)].get());
    }
    template <>
    inline NavigationComponent* getComponent() const {
        return static_cast<NavigationComponent*>(
            _components[to_uint(SGNComponent::ComponentType::SGN_COMP_NAVIGATION)].get());
    }
    template <>
    inline PhysicsComponent* getComponent() const {
        return static_cast<PhysicsComponent*>(
            _components[to_uint(SGNComponent::ComponentType::SGN_COMP_PHYSICS)].get());
    }
    template <>
    inline RenderingComponent* getComponent() const {
        return static_cast<RenderingComponent*>(
            _components[to_uint(SGNComponent::ComponentType::SGN_COMP_RENDERING)].get());
    }

    inline StateTracker<bool>& getTrackedBools() { return _trackedBools; }

    inline void registerDeletionCallback(const DELEGATE_CBK<>& cbk) {
        _deletionCallbacks.push_back(cbk);
    }

    bool operator==(const SceneGraphNode& other) const {
        return this->getGUID() == other.getGUID();
    }

    bool operator!=(const SceneGraphNode& other) const {
        return this->getGUID() != other.getGUID();
    }

   protected:
    SET_DELETE_FRIEND
    SET_SAFE_UPDATE_FRIEND
    SET_DELETE_VECTOR_FRIEND
    SET_DELETE_HASHMAP_FRIEND
    SET_UNIQUE_PTR_DELETE_FRIEND(SceneGraphNode)

    friend class SceneGraph;
    friend vectorImpl<SceneGraphNode>;
    explicit SceneGraphNode(SceneNode* const node, const stringImpl& name);
    ~SceneGraphNode();

   protected:
    friend class RenderPassCuller;
    void inView(const bool state);

   protected:
    friend class RenderingComponent;
    bool prepareDraw(const SceneRenderState& sceneRenderState,
                     const RenderStage& currentRenderStage);

   private:
    inline void setName(const stringImpl& name) { _name = name; }
    inline void scheduleReset(RenderStage currentStage) {
        _reset[currentStage] = true;
    }

   private:
    SceneNode* _node;
    NodeChildren _children;
    SceneGraphNode* _parent;
    std::atomic<bool> _active;
    std::atomic<bool> _loaded;
    std::atomic<bool> _inView;
    std::atomic<bool> _boundingBoxDirty;
    U32 _bbAddExclusionList;
    bool _selected;
    bool _isSelectable;
    bool _wasActive;
    bool _sorted;
    bool _silentDispose;
    bool _shouldDelete;
    ///_initialBoundingBox is a copy of the initialy calculate BB for
    ///transformation
    /// it should be copied in every computeBoungingBox call;
    BoundingBox _initialBoundingBox, _initialBoundingBoxCache;
    BoundingBox _boundingBox;
    BoundingSphere _boundingSphere;  ///<For faster visibility culling

    U32 _instanceID;
    U32 _childQueue;
    D32 _updateTimer;
    U64 _elapsedTime;  //< the total amount of time that passed since this node
                       //was created
    stringImpl _name;

    UsageContext _usageContext;
    std::unique_ptr<SGNComponent> _components[to_const_uint(
        SGNComponent::ComponentType::COUNT)];
    vectorImpl<DELEGATE_CBK<>> _deletionCallbacks;
    hashMapImpl<RenderStage, bool, hashAlg::hash<RenderStage>> _reset;

    StateTracker<bool> _trackedBools;
};
};  // namespace Divide
#endif