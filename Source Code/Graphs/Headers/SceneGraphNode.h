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
#ifndef _SCENE_GRAPH_NODE_H_
#define _SCENE_GRAPH_NODE_H_

#include "SceneNode.h"
#include "SGNRelationshipCache.h"
#include "Utility/Headers/StateTracker.h"

#include "ECS/Components/Headers/EditorComponent.h"
#include "ECS/Components/Headers/SGNComponent.h"

namespace Divide {

class SceneGraph;
class SceneState;
class PropertyWindow;
class BoundsComponent;
class TransformComponent;

struct TransformDirty;

struct SceneGraphNodeDescriptor {
    SceneNode_ptr    _node = nullptr;
    stringImpl       _name = "";
    U32              _componentMask = 0;
    NodeUsageContext _usageContext = NodeUsageContext::NODE_STATIC;
};

namespace Attorney {
    class SceneGraphNodeEditor;
    class SceneGraphNodeComponent;
};

typedef std::tuple<I64, F32/*min*/, F32/*max*/> SGNRayResult;

class SceneGraphNode : public ECS::Entity<SceneGraphNode>,
                       protected ECS::Event::IEventListener,
                       public GUIDWrapper,
                       public PlatformContextComponent
{
    static const size_t INITIAL_CHILD_COUNT = 128;

    friend class Attorney::SceneGraphNodeEditor;
    friend class Attorney::SceneGraphNodeComponent;
   public:

    enum class SelectionFlag : U8 {
        SELECTION_NONE = 0,
        SELECTION_HOVER,
        SELECTION_SELECTED,
        SELECTION_COUNT
    };

    enum class UpdateFlag : U8 {
        SPATIAL_PARTITION = toBit(1),
        THREADED_LOAD = toBit(2),
        COUNT = 2
    };

    /// Called from SceneGraph "sceneUpdate"
    void sceneUpdate(const U64 deltaTimeUS, SceneState& sceneState);
    void frameStarted();
    void frameEnded();
    /*Node Management*/
    /// Always use the level of redirection needed to reduce virtual function overhead
    /// Use getNode<SceneNode> if you need material properties for ex. or getNode<SubMesh> for animation transforms
    template <typename T = SceneNode>
    inline std::shared_ptr<T> getNode() const {
        static_assert(std::is_base_of<SceneNode, T>::value, "SceneGraphNode::getNode error: Invalid target node type!");
        return std::static_pointer_cast<T>(_node);
    }

    ECS::ECSEngine& GetECSEngine();
    const ECS::ECSEngine& GetECSEngine() const;
    ECS::EntityManager* GetEntityManager();
    ECS::EntityManager* GetEntityManager() const;
    ECS::ComponentManager* GetComponentManager();
    ECS::ComponentManager* GetComponentManager() const;

    template<class E, class... ARGS>
    void SendEvent(ARGS&&... eventArgs)
    {
        GetECSEngine().SendEvent<E>(std::forward<ARGS>(eventArgs)...);
    }

    template<class E, class... ARGS>
    void SendAndDispatchEvent(ARGS&&... eventArgs)
    {
        GetECSEngine().SendEventAndDispatch<E>(std::forward<ARGS>(eventArgs)...);
    }
    /// Add node increments the node's ref counter if the node was already added
    /// to the scene graph
    SceneGraphNode* addNode(const SceneGraphNodeDescriptor& descriptor);
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
    void intersect(const Ray& ray, F32 start, F32 end, vector<SGNRayResult>& intersections) const;

    /// Selection helper functions
    void setSelectionFlag(SelectionFlag flag);
    inline SelectionFlag getSelectionFlag() const { return _selectionFlag; }

    void lockVisibility(const bool state);
    inline bool visibilityLocked() const { return _visibilityLocked; }

    const stringImpl& name() const { return _name; }
    /*Node Management*/

    /*Parent <-> Children*/
    inline SceneGraphNode* getParent() const {
        return _parent;
    }

    void setParent(SceneGraphNode& parent);

    /*Node State*/
    void setActive(const bool state);
    inline bool isActive() const { return _active; }

    inline const NodeUsageContext& usageContext() const { return _usageContext; }
    void usageContext(const NodeUsageContext& newContext);

    inline U64 getElapsedTimeUS() const { return _elapsedTimeUS; }

    template <typename T>
    inline T* get() const {
        // ToDo: Optimise this -Ionut
        return GetComponentManager()->GetComponent<T>(GetEntityID());
    }

    inline void lockToCamera(U64 cameraNameHash) { _lockToCamera = cameraNameHash; }

    inline U32 componentMask() const { return _componentMask; }

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
        SharedLock r_lock(_childLock);
        assert(idx <  getChildCountLocked());
        return *_children.at(idx);
    }

    inline const SceneGraphNode& getChild(U32 idx) const {
        SharedLock r_lock(_childLock);
        assert(idx <  getChildCountLocked());
        return *_children.at(idx);
    }

    inline U32 getChildCount() const {
        SharedLock r_lock(_childLock);
        return getChildCountLocked();
    }

    inline U32 getChildCountLocked() const {
        return to_U32(_children.size());
    }

    inline bool getFlag(UpdateFlag flag) const {
        return BitCompare(_updateFlags, to_U32(flag));
    }

    inline void clearUpdateFlag(UpdateFlag flag) {
        ClearBit(_updateFlags, to_base(flag));
    }

   /*protected:
    SET_DELETE_FRIEND
    SET_SAFE_UPDATE_FRIEND
    SET_DELETE_VECTOR_FRIEND
    SET_DELETE_HASHMAP_FRIEND

    friend class SceneGraph;
    friend class std::shared_ptr<SceneGraphNode> ;*/
    explicit SceneGraphNode(SceneGraph& sceneGraph, const SceneGraphNodeDescriptor& descriptor);
    ~SceneGraphNode();

    void postLoad();

    inline SceneGraph& parentGraph() {
        return _sceneGraph;
    }

    inline const SceneGraph& parentGraph() const {
        return _sceneGraph;
    }

    void saveToXML(const stringImpl& sceneLocation) const;
    void loadFromXML(const boost::property_tree::ptree& pt);

   protected:
    friend class RenderPassCuller;
    // Returns true if the node should be culled (is not visible for the current stage)
    bool cullNode(const Camera& currentCamera,
                  F32 maxDistanceFromCameraSq,
                  RenderStage currentStage,
                  Frustum::FrustCollision& collisionTypeOut,
                  F32& minDistanceSq) const;

   protected:
    friend class RenderingComponent;
    bool prepareRender(const Camera& camera, RenderStagePass renderStagePass);
    void onRefreshNodeData(GFX::CommandBuffer& bufferInOut);
   protected:
    friend class SceneGraph;
    void onCameraUpdate(const I64 cameraGUID,
                        const vec3<F32>& cameraEye,
                        const mat4<F32>& cameraView);

    void onNetworkSend(U32 frameCount);

    inline void setUpdateFlag(UpdateFlag flag) {
        SetBit(_updateFlags, to_base(flag));
    }

    void getOrderedNodeList(vector<SceneGraphNode*>& nodeList);

    void processDeleteQueue(vector<vec_size>& childList);

    bool saveCache(ByteBuffer& outputBuffer) const;
    bool loadCache(ByteBuffer& inputBuffer);

   protected:
    void setTransformDirty(U32 transformMask);
    void setParentTransformDirty(U32 transformMask);
    void onBoundsUpdated();
    void postLoad(SceneNode& sceneNode, SceneGraphNode& sgn);

   private:
    void addToDeleteQueue(U32 idx);

    inline void name(const stringImpl& name) { 
        _name = name;
    }

    void RegisterEventCallbacks();


   public:
    template<class T, class ...P>
    T* AddSGNComponent(P&&... param) {
        SGNComponent* comp = static_cast<SGNComponent*>(AddComponent<T>(*this, this->context(), std::forward<P>(param)...));
        _editorComponents.emplace_back(&comp->getEditorComponent());
        SetBit(_componentMask, comp->type());

        return static_cast<T*>(comp);
    }

    template<class T>
    void RemoveSGNComponent() {
        SGNComponent* comp = static_cast<SGNComponent>(GetComponent<T>());
        if (comp) {
            _editorComponents.erase(
                std::remove_if(std::begin(_editorComponents), std::end(_editorComponents),
                               [comp](EditorComponent* editorComp)
                               -> bool { return comp->getEditorComponent().getGUID() == editorComp->getGUID(); }),
                std::end(_editorComponents));
            ClearBit(_componentMask, comp->type());
            RemoveComponent<T>();
        }
    }

    void RemoveAllSGNComponents() {
        GetComponentManager()->RemoveAllComponents(GetEntityID());
        _editorComponents.clear();
        _componentMask = 0;
    }

    void AddMissingComponents(U32 componentMask);

   private:
    friend class SGNRelationshipCache;
    inline const SGNRelationshipCache& relationshipCache() const {
        return _relationshipCache;
    }
    void invalidateRelationshipCache();

   private:
    // An SGN doesn't exist outside of a scene graph
    SceneGraph& _sceneGraph;

    U64 _lockToCamera = 0;
    //ToDo: make this work in a multi-threaded environment
    //mutable I8 _frustPlaneCache;
    U64 _elapsedTimeUS;
    U32 _componentMask;
    stringImpl _name;
    SceneNode_ptr _node;
    SceneGraphNode* _parent;
    vectorEASTL<SceneGraphNode*> _children;
    mutable SharedMutex _childLock;
    std::atomic_bool _active;
    std::atomic_bool _visibilityLocked;
    std::atomic_uint _updateFlags;

    SelectionFlag _selectionFlag;

    NodeUsageContext _usageContext;

    StateTracker<bool> _trackedBools;

    SGNRelationshipCache _relationshipCache;

    // ToDo: Remove this HORRIBLE hack -Ionut
    vectorFast<EditorComponent*> _editorComponents;
};

template<typename T, class ... Args>
void AddSGNComponent(SceneGraphNode& parentSGN, Args&&... args) {
    parentSGN.AddSGNComponent<T>(std::forward<Args>(args)...);
}

namespace Attorney {
    class SceneGraphNodeEditor {
        private:
        static vectorFast<EditorComponent*>& editorComponents(SceneGraphNode& node) {
            return node._editorComponents;
        }

        static const vectorFast<EditorComponent*>& editorComponents(const SceneGraphNode& node) {
            return node._editorComponents;
        }

        friend class Divide::PropertyWindow;
    };

    class SceneGraphNodeComponent {
    private:
        static void setTransformDirty(SceneGraphNode& node, U32 transformMask) {
            node.setTransformDirty(transformMask);
        }

        static void onBoundsUpdated(SceneGraphNode& node) {
            node.onBoundsUpdated();
        }

        friend class Divide::BoundsComponent;
        friend class Divide::RenderingComponent;
        friend class Divide::TransformComponent;
    };

};  // namespace Attorney


};  // namespace Divide
#endif
