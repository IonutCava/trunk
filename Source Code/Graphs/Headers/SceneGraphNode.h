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

#include "ECS/Components/Headers/EditorComponent.h"
#include "ECS/Components/Headers/SGNComponent.h"

namespace Divide {

class SceneGraph;
class SceneState;
class PropertyWindow;
class BoundsComponent;
class RenderPassCuller;
class RenderingComponent;
class TransformComponent;

struct NodeCullParams;

struct SceneGraphNodeDescriptor {
    SceneNode_ptr    _node = nullptr;
    Str64            _name = "";
    size_t           _instanceCount = 1;
    U32              _componentMask = 0;
    NodeUsageContext _usageContext = NodeUsageContext::NODE_STATIC;
    bool             _serialize = true;
};

namespace Attorney {
    class SceneGraphNodeEditor;
    class SceneGraphNodeComponent;
    class SceneGraphNodeSceneGraph;
    class SceneGraphNodeRenderPassCuller;
    class SceneGraphNodeRelationshipCache;
};

struct SGNRayResult {
    I64 sgnGUI = -1;
    F32 dist = std::numeric_limits<F32>::max();
    const char* name = nullptr;
};

class SceneGraphNode final : public ECS::Entity<SceneGraphNode>,
                             private ECS::Event::IEventListener,
                             public GUIDWrapper,
                             public PlatformContextComponent
{
    friend class Attorney::SceneGraphNodeEditor;
    friend class Attorney::SceneGraphNodeComponent;
    friend class Attorney::SceneGraphNodeSceneGraph;
    friend class Attorney::SceneGraphNodeRenderPassCuller;
    friend class Attorney::SceneGraphNodeRelationshipCache;

    public:

        enum class Flags : U16 {
            SPATIAL_PARTITION_UPDATE_QUEUED = toBit(1),
            LOADING = toBit(2),
            HOVERED = toBit(3),
            SELECTED = toBit(4),
            ACTIVE = toBit(5),
            VISIBILITY_LOCKED = toBit(6),
            BOUNDING_BOX_RENDERED = toBit(7),
            SKELETON_RENDERED = toBit(8),
            COUNT = 8
        };

    public:
        /// Avoid creating SGNs directly. Use the "addChildNode" on an existing node (even root) or "addNode" on the existing SceneGraph
        explicit SceneGraphNode(SceneGraph& sceneGraph, const SceneGraphNodeDescriptor& descriptor);
        ~SceneGraphNode();

        /// Return a reference to the parent graph. Operator= should be disabled
        inline SceneGraph& sceneGraph() noexcept { return _sceneGraph; }

        /// Add child node increments the node's ref counter if the node was already added to the scene graph
        SceneGraphNode* addChildNode(const SceneGraphNodeDescriptor& descriptor);

        /// If this function returns true, the node will no longer be part of the scene hierarchy.
        /// If the node is not a child of the calling node, we will recursively look in all of its children for a match
        bool removeChildNode(const SceneGraphNode& node, bool recursive = true);

        /// Find a child Node using the given name (either SGN name or SceneNode name)
        SceneGraphNode* findChild(const Str128& name, bool sceneNodeName = false, bool recursive = false) const;

        /// Find a child using the given SGNN or SceneNode GUID
        SceneGraphNode* findChild(I64 GUID, bool sceneNodeGuid = false, bool recursive = false) const;

        /// If this function returns true, at least one node of the specified type was removed.
        bool removeNodesByType(SceneNodeType nodeType);

        /// Changing a node's parent means removing this node from the current parent's child list and appending it to the new parent's list
        void setParent(SceneGraphNode& parent);

        /// Checks if we have a parent matching the typeMask. We check recursively until we hit the top node (if ignoreRoot is false, top node is Root)
        bool isChildOfType(U16 typeMask, bool ignoreRoot) const;

        /// Returns true if the current node is related somehow to the specified target node (see RelationshipType enum for more details)
        bool isRelated(const SceneGraphNode& target) const;

        /// Returns true if the specified target node is a parent or grandparent(if recursive == true) of the current node
        bool isChild(const SceneGraphNode& target, bool recursive) const;

        /// Recursively call the delegate function on all children. Use start and end to only affect a range (useful for parallel algorithms)
        void forEachChild(DELEGATE<void, SceneGraphNode*, I32>&& callback, U32 start = 0u, U32 end = 0u);

        /// Recursively call the delegate function on all children. Use start and end to only affect a range (useful for parallel algorithms)
        void forEachChild(DELEGATE<void, const SceneGraphNode*, I32>&& callback, U32 start = 0u, U32 end = 0u) const;

        /// Recursively call the delegate function on all children. Returns false if the loop was interrupted. Use start and end to only affect a range (useful for parallel algorithms)
        bool forEachChildInterruptible(DELEGATE<bool, SceneGraphNode*, I32>&& callback, U32 start = 0u, U32 end = 0u);

        /// Recursively call the delegate function on all children. Returns false if the loop was interrupted. Use start and end to only affect a range (useful for parallel algorithms)
        bool forEachChildInterruptible(DELEGATE<bool, const SceneGraphNode*, I32>&& callback, U32 start = 0u, U32 end = 0u) const;

        /// A "locked" call assumes that either access is guaranteed thread-safe or that the child lock is already aquired
        inline const vectorEASTL<SceneGraphNode*>& getChildrenLocked() const noexcept { return _children; }

        /// A "locked" call assumes that either access is guaranteed thread-safe or that the child lock is already aquired
        inline U32 getChildCountLocked() const noexcept { return to_U32(_children.size()); }

        /// Return a specific child by indes. Does not recurse.
        inline SceneGraphNode& getChild(U32 idx) {
            SharedLock r_lock(_childLock);
            assert(idx <  getChildCountLocked());
            return *_children.at(idx);
        }

        /// Return a specific child by indes. Does not recurse.
        inline const SceneGraphNode& getChild(U32 idx) const {
            SharedLock r_lock(_childLock);
            assert(idx <  getChildCountLocked());
            return *_children.at(idx);
        }

        /// Return the current number of children that the current node has
        inline U32 getChildCount() const {
            SharedLock r_lock(_childLock);
            return getChildCountLocked();
        }

        /// Called from parent SceneGraph
        void sceneUpdate(const U64 deltaTimeUS, SceneState& sceneState);

        /// Called from parent SceneGraph
        void frameStarted();

        /// Called from parent SceneGraph
        void frameEnded();

        /// Invoked by the contained SceneNode when it finishes all of its internal loading and is ready for processing
        void postLoad();

        /// Find the graph nodes whom's bounding boxes intersects the given ray
        bool intersect(const Ray& ray, F32 start, F32 end, vector<SGNRayResult>& intersections) const;

        void changeUsageContext(const NodeUsageContext& newContext);

        /// General purpose flag management. Certain flags propagate to children (e.g. selection)!
        void setFlag(Flags flag) noexcept;
        /// Clearing a flag might propagate to child nodes (e.g. selection).
        void clearFlag(Flags flag) noexcept;
        /// Returns true only if the currrent node has the specified flag. Does not check children!
        bool hasFlag(Flags flag) const noexcept;

        /// Always use the level of redirection needed to reduce virtual function overhead.
        /// Use getNode<SceneNode> if you need material properties for ex. or getNode<SkinnedSubMesh> for animation transforms
        template <typename T = SceneNode>
        typename std::enable_if<std::is_base_of<SceneNode, T>::value, T&>::type
        getNode() { return static_cast<T&>(*_node); }

        /// Always use the level of redirection needed to reduce virtual function overhead.
        /// Use getNode<SceneNode> if you need material properties for ex. or getNode<SkinnedSubMesh> for animation transforms
        template <typename T = SceneNode>
        typename std::enable_if<std::is_base_of<SceneNode, T>::value, const T&>::type
        getNode() const { return static_cast<const T&>(*_node); }

        /// Returns a pointer to a specific component. Returns null if the SGN doesn't have the component requested
        template <typename T>
        inline T* get() const { return _compManager->GetComponent<T>(GetEntityID()); } //< ToDo: Optimise this -Ionut

        template <>
        inline TransformComponent* get() const { return Hacks._transformComponentCache; }

        void SendEvent(ECSCustomEventType eventType);

        /// Sends a global event but dispatched is handled between update steps
        template<class E, class... ARGS>
        inline void SendEvent(ARGS&&... eventArgs) { GetECSEngine().SendEvent<E>(std::forward<ARGS>(eventArgs)...); }
        /// Sends a global event with dispatched happening immediately. Avoid using often. Bad for performance.
        template<class E, class... ARGS>
        inline void SendAndDispatchEvent(ARGS&&... eventArgs) { GetECSEngine().SendEventAndDispatch<E>(std::forward<ARGS>(eventArgs)...); }

        /// Emplacement call for an ECS component. Pass in the component's constructor parameters. Can only add one component of a single type. 
        /// This may be bad for scripts, but there are workarounds
        template<class T, class ...P>
        typename std::enable_if<std::is_base_of<SGNComponent, T>::value, T*>::type
        AddSGNComponent(P&&... param) {
            SGNComponent* comp = static_cast<SGNComponent*>(AddComponent<T>(*this, this->context(), std::forward<P>(param)...));
            Hacks._editorComponents.emplace_back(&comp->getEditorComponent());
            SetBit(_componentMask, to_U32(comp->type()));
            if (comp->type()._value == ComponentType::TRANSFORM) {
                //Ewww
                Hacks._transformComponentCache = (TransformComponent*)comp;
            }
            return static_cast<T*>(comp);
        }

        /// Remove a component by type (if any). Because we have a limit of one component type per node, this works as expected
        template<class T>
        typename std::enable_if<std::is_base_of<SGNComponent, T>::value, void>::type
        RemoveSGNComponent() {
            SGNComponent* comp = static_cast<SGNComponent>(GetComponent<T>());
            if (comp) {
                I64 targetGUID = comp->getEditorComponent().getGUID();
                _editorComponents.erase(
                    std::remove_if(std::begin(_editorComponents), std::end(_editorComponents),
                        [targetGUID](EditorComponent* editorComp)
                        -> bool { return editorComp->getGUID() == targetGUID; }),
                    std::end(_editorComponents));
                ClearBit(_componentMask, comp->type());
                RemoveComponent<T>();
                if (comp->type()._value == ComponentType::TRANSFORM) {
                    Hacks._transformComponentCache = nullptr;
                }
            }
        }

        void AddMissingComponents(U32 componentMask);
        /// Serialization: save to XML file
        void saveToXML(const Str256& sceneLocation) const;
        /// Serialization: load from XML file (expressed as a boost property_tree)
        void loadFromXML(const boost::property_tree::ptree& pt);

    private:
        /// Process any events that might of queued up during the ECS Update stages
        void processEvents();
        /// Returns true if the node should be culled (is not visible for the current stage). Calls "preCullNode" internally.
        bool cullNode(const NodeCullParams& params, Frustum::FrustCollision& collisionTypeOut, F32& distanceToClosestPointSQ) const;
        /// Fast distance-to-camera and min-LoD checks. Part of the cullNode call but usefull for quick visibility checks elsewhere
        bool preCullNode(const BoundsComponent& bounds, const NodeCullParams& params, F32& distanceToClosestPointSQ) const;
        /// Called for every single stage of every render pass. Useful for checking materials, doing compute events, etc
        bool preRender(const Camera& camera, RenderStagePass renderStagePass, bool refreshData, bool& rebuildCommandsOut);
        /// Called after preRender and after we rebuild our command buffers. Useful for modifying the command buffer that's going to be used for this RenderStagePass
        bool prepareRender(RenderingComponent& rComp, const Camera& camera, RenderStagePass renderStagePass, bool refreshData);
        /// Called every time we are about to upload or validate our render data to the GPU. Perfect time for some more compute or verifying push constants.
        void onRefreshNodeData(RenderStagePass renderStagePass, const Camera& camera, bool quick, GFX::CommandBuffer& bufferInOut);
        /// Returns true if this node should be drawn based on the specified parameters. Does not do any culling. Just a "if it were to be in view, it would draw".
        bool getDrawState(RenderStagePass stagePass, U8 LoD) const;
        /// Called whenever we send a networking packet from our NetworkingComponent (if any). FrameCount is the frame ID sent with the packet.
        void onNetworkSend(U32 frameCount);
        /// Returns a bottom-up list(leafs -> root) of all of the nodes parented under the current one.
        void getOrderedNodeList(vectorEASTL<SceneGraphNode*>& nodeList);
        /// Destructs all of the nodes specified in the list and removes them from the _children container.
        void processDeleteQueue(vector<vec_size>& childList);
        /// Similar to the saveToXML call but is geared towards temporary state (e.g. save game)
        bool saveCache(ByteBuffer& outputBuffer) const;
        /// Similar to the loadFromXML call but is geared towards temporary state (e.g. save game)
        bool loadCache(ByteBuffer& inputBuffer);
        /// Called by the TransformComponent whenever the transform changed. Useful for Octree updates for example.
        void setTransformDirty(U32 transformMask);
        /// As opposed to "postLoad()" that's called when the SceneNode is ready for processing, this call is used as a callback for when the SceneNode finishes loading as a Resource. postLoad() is called after this always.
        void postLoad(SceneNode& sceneNode, SceneGraphNode& sgn);
        /// This indirect is used to avoid including ECS headers in this file, but we still need the ECS engine for templated methods
        ECS::ECSEngine& GetECSEngine();
        /// Only called from withing "setParent()" (which is also called from "addChildNode()"). Used to mark the existing relationship cache as invalid.
        void invalidateRelationshipCache(SceneGraphNode *source = nullptr);

    private:
        SGNRelationshipCache _relationshipCache;
        vectorEASTL<SceneGraphNode*> _children;
        // ToDo: Remove this HORRIBLE hack -Ionut
        struct hacks {
            vectorFast<EditorComponent*> _editorComponents;
            TransformComponent* _transformComponentCache = nullptr;
        } Hacks;

        moodycamel::ConcurrentQueue<ECSCustomEventType> _events;
        eastl::set<ECSCustomEventType> _uniqueEventsCache;

        mutable SharedMutex _childLock;

        REFERENCE_R(SceneGraph, sceneGraph);
        PROPERTY_R(SceneNode_ptr, node);
        POINTER_R(ECS::ComponentManager, compManager, nullptr);
        POINTER_R(SceneGraphNode, parent, nullptr);
        PROPERTY_R(Str64, name, "");
        PROPERTY_RW(U64, lockToCamera, 0u);
        PROPERTY_R(U64, elapsedTimeUS, 0u);
        PROPERTY_R(U64, lastDeltaTimeUS, 0u);
        PROPERTY_R(U32, componentMask, 0u);
        PROPERTY_R(U32, nodeFlags, 0u);
        PROPERTY_R(U32, instanceCount, 1u);
        PROPERTY_R(bool, serialize, true);
        PROPERTY_RW(NodeUsageContext, usageContext, NodeUsageContext::NODE_STATIC);
        //ToDo: make this work in a multi-threaded environment
        //mutable I8 _frustPlaneCache;
};

namespace Attorney {
    class SceneGraphNodeEditor {
    private:
        static vectorFast<EditorComponent*>& editorComponents(SceneGraphNode& node) {
            return node.Hacks._editorComponents;
        }

        static const vectorFast<EditorComponent*>& editorComponents(const SceneGraphNode& node) {
            return node.Hacks._editorComponents;
        }

        friend class Divide::PropertyWindow;
    };

    class SceneGraphNodeSceneGraph {
    private:
        static void processEvents(SceneGraphNode& node) {
            node.processEvents();
        }

        static void onNetworkSend(SceneGraphNode& node, U32 frameCount) {
            node.onNetworkSend(frameCount);
        }

        static void getOrderedNodeList(SceneGraphNode& node, vectorEASTL<SceneGraphNode*>& nodeList) {
            node.getOrderedNodeList(nodeList);
        }

        static void processDeleteQueue(SceneGraphNode& node, vector<vec_size>& childList) {
            node.processDeleteQueue(childList);
        }

        static bool saveCache(const SceneGraphNode& node, ByteBuffer& outputBuffer) {
            return node.saveCache(outputBuffer);
        }

        static bool loadCache(SceneGraphNode& node, ByteBuffer& inputBuffer) {
            return node.loadCache(inputBuffer);
        }

        friend class Divide::SceneGraph;
    };

    class SceneGraphNodeComponent {
    private:
        static void setTransformDirty(SceneGraphNode& node, U32 transformMask) {
            node.setTransformDirty(transformMask);
        }

        static bool preRender(SceneGraphNode& node, const Camera& camera, RenderStagePass renderStagePass, bool refreshData, bool& rebuildCommandsOut) {
            return node.preRender(camera, renderStagePass, refreshData, rebuildCommandsOut);
        }

        static bool prepareRender(SceneGraphNode& node, RenderingComponent& rComp, const Camera& camera, RenderStagePass renderStagePass, bool refreshData) {
            return node.prepareRender(rComp, camera, renderStagePass, refreshData);
        }

        static void onRefreshNodeData(SceneGraphNode& node, RenderStagePass renderStagePass, const Camera& camera, bool quick, GFX::CommandBuffer& bufferInOut) {
            node.onRefreshNodeData(renderStagePass, camera, quick, bufferInOut);
        }

        static bool getDrawState(const SceneGraphNode& node, RenderStagePass stagePass, U8 LoD) {
            return node.getDrawState(stagePass, LoD);
        }

        friend class Divide::BoundsComponent;
        friend class Divide::RenderingComponent;
        friend class Divide::TransformComponent;
    };

    class SceneGraphNodeRenderPassCuller {
    private:

        // Returns true if the node should be culled (is not visible for the current stage)
        static bool cullNode(const SceneGraphNode& node, const NodeCullParams& params, Frustum::FrustCollision& collisionTypeOut, F32& distanceToClosestPointSQ) {
            return node.cullNode(params, collisionTypeOut, distanceToClosestPointSQ);
        }

        static bool preCullNode(const SceneGraphNode& node, const BoundsComponent& bounds, const NodeCullParams& params, F32& distanceToClosestPointSQ) {
            return node.preCullNode(bounds, params, distanceToClosestPointSQ);
        }

        friend class Divide::RenderPassCuller;
    };

    class SceneGraphNodeRelationshipCache {
    private:
        static const SGNRelationshipCache& relationshipCache(const SceneGraphNode& node) noexcept {
            return node._relationshipCache;
        }

        friend class Divide::SGNRelationshipCache;
    };
    
};  // namespace Attorney


};  // namespace Divide
#endif
