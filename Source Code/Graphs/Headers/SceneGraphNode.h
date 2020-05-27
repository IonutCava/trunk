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
class RenderPassManager;
class RenderingComponent;
class TransformComponent;

struct FrameEvent;
struct NodeCullParams;

struct SceneGraphNodeDescriptor {
    SceneNode_ptr    _node = nullptr;
    Str64            _name = "";
    size_t           _instanceCount = 1;
    U32              _componentMask = 0u;
    NodeUsageContext _usageContext = NodeUsageContext::NODE_STATIC;
    bool             _serialize = true;
};

namespace Attorney {
    class SceneGraphNodeEditor;
    class SceneGraphNodeComponent;
    class SceneGraphNodeSceneGraph;
    class SceneGraphNodeRenderPassCuller;
    class SceneGraphNodeRenderPassManager;
    class SceneGraphNodeRelationshipCache;
};

struct SGNRayResult {
    I64 sgnGUID = -1;
    F32 dist = std::numeric_limits<F32>::max();
    const char* name = nullptr;
};

class SceneGraphNode final : public ECS::Entity<SceneGraphNode>,
                             public GUIDWrapper,
                             public PlatformContextComponent
{
    friend class Attorney::SceneGraphNodeEditor;
    friend class Attorney::SceneGraphNodeComponent;
    friend class Attorney::SceneGraphNodeSceneGraph;
    friend class Attorney::SceneGraphNodeRenderPassCuller;
    friend class Attorney::SceneGraphNodeRenderPassManager;
    friend class Attorney::SceneGraphNodeRelationshipCache;

    public:

        enum class Flags : U16 {
            SPATIAL_PARTITION_UPDATE_QUEUED = toBit(1),
            LOADING = toBit(2),
            HOVERED = toBit(3),
            SELECTED = toBit(4),
            ACTIVE = toBit(5),
            VISIBILITY_LOCKED = toBit(6),
            MESH_POST_RENDERED = toBit(7),
            COUNT = 7
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
        bool removeChildNode(const SceneGraphNode& node, bool recursive = true, bool deleteNode = true);

        /// Find a child Node using the given name (either SGN name or SceneNode name)
        SceneGraphNode* findChild(const U64 nameHash, bool sceneNodeName = false, bool recursive = false) const;

        /// Find a child using the given SGNN or SceneNode GUID
        SceneGraphNode* findChild(I64 GUID, bool sceneNodeGuid = false, bool recursive = false) const;

        /// If this function returns true, at least one node of the specified type was removed.
        bool removeNodesByType(SceneNodeType nodeType);

        /// Changing a node's parent means removing this node from the current parent's child list and appending it to the new parent's list (happens after a full frame)
        void setParent(SceneGraphNode& parent, bool defer = false);

        /// Checks if we have a parent matching the typeMask. We check recursively until we hit the top node (if ignoreRoot is false, top node is Root)
        bool isChildOfType(U16 typeMask) const;

        /// Returns true if the current node is related somehow to the specified target node (see RelationshipType enum for more details)
        bool isRelated(const SceneGraphNode& target) const;

        /// Returns true if the specified target node is a parent or grandparent(if recursive == true) of the current node
        bool isChild(const SceneGraphNode& target, bool recursive) const;

        /// Recursively call the delegate function on all children. Returns false if the loop was interrupted. Use start and end to only affect a range (useful for parallel algorithms)
        template<class Predicate>
        bool forEachChild(U32 start, U32 end, Predicate pred) const;
        template<class Predicate>
        bool forEachChild(Predicate pred) const;

        /// A "locked" call assumes that either access is guaranteed thread-safe or that the child lock is already aquired
        inline const vectorEASTL<SceneGraphNode*>& getChildrenLocked() const noexcept { return _children; }

        /// Return the current number of children that the current node has
        inline U32 getChildCount() const noexcept { return _childCount.load(); }

        inline void lockChildrenForWrite() const { _childLock.lock(); }

        inline void unlockChildrenForWrite() const { _childLock.unlock(); }

        inline void lockChildrenForRead() const { _childLock.lock_shared(); }

        inline void unlockChildrenForRead() const { _childLock.unlock_shared(); }

        /// Return a specific child by indes. Does not recurse.
        inline SceneGraphNode& getChild(U32 idx) {
            SharedLock<SharedMutex> r_lock(_childLock);
            return *_children[idx];
        }

        /// Return a specific child by indes. Does not recurse.
        inline const SceneGraphNode& getChild(U32 idx) const {
            SharedLock<SharedMutex> r_lock(_childLock);
            return *_children[idx];
        }

        /// Called from parent SceneGraph
        void sceneUpdate(const U64 deltaTimeUS, SceneState& sceneState);

        /// Invoked by the contained SceneNode when it finishes all of its internal loading and is ready for processing
        void postLoad();

        /// Find the graph nodes whom's bounding boxes intersects the given ray
        bool intersect(const Ray& ray, F32 start, F32 end, vectorEASTL<SGNRayResult>& intersections) const;

        void changeUsageContext(const NodeUsageContext& newContext);

        /// General purpose flag management. Certain flags propagate to children (e.g. selection)!
        void setFlag(Flags flag, bool recursive = true) noexcept;
        /// Clearing a flag might propagate to child nodes (e.g. selection).
        void clearFlag(Flags flag, bool recursive = true) noexcept;
        /// Returns true only if the currrent node has the specified flag. Does not check children!
        bool hasFlag(Flags flag) const noexcept;

        /// Always use the level of redirection needed to reduce virtual function overhead.
        /// Use getNode<SceneNode> if you need material properties for ex. or getNode<SkinnedSubMesh> for animation transforms
        template <typename T = SceneNode>
        typename std::enable_if<std::is_base_of<SceneNode, T>::value, T&>::type
        getNode() noexcept { return static_cast<T&>(*_node); }

        /// Always use the level of redirection needed to reduce virtual function overhead.
        /// Use getNode<SceneNode> if you need material properties for ex. or getNode<SkinnedSubMesh> for animation transforms
        template <typename T = SceneNode>
        typename std::enable_if<std::is_base_of<SceneNode, T>::value, const T&>::type
        getNode() const noexcept { return static_cast<const T&>(*_node); }

        const SceneNode_ptr& getNodePtr() const noexcept { return _node; }

        /// Returns a pointer to a specific component. Returns null if the SGN doesn't have the component requested
        template <typename T>
        inline T* get() const { return _compManager->GetComponent<T>(GetEntityID()); } //< ToDo: Optimise this -Ionut

        template <>
        inline TransformComponent* get() const noexcept { return Hacks._transformComponentCache; }

        template <>
        inline BoundsComponent* get() const noexcept { return Hacks._boundsComponentCache; }

        void SendEvent(ECS::CustomEvent&& event);
        void SendEvent(const ECS::CustomEvent& event);

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
            if (comp->type()._value == ComponentType::BOUNDS) {
                //Ewww x2
                Hacks._boundsComponentCache = (BoundsComponent*)comp;
            }

            return static_cast<T*>(comp);
        }

        /// Remove a component by type (if any). Because we have a limit of one component type per node, this works as expected
        template<class T>
        typename std::enable_if<std::is_base_of<SGNComponent, T>::value, void>::type
        RemoveSGNComponent() {
            SGNComponent* comp = (SGNComponent*)(GetComponent<T>());
            if (comp != nullptr) {
                const I64 targetGUID = comp->getEditorComponent().getGUID();

                Hacks._editorComponents.erase(std::remove_if(std::begin(Hacks._editorComponents),
                                                             std::end(Hacks._editorComponents),
                                                             [targetGUID](EditorComponent* editorComp) noexcept -> bool {
                                                                 return editorComp->getGUID() == targetGUID;
                                                             }),
                                              std::end(Hacks._editorComponents));

                ClearBit(_componentMask, to_U32(comp->type()));
                RemoveComponent<T>();

                if (comp->type()._value == ComponentType::TRANSFORM) {
                    Hacks._transformComponentCache = nullptr;
                }
                if (comp->type()._value == ComponentType::BOUNDS) {
                    Hacks._boundsComponentCache = nullptr;
                }
            }
        }

        void AddComponents(U32 componentMask, bool allowDuplicates);
        void RemoveComponents(U32 componentMask);

        /// Serialization: save to XML file
        void saveToXML(const Str256& sceneLocation, DELEGATE<void, std::string_view> msgCallback = {}) const;
        void loadFromXML(const Str256& sceneLocation);
        /// Serialization: load from XML file (expressed as a boost property_tree)
        void loadFromXML(const boost::property_tree::ptree& pt);

    private:
        /// Process any events that might of queued up during the ECS Update stages
        void processEvents();
        /// Returns true if the node should be culled (is not visible for the current stage). Calls "preCullNode" internally.
        bool cullNode(const NodeCullParams& params, Frustum::FrustCollision& collisionTypeOut, F32& distanceToClosestPointSQ) const;
        /// Fast distance-to-camera and min-LoD checks. Part of the cullNode call but usefull for quick visibility checks elsewhere
        bool preCullNode(const BoundsComponent& bounds, const NodeCullParams& params, F32& distanceToClosestPointSQ) const;
        /// Called after preRender and after we rebuild our command buffers. Useful for modifying the command buffer that's going to be used for this RenderStagePass
        bool prepareRender(RenderingComponent& rComp, const RenderStagePass& renderStagePass, const Camera& camera, bool refreshData);
        /// Called before we start preparing draw packages. Perfect for adding non-node specific commands to the command buffer (e.g. culling tasks)
        void onRefreshNodeData(const RenderStagePass& renderStagePass, const Camera& camera, bool refreshData, GFX::CommandBuffer& bufferInOut);
        /// Returns true if this node should be drawn based on the specified parameters. Does not do any culling. Just a "if it were to be in view, it would draw".
        bool getDrawState(RenderStagePass stagePass, U8 LoD) const;
        /// Called whenever we send a networking packet from our NetworkingComponent (if any). FrameCount is the frame ID sent with the packet.
        void onNetworkSend(U32 frameCount);
        /// Returns a bottom-up list(leafs -> root) of all of the nodes parented under the current one.
        void getOrderedNodeList(vectorEASTL<SceneGraphNode*>& nodeList);
        /// Destructs all of the nodes specified in the list and removes them from the _children container.
        void processDeleteQueue(vectorEASTL<size_t>& childList);
        /// Called on every new frame
        void frameStarted(const FrameEvent& evt);
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
        /// Changes this node's parent
        void setParentInternal();

        void occlusionCull(const RenderStagePass& stagePass,
                           const Texture_ptr& depthBuffer,
                           const Camera& camera,
                           GFX::SendPushConstantsCommand& HIZPushConstantsCMDInOut,
                           GFX::CommandBuffer& bufferInOut) const;
    private:
        SGNRelationshipCache _relationshipCache;
        vectorEASTL<SceneGraphNode*> _children;
        std::atomic_uint _childCount;

        // ToDo: Remove this HORRIBLE hack -Ionut
        struct hacks {
            vectorEASTLFast<EditorComponent*> _editorComponents;
            TransformComponent* _transformComponentCache = nullptr;
            BoundsComponent* _boundsComponentCache = nullptr;
        } Hacks;

        Mutex _eventsLock;
        eastl::queue<ECS::CustomEvent, eastl::deque<ECS::CustomEvent, eastl::dvd_eastl_allocator>> _events;

        mutable SharedMutex _childLock;

        REFERENCE_R(SceneGraph, sceneGraph);
        PROPERTY_R(SceneNode_ptr, node, nullptr);
        POINTER_R(ECS::ComponentManager, compManager, nullptr);
        POINTER_R(SceneGraphNode, parent, nullptr);
        PROPERTY_R(Str64, name, "");
        PROPERTY_R(I64, queuedNewParent, -1);
        PROPERTY_RW(U64, lockToCamera, 0u);
        PROPERTY_R(U64, elapsedTimeUS, 0u);
        PROPERTY_R(U64, lastDeltaTimeUS, 0u);
        PROPERTY_R(U32, componentMask, 0u);
        PROPERTY_R(U32, nodeFlags, 0u);
        PROPERTY_R(U32, instanceCount, 1u);
        PROPERTY_R(bool, serialize, true);
        PROPERTY_R(NodeUsageContext, usageContext, NodeUsageContext::NODE_STATIC);
        //ToDo: make this work in a multi-threaded environment
        mutable I8 _frustPlaneCache = -1;
};

namespace Attorney {
    class SceneGraphNodeEditor {
    private:
        static vectorEASTLFast<EditorComponent*>& editorComponents(SceneGraphNode& node) noexcept {
            return node.Hacks._editorComponents;
        }

        static const vectorEASTLFast<EditorComponent*>& editorComponents(const SceneGraphNode& node) noexcept {
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

        static void frameStarted(SceneGraphNode& node, const FrameEvent& evt) {
            node.frameStarted(evt);
        }

;        static void processDeleteQueue(SceneGraphNode& node, vectorEASTL<size_t>& childList) {
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

        static bool prepareRender(SceneGraphNode& node, RenderingComponent& rComp, const Camera& camera, const RenderStagePass& renderStagePass, bool refreshData) {
            return node.prepareRender(rComp, renderStagePass, camera, refreshData);
        }

        static void onRefreshNodeData(SceneGraphNode& node, const RenderStagePass& renderStagePass, const Camera& camera, bool refreshData, GFX::CommandBuffer& bufferInOut) {
            node.onRefreshNodeData(renderStagePass, camera, refreshData, bufferInOut);
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

    class SceneGraphNodeRenderPassManager
    {
    private:

        static void occlusionCullNode(const SceneGraphNode& node, const RenderStagePass& stagePass, const Texture_ptr& depthBuffer, const Camera& camera, GFX::SendPushConstantsCommand& HIZPushConstantsCMDInOut, GFX::CommandBuffer& bufferInOut) {
            node.occlusionCull(stagePass, depthBuffer, camera, HIZPushConstantsCMDInOut, bufferInOut);
        }

        friend class Divide::RenderPassManager;
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

#include "SceneGraphNode.inl"