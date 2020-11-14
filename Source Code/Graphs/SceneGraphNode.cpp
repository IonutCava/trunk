#include "stdafx.h"

#include "Headers/SceneGraphNode.h"
#include "Headers/SceneGraph.h"

#include "Core/Headers/PlatformContext.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Water/Headers/Water.h"
#include "Geometry/Material/Headers/Material.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderPackage.h"
#include "Scenes/Headers/SceneState.h"

#include "Editor/Headers/Editor.h"

#include "ECS/Systems/Headers/ECSManager.h"

#include "ECS/Components/Headers/AnimationComponent.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/DirectionalLightComponent.h"
#include "ECS/Components/Headers/NetworkingComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"


namespace Divide {

namespace {
    [[nodiscard]] bool IsTransformNode(const SceneNodeType nodeType, const ObjectType objType) noexcept {
        return nodeType == SceneNodeType::TYPE_TRANSFORM ||
               nodeType == SceneNodeType::TYPE_TRIGGER ||
               objType._value == ObjectType::MESH;
    }

    bool PropagateFlagToChildren(const SceneGraphNode::Flags flag) noexcept {
        return flag == SceneGraphNode::Flags::SELECTED || 
               flag == SceneGraphNode::Flags::HOVERED ||
               flag == SceneGraphNode::Flags::ACTIVE ||
               flag == SceneGraphNode::Flags::VISIBILITY_LOCKED;
    }
};

SceneGraphNode::SceneGraphNode(SceneGraph* sceneGraph, const SceneGraphNodeDescriptor& descriptor)
    : GUIDWrapper(),
      PlatformContextComponent(sceneGraph->parentScene().context()),
      _relationshipCache(this),
      _sceneGraph(sceneGraph),
      _node(descriptor._node),
      _compManager(sceneGraph->GetECSEngine().GetComponentManager()),
      _instanceCount(to_U32(descriptor._instanceCount)),
      _serialize(descriptor._serialize),
      _usageContext(descriptor._usageContext)
{
    std::atomic_init(&_childCount, 0u);
    for (auto& it : Events._eventsFreeList) {
        std::atomic_init(&it, true);
    }
    _name = descriptor._name.empty() ? Util::StringFormat("%s_SGN", _node->resourceName().empty() ? "ERROR"   
                                                                        : _node->resourceName().c_str()).c_str()
                : descriptor._name;

    setFlag(Flags::ACTIVE);
    clearFlag(Flags::VISIBILITY_LOCKED);

    if (_node == nullptr) {
        _node = std::make_shared<SceneNode>(sceneGraph->parentScene().resourceCache(),
                                              generateGUID(),
                                              "EMPTY",
                                              ResourcePath{"EMPTY"},
                                              ResourcePath{},
                                              SceneNodeType::TYPE_TRANSFORM,
                                              to_base(ComponentType::TRANSFORM));
    }

    if (_node->type() == SceneNodeType::TYPE_TRANSFORM) {
        _node->load();
    }

    assert(_node != nullptr);

    AddComponents(descriptor._componentMask, false);
    AddComponents(_node->requiredComponentMask(), false);

    Attorney::SceneNodeSceneGraph::registerSGNParent(_node.get(), this);
}

/// If we are destroying the current graph node
SceneGraphNode::~SceneGraphNode()
{
    Console::printfn(Locale::get(_ID("REMOVE_SCENEGRAPH_NODE")), name().c_str(), _node->resourceName().c_str());

    Attorney::SceneGraphSGN::onNodeDestroy(_sceneGraph, this);
    Attorney::SceneNodeSceneGraph::unregisterSGNParent(_node.get(), this);
    if (Attorney::SceneNodeSceneGraph::parentCount(_node.get()) == 0) {
        assert(_node.use_count() <= Attorney::SceneNodeSceneGraph::maxReferenceCount(_node.get()));

        _node.reset();
    }

    // Bottom up
    for (U32 i = 0; i < getChildCount(); ++i) {
        _sceneGraph->destroySceneGraphNode(_children[i]);
    }
    _children.clear();
    //_childCount.store(0u);

    _compManager->RemoveAllComponents(GetEntityID());
}

ECS::ECSEngine& SceneGraphNode::GetECSEngine() const noexcept {
    return _sceneGraph->GetECSEngine();
}

void SceneGraphNode::AddComponents(const U32 componentMask, const bool allowDuplicates) {

    for (ComponentType::_integral i = 1; i < ComponentType::COUNT + 1; ++i) {
        const U32 componentBit = 1 << i;

        // Only add new components;
        if (BitCompare(componentMask, componentBit) && (allowDuplicates || !BitCompare(_componentMask, componentBit))) {
            _componentMask |= componentBit;
            SGNComponent::construct(ComponentType::_from_integral(componentBit), this);
        }
    };
}

void SceneGraphNode::RemoveComponents(const U32 componentMask) {
    for (ComponentType::_integral i = 1; i < ComponentType::COUNT + 1; ++i) {
        const U32 componentBit = 1 << i;
        if (BitCompare(componentMask, componentBit) && BitCompare(_componentMask, componentBit)) {
            SGNComponent::destruct(ComponentType::_from_integral(componentBit), this);
        }
    }
}

void SceneGraphNode::setTransformDirty(const U32 transformMask) {
    Attorney::SceneGraphSGN::onNodeTransform(_sceneGraph, this);

    SharedLock<SharedMutex> r_lock(_childLock);
    for (SceneGraphNode* node : _children) {
        TransformComponent* tComp = node->get<TransformComponent>();
        if (tComp != nullptr) {
            Attorney::TransformComponentSGN::onParentTransformDirty(*tComp, transformMask);
        }
    }
}

void SceneGraphNode::changeUsageContext(const NodeUsageContext& newContext) {
    _usageContext = newContext;

    TransformComponent* tComp = get<TransformComponent>();
    if (tComp) {
        Attorney::TransformComponentSGN::onParentUsageChanged(*tComp, _usageContext);
    }

    RenderingComponent* rComp = get<RenderingComponent>();
    if (rComp) {
        Attorney::RenderingComponentSGN::onParentUsageChanged(*rComp, _usageContext);
    }
}

/// Change current SceneGraphNode's parent
void SceneGraphNode::setParent(SceneGraphNode* parent, const bool defer) {
    _queuedNewParent = parent->getGUID();
    if (!defer) {
        setParentInternal();
    }
}

void SceneGraphNode::setParentInternal() {
    if (_queuedNewParent == -1) {
        return;
    }
    SceneGraphNode* newParent = sceneGraph()->findNode(_queuedNewParent);
    _queuedNewParent = -1;

    if (newParent == nullptr) {
        return;
    }

    assert(newParent->getGUID() != getGUID());
    
    { //Clear old parent
        if (_parent != nullptr) {
            if (_parent->getGUID() == newParent->getGUID()) {
                return;
            }

            // Remove us from the old parent's children map
            _parent->removeChildNode(this, false, false);
        }
    }
    // Set the parent pointer to the new parent
    _parent = newParent;

    {// Add ourselves in the new parent's children map
        {
            _parent->_childCount.fetch_add(1);
            UniqueLock<SharedMutex> w_lock(_parent->_childLock);
            _parent->_children.push_back(this);
        }
        Attorney::SceneGraphSGN::onNodeAdd(_sceneGraph, this);
        // That's it. Parent Transforms will be updated in the next render pass;
        _parent->invalidateRelationshipCache();
    }
    {// Carry over new parent's flags and settings
        constexpr Flags flags[] = { Flags::SELECTED, Flags::HOVERED, Flags::ACTIVE, Flags::VISIBILITY_LOCKED };
        for (Flags flag : flags) {
            if (_parent->hasFlag(flag)) {
                setFlag(flag);
            } else {
                clearFlag(flag);
            }
        }

        changeUsageContext(_parent->usageContext());
    }
}

/// Add a new SceneGraphNode to the current node's child list based on a SceneNode
SceneGraphNode* SceneGraphNode::addChildNode(const SceneGraphNodeDescriptor& descriptor) {
    // Create a new SceneGraphNode with the SceneNode's info
    // We need to name the new SceneGraphNode
    // If we did not supply a custom name use the SceneNode's name

    SceneGraphNode* sceneGraphNode = _sceneGraph->createSceneGraphNode(_sceneGraph, descriptor);
    assert(sceneGraphNode != nullptr && sceneGraphNode->_node->getState() != ResourceState::RES_CREATED);

    // Set the current node as the new node's parent
    sceneGraphNode->setParent(this);

    if (sceneGraphNode->_node->getState() == ResourceState::RES_LOADED) {
        PostLoad(sceneGraphNode->_node.get(), sceneGraphNode);
    } else if (sceneGraphNode->_node->getState() == ResourceState::RES_LOADING) {
        setFlag(Flags::LOADING);
        sceneGraphNode->_node->addStateCallback(ResourceState::RES_LOADED,
            [this, sceneGraphNode](CachedResource* res) {
                PostLoad(static_cast<SceneNode*>(res), sceneGraphNode);
                clearFlag(Flags::LOADING);
            }
        );
    }
    // return the newly created node
    return sceneGraphNode;
}

void SceneGraphNode::PostLoad(SceneNode* sceneNode, SceneGraphNode* sgn) {
    Attorney::SceneNodeSceneGraph::postLoad(sceneNode, sgn);
    sgn->Hacks._editorComponents.emplace_back(&Attorney::SceneNodeSceneGraph::getEditorComponent(sceneNode));
}

bool SceneGraphNode::removeNodesByType(SceneNodeType nodeType) {
    // Bottom-up pattern
    U32 removalCount = 0, childRemovalCount = 0;
    forEachChild([nodeType, &childRemovalCount](SceneGraphNode* child, I32 /*childIdx*/) {
        if (child->removeNodesByType(nodeType)) {
            ++childRemovalCount;
        }
        return true;
    });

    {
        SharedLock<SharedMutex> r_lock(_childLock);
        const U32 count = to_U32(_children.size());
        for (U32 i = 0; i < count; ++i) {
            if (_children[i]->getNode().type() == nodeType) {
                _sceneGraph->addToDeleteQueue(this, i);
                ++removalCount;
            }
        }
    }

    if (removalCount > 0) {
        return true;
    }

    return childRemovalCount > 0;
}

bool SceneGraphNode::removeChildNode(const SceneGraphNode* node, const bool recursive, bool deleteNode) {

    const I64 targetGUID = node->getGUID();
    {
        SharedLock<SharedMutex> r_lock(_childLock);
        const U32 count = to_U32(_children.size());
        for (U32 i = 0; i < count; ++i) {
            if (_children[i]->getGUID() == targetGUID) {
                if (deleteNode) {
                    _sceneGraph->addToDeleteQueue(this, i);
                } else {
                    _children.erase(_children.begin() + i);
                    _childCount.fetch_sub(1);
                }
                return true;
            }
        }
    }

    // If this didn't finish, it means that we found our node
    return !recursive || 
           !forEachChild([&node, deleteNode](SceneGraphNode* child, I32 /*childIdx*/) {
                if (child->removeChildNode(node, true, deleteNode)) {
                    return false;
                }
                return true;
            });
}

void SceneGraphNode::postLoad() {
    SendEvent(
    {
        ECS::CustomEvent::Type::EntityPostLoad
    });
}

bool SceneGraphNode::isChildOfType(const U16 typeMask) const {
    SceneGraphNode* parentNode = parent();
    while (parentNode != nullptr) {
        if (BitCompare(typeMask, to_base(parentNode->getNode<>().type()))) {
            return true;
        }
        parentNode = parentNode->parent();
    }

    return false;
}

bool SceneGraphNode::isRelated(const SceneGraphNode* target) const {
    const I64 targetGUID = target->getGUID();
    // We also ignore grandparents as this will usually be the root;
    return _relationshipCache.classifyNode(targetGUID) != SGNRelationshipCache::RelationshipType::COUNT;
}

bool SceneGraphNode::isChild(const SceneGraphNode* target, const bool recursive) const {
    const I64 targetGUID = target->getGUID();

    const SGNRelationshipCache::RelationshipType type = _relationshipCache.classifyNode(targetGUID);
    if (type == SGNRelationshipCache::RelationshipType::GRANDCHILD && recursive) {
        return true;
    }

    return type == SGNRelationshipCache::RelationshipType::CHILD;
}

SceneGraphNode* SceneGraphNode::findChild(const U64 nameHash, const bool sceneNodeName, const bool recursive) const {
    SharedLock<SharedMutex> r_lock(_childLock);
    for (auto& child : _children) {
        const U64 cmpHash = sceneNodeName ? _ID(child->getNode().resourceName().c_str()) : _ID(child->name().c_str());
        if (cmpHash == nameHash) {
            return child;
        }
        if (recursive) {
            SceneGraphNode* recChild = child->findChild(nameHash, sceneNodeName, recursive);
            if (recChild != nullptr) {
                return recChild;
            }
        }
    }

    // no child's name matches or there are no more children
    // so return nullptr, indicating that the node was not found yet
    return nullptr;
}

SceneGraphNode* SceneGraphNode::findChild(const I64 GUID, const bool sceneNodeGuid, const bool recursive) const {
    if (GUID != -1) {
        SharedLock<SharedMutex> r_lock(_childLock);
        for (auto& child : _children) {
            if (sceneNodeGuid ? child->getNode().getGUID() == GUID : child->getGUID() == GUID) {
                return child;
            }
            if (recursive) {
                SceneGraphNode* recChild = child->findChild(GUID, sceneNodeGuid, true);
                if (recChild != nullptr) {
                    return recChild;
                }
            }
        }
    }
    // no child's name matches or there are no more children
    // so return nullptr, indicating that the node was not found yet
    return nullptr;
}

bool SceneGraphNode::intersect(const SGNIntersectionParams& params, vectorEASTL<SGNRayResult>& intersections) const {
    vectorEASTL<SGNRayResult> ret = {};
    // If we start from the root node, process children only
    if (_sceneGraph->getRoot()->getGUID() == this->getGUID()) {
        return forEachChild([&](const SceneGraphNode* child, I32 /*childIdx*/) {
            child->intersect(params, intersections);
            return true;
        });
    }

    const auto isIgnored = [&](const SceneNodeType type) {
        for (size_t i = 0; i < params._ignoredTypesCount; ++i) {
            if (type == params._ignoredTypes[i]) {
                return true;
            }
        }
        return false;
    };

    // If we hit a bounding sphere, we proceed to the more expensive OBB test
    if (get<BoundsComponent>()->getBoundingSphere().intersect(params._ray, params._range.min, params._range.max).hit) {
        const RayResult result = get<BoundsComponent>()->getOBB().intersect(params._ray, params._range.min, params._range.max);
        if (result.hit) {
            // If we also hit the OBB we should probably register the intersection since we aren't going to go down to a triangle level intersection or whatnot
            // At least not now ...
            //
            //   const SceneNode& sceneNode = node->getNode();
            const SceneNodeType snType = getNode().type();
            ObjectType objectType = ObjectType::COUNT;
            if (snType == SceneNodeType::TYPE_OBJECT3D) {
                objectType = static_cast<const Object3D&>(getNode()).getObjectType();
            }

            if (!isIgnored(snType) && (params._includeTransformNodes || !IsTransformNode(snType, objectType))) {
                intersections.push_back({ getGUID(), result.dist, name().c_str() });
            }
            forEachChild([&](const SceneGraphNode* child, I32 /*childIdx*/) {
                child->intersect(params, intersections);
                return true;
            });
        }
    }

    return !intersections.empty();
}

void SceneGraphNode::getAllNodes(vectorEASTL<SceneGraphNode*>& nodeList) {
    // Compute from leaf to root to ensure proper calculations
    {
        SharedLock<SharedMutex> r_lock(_childLock);
        for (auto& child : _children) {
            child->getAllNodes(nodeList);
        }
    }

    nodeList.push_back(this);
}

void SceneGraphNode::processDeleteQueue(vectorEASTL<size_t>& childList) {
    // See if we have any children to delete
    if (!childList.empty()) {
        UniqueLock<SharedMutex> w_lock(_childLock);
        for (const size_t childIdx : childList) {
            _sceneGraph->destroySceneGraphNode(_children[childIdx]);
        }
        EraseIndices(_children, childList);
        _childCount.store(to_U32(_children.size()));
    }
}

void SceneGraphNode::frameStarted(const FrameEvent& evt) const {
    NOP();

    forEachChild([&evt](SceneGraphNode* child, I32 /*childIdx*/) {
        child->frameStarted(evt);
        return true;
    });
}

/// Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::sceneUpdate(const U64 deltaTimeUS, SceneState& sceneState) {
    OPTICK_EVENT();

    setParentInternal();

    // update local time
    _elapsedTimeUS += deltaTimeUS;
    _lastDeltaTimeUS = deltaTimeUS;

    if (hasFlag(Flags::ACTIVE)) {
        if (_lockToCamera != 0) {
            TransformComponent* tComp = get<TransformComponent>();
            if (tComp) {
                Camera* cam = Camera::findCamera(_lockToCamera);
                if (cam) {
                    cam->updateLookAt();
                    tComp->setOffset(true, cam->worldMatrix());
                }
            }
        }

        Attorney::SceneNodeSceneGraph::sceneUpdate(_node.get(), deltaTimeUS, this, sceneState);
    }

    if (hasFlag(Flags::PARENT_POST_RENDERED)) {
        clearFlag(Flags::PARENT_POST_RENDERED);
    }

    if (get<RenderingComponent>() == nullptr) {
        BoundsComponent* bComp = get<BoundsComponent>();
        if (bComp->showAABB()) {
            const BoundingBox& bb = bComp->getBoundingBox();
            _context.gfx().debugDrawBox(bb.getMin(), bb.getMax(), DefaultColours::WHITE);
        }
    }
}

void SceneGraphNode::processEvents() {
    OPTICK_EVENT();
    const ECS::EntityId id = GetEntityID();
    
    for (size_t idx = 0; idx < Events.EVENT_QUEUE_SIZE; ++idx) {
        if (!Events._eventsFreeList[idx].exchange(true)) {
            const ECS::CustomEvent& evt = Events._events[idx];

            switch (evt._type) {
                case ECS::CustomEvent::Type::RelationshipCacheInvalidated: {
                    if (!_relationshipCache.isValid()) {
                        _relationshipCache.rebuild();
                    }
                } break;
                case ECS::CustomEvent::Type::EntityFlagChanged: {
                    if (static_cast<Flags>(evt._flag) == Flags::SELECTED) {
                        RenderingComponent* rComp = get<RenderingComponent>();
                        if (rComp != nullptr) {
                            const bool state = evt._dataFirst == 1u;
                            const bool recursive = evt._dataSecond == 1u;
                            rComp->toggleRenderOption(RenderingComponent::RenderOptions::RENDER_SELECTION, state, recursive);
                        }
                    }
                } break;
                default: break;
            };

            _compManager->PassDataToAllComponents(id, evt);
        }
    }
}

bool SceneGraphNode::prepareRender(RenderingComponent& rComp, const RenderStagePass& renderStagePass, const Camera& camera, const bool refreshData) {
    OPTICK_EVENT();

    AnimationComponent* aComp = get<AnimationComponent>();
    if (aComp) {
        RenderPackage& pkg = rComp.getDrawPackage(renderStagePass);
        {
            const AnimationComponent::AnimData data = aComp->getAnimationData();
            if (data._boneBuffer != nullptr) {
                ShaderBufferBinding bufferBinding;
                bufferBinding._binding = ShaderBufferLocation::BONE_TRANSFORMS;
                bufferBinding._buffer = data._boneBuffer;
                bufferBinding._elementRange = data._boneBufferRange;
                pkg.addShaderBuffer(0, bufferBinding);
            }
            if (data._prevBoneBuffer != nullptr) {
                ShaderBufferBinding bufferBinding;
                bufferBinding._binding = ShaderBufferLocation::BONE_TRANSFORMS_PREV;
                bufferBinding._buffer = data._prevBoneBuffer;
                bufferBinding._elementRange = data._prevBoneBufferRange;
                pkg.addShaderBuffer(0, bufferBinding);
            }
        }
    }

    return _node->prepareRender(this, rComp, renderStagePass, camera, refreshData);
}

void SceneGraphNode::onRefreshNodeData(const RenderStagePass& renderStagePass, const Camera& camera, const bool refreshData, GFX::CommandBuffer& bufferInOut) const {
    _node->onRefreshNodeData(this, renderStagePass, camera, refreshData, bufferInOut);
}

bool SceneGraphNode::getDrawState(const RenderStagePass stagePass, const U8 LoD) const {
    return _node->renderState().drawState(stagePass, LoD);
}

void SceneGraphNode::onNetworkSend(U32 frameCount) const {
    forEachChild([frameCount](SceneGraphNode* child, I32 /*childIdx*/) {
        child->onNetworkSend(frameCount);
        return true;
    });

    NetworkingComponent* net = get<NetworkingComponent>();
    if (net) {
        net->onNetworkSend(frameCount);
    }
}


bool SceneGraphNode::preCullNode(const BoundsComponent& bounds, const NodeCullParams& params, F32& distanceToClosestPointSQ) const {
    OPTICK_EVENT();

    // If the node is still loading, DO NOT RENDER IT. Bad things happen :D
    if (!hasFlag(Flags::LOADING)) {
        // Get camera info
        const vec3<F32>& eye = params._currentCamera->getEye();

        // Check distance to sphere edge (center - radius)
        distanceToClosestPointSQ = bounds.getBoundingBox().nearestDistanceFromPointSquared(eye);
        if (distanceToClosestPointSQ < params._cullMaxDistanceSq) {
            const F32 upperBound = params._minExtents.maxComponent();
            if (upperBound > 0.0f &&
                bounds.getBoundingBox().getExtent().maxComponent() < upperBound)
            {
                return true;
            }

            RenderingComponent* rComp = get<RenderingComponent>();
            const vec2<F32>& renderRange = rComp->renderRange();
            if (IS_IN_RANGE_INCLUSIVE(distanceToClosestPointSQ, SIGNED_SQUARED(renderRange.min), SQUARED(renderRange.max))) {
                return params._minLoD > -1 &&
                       rComp->getLoDLevel(bounds.getBoundingSphere().getCenter(), eye, params._stage, params._lodThresholds) > params._minLoD;
            }
        }
    }

    return true;
}

bool SceneGraphNode::cullNode(const NodeCullParams& params,
                              FrustumCollision& collisionTypeOut,
                              F32& distanceToClosestPointSQ) const {
    OPTICK_EVENT();

    collisionTypeOut = FrustumCollision::FRUSTUM_OUT;

    // Some nodes should always render for different reasons (eg, trees are instanced and bound to the parent chunk)
    if (hasFlag(Flags::VISIBILITY_LOCKED)) {
        collisionTypeOut = FrustumCollision::FRUSTUM_IN;
        return false;
    }

    const BoundsComponent* bComp = get<BoundsComponent>();
    if (preCullNode(*bComp, params, distanceToClosestPointSQ)) {
        return true;
    }

    STUBBED("ToDo: make this work in a multi-threaded environment -Ionut");
    _frustPlaneCache = -1;
    I8 fakePlaneCache = -1;

    const BoundingSphere& sphere = bComp->getBoundingSphere();
    const BoundingBox& boundingBox = bComp->getBoundingBox();
    const F32 radius = sphere.getRadius();
    const vec3<F32>& center = sphere.getCenter();

    // Get camera info
    const vec3<F32>& eye = params._currentCamera->getEye();
    // Sphere is in range, so check bounds primitives against the frustum
    if (!boundingBox.containsPoint(eye)) {
        const Frustum& frustum = params._currentCamera->getFrustum();
        // Check if the bounding sphere is in the frustum, as Frustum <-> Sphere check is fast
        collisionTypeOut = frustum.ContainsSphere(center, radius, fakePlaneCache);
        if (collisionTypeOut == FrustumCollision::FRUSTUM_INTERSECT) {
            // If the sphere is not completely in the frustum, check the AABB
            collisionTypeOut = frustum.ContainsBoundingBox(boundingBox, fakePlaneCache);
        }
    } else {
        // We are inside the AABB. So ... intersect?
        collisionTypeOut = FrustumCollision::FRUSTUM_INTERSECT;
    }

    return collisionTypeOut == FrustumCollision::FRUSTUM_OUT;
}

void SceneGraphNode::occlusionCull(const RenderStagePass& stagePass,
                                   const Texture_ptr& depthBuffer,
                                   const Camera& camera,
                                   GFX::SendPushConstantsCommand& HIZPushConstantsCMDInOut,
                                   GFX::CommandBuffer& bufferInOut) const {
    Attorney::SceneNodeSceneGraph::occlusionCullNode(_node.get(), stagePass, depthBuffer, camera, HIZPushConstantsCMDInOut, bufferInOut);
}

void SceneGraphNode::invalidateRelationshipCache(SceneGraphNode* source) {
    if (source == this || !_relationshipCache.isValid()) {
        return;
    }

    SendEvent(
    {
        ECS::CustomEvent::Type::RelationshipCacheInvalidated
    });

    _relationshipCache.invalidate();

    if (_parent && _parent->parent()) {
        _parent->invalidateRelationshipCache(this);

        forEachChild([this, source](SceneGraphNode* child, I32 /*childIdx*/) {
            if (!source || child->getGUID() != source->getGUID()) {
                child->invalidateRelationshipCache(this);
            }
            return true;
        });
    }
}

bool SceneGraphNode::saveCache(ByteBuffer& outputBuffer) const {
    getNode().saveCache(outputBuffer);

    for (EditorComponent* editorComponent : Hacks._editorComponents) {
        if (!Attorney::EditorComponentSceneGraphNode::saveCache(*editorComponent, outputBuffer)) {
            return false;
        }
    }

    if (!_sceneGraph->GetECSManager().saveCache(this, outputBuffer)) {
        return false;
    }

    return true;
}

bool SceneGraphNode::loadCache(ByteBuffer& inputBuffer) {
    getNode().loadCache(inputBuffer);

    for (EditorComponent* editorComponent : Hacks._editorComponents) {
        if (!Attorney::EditorComponentSceneGraphNode::loadCache(*editorComponent, inputBuffer)) {
            return false;
        }
    }

    if (!_sceneGraph->GetECSManager().loadCache(this, inputBuffer)) {
        return false;
    }

    return true;
}

void SceneGraphNode::saveToXML(const Str256& sceneLocation, DELEGATE<void, std::string_view> msgCallback) const {
    if (!serialize()) {
        return;
    }

    if (msgCallback) {
        msgCallback(Util::StringFormat("Saving node [ %s ] ...", name().c_str()).c_str());
    }

    boost::property_tree::ptree pt;
    pt.put("static", usageContext() == NodeUsageContext::NODE_STATIC);

    getNode().saveToXML(pt);

    for (EditorComponent* editorComponent : Hacks._editorComponents) {
        Attorney::EditorComponentSceneGraphNode::saveToXML(*editorComponent, pt);
    }

    auto targetFile = sceneLocation + "/nodes/";
    targetFile.append(parent()->name());
    targetFile.append("_");
    targetFile.append(name());
    targetFile.append(".xml");
    XML::writeXML(targetFile.c_str(), pt);

    forEachChild([&sceneLocation, &msgCallback](const SceneGraphNode* child, I32 /*childIdx*/){
        child->saveToXML(sceneLocation, msgCallback);
        return true;
    });
}

void SceneGraphNode::loadFromXML(const Str256& sceneLocation) {
    boost::property_tree::ptree pt;
    auto targetFile = sceneLocation + "/nodes/";
    targetFile.append(parent()->name());
    targetFile.append("_");
    targetFile.append(name());
    targetFile.append(".xml");
    XML::readXML(targetFile.c_str(), pt);

    loadFromXML(pt);
}

void SceneGraphNode::loadFromXML(const boost::property_tree::ptree& pt) {
    if (!serialize()) {
        return;
    }

    changeUsageContext(pt.get("static", false) ? NodeUsageContext::NODE_STATIC : NodeUsageContext::NODE_DYNAMIC);

    U32 componentsToLoad = 0;
    for (ComponentType::_integral i = 1; i < ComponentType::COUNT + 1; ++i) {
        ComponentType type = ComponentType::_from_integral(1 << i);
        if (pt.count(type._to_string()) > 0) {
            componentsToLoad |= type._to_integral();
        }
    }

    if (componentsToLoad != 0) {
        AddComponents(componentsToLoad, false);
    }

    for (EditorComponent* editorComponent : Hacks._editorComponents) {
        Attorney::EditorComponentSceneGraphNode::loadFromXML(*editorComponent, pt);
    }
}

void SceneGraphNode::setFlag(Flags flag, bool recursive) noexcept {
    const bool hadFlag = hasFlag(flag);
    if (!hadFlag) {
        SetBit(_nodeFlags, to_U32(flag));
    }

    if (recursive && PropagateFlagToChildren(flag)) {
        forEachChild([flag, recursive](SceneGraphNode* child, I32 /*childIdx*/) {
            child->setFlag(flag, recursive);
            return true;
        });
    }

    if (!hadFlag) {
        ECS::CustomEvent evt = {
            ECS::CustomEvent::Type::EntityFlagChanged,
            nullptr,
            to_U32(flag)
        };
        evt._dataFirst = 1u;
        evt._dataSecond = recursive ? 1u : 0u;

        SendEvent(MOV(evt));
    }
}

void SceneGraphNode::clearFlag(Flags flag, bool recursive) noexcept {
    const bool hadFlag = hasFlag(flag);
    if (hadFlag) {
        ClearBit(_nodeFlags, to_U32(flag));
    }

    if (recursive && PropagateFlagToChildren(flag)) {
        forEachChild([flag, recursive](SceneGraphNode* child, I32 /*childIdx*/) {
            child->clearFlag(flag, recursive);
            return true;
        });
    }
    
    if (hadFlag) {
        ECS::CustomEvent evt = {
            ECS::CustomEvent::Type::EntityFlagChanged,
            nullptr,
            to_U32(flag)
        };
        evt._dataFirst = 0u;
        evt._dataSecond = recursive ? 1u : 0u;

        SendEvent(MOV(evt));
    }
}

bool SceneGraphNode::hasFlag(const Flags flag) const noexcept {
    return BitCompare(_nodeFlags, to_U32(flag));
}

void SceneGraphNode::SendEvent(ECS::CustomEvent&& event) {
    size_t idx = 0;
    while (true) {
        bool flush = false;
        {
            if (Events._eventsFreeList[idx].exchange(false)) {
                Events._events[idx] = MOV(event);
                return;
            }

            if (++idx >= Events.EVENT_QUEUE_SIZE) {
                idx %= Events.EVENT_QUEUE_SIZE;
                flush = Runtime::isMainThread();
            }
        }
        if (flush) {
            processEvents();
        }
    }
}

};