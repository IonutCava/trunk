#include "stdafx.h"

#include "Headers/SceneGraph.h"
#include "Headers/SceneGraphNode.h"

#include "Core/Headers/PlatformContext.h"
#include "Scenes/Headers/SceneState.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"
#include "Environment/Water/Headers/Water.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Platform/Video/Headers/RenderPackage.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

#include "Editor/Headers/Editor.h"

#include "ECS/Events/Headers/EntityEvents.h"
#include "ECS/Events/Headers/BoundsEvents.h"
#include "ECS/Events/Headers/TransformEvents.h"
#include "ECS/Systems/Headers/ECSManager.h"

#include "ECS/Components/Headers/IKComponent.h"
#include "ECS/Components/Headers/RagdollComponent.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/AnimationComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/NetworkingComponent.h"
#include "ECS/Components/Headers/SpotLightComponent.h"
#include "ECS/Components/Headers/PointLightComponent.h"
#include "ECS/Components/Headers/DirectionalLightComponent.h"


namespace Divide {

namespace {
    constexpr size_t INITIAL_CHILD_COUNT = 16;

    inline bool PropagateFlagToChildren(SceneGraphNode::Flags flag) {
        return flag == SceneGraphNode::Flags::SELECTED || 
               flag == SceneGraphNode::Flags::HOVERED ||
               flag == SceneGraphNode::Flags::ACTIVE ||
               flag == SceneGraphNode::Flags::VISIBILITY_LOCKED;
    }
};

SceneGraphNode::SceneGraphNode(SceneGraph& sceneGraph, const SceneGraphNodeDescriptor& descriptor)
    : GUIDWrapper(),
      ECS::Event::IEventListener(sceneGraph.GetECSEngine()),
      PlatformContextComponent(sceneGraph.parentScene().context()),
      _sceneGraph(sceneGraph),
      _compManager(sceneGraph.GetECSEngine().GetComponentManager()),
      _serialize(descriptor._serialize),
      _node(descriptor._node),
      _instanceCount(to_U32(descriptor._instanceCount)),
      _componentMask(0),
      _usageContext(descriptor._usageContext),
      //_frustPlaneCache(-1),
      _elapsedTimeUS(0ULL),
      _lastDeltaTimeUS(0ULL),
      _relationshipCache(*this)
{
    _name = (descriptor._name.empty() ? Util::StringFormat("%s_SGN", (_node->resourceName().empty() ? "ERROR"   
                                                                                                    : _node->resourceName().c_str()))
                                      : descriptor._name);

    setFlag(Flags::ACTIVE);
    clearFlag(Flags::VISIBILITY_LOCKED);

    if (_node == nullptr) {
        _node = std::make_shared<SceneNode>(sceneGraph.parentScene().resourceCache(), GUIDWrapper::generateGUID(), "");
    }

    if (_node->type() == SceneNodeType::TYPE_EMPTY || _node->type() == SceneNodeType::TYPE_ROOT) {
        _node->load();
    }

    assert(_node != nullptr);
    _children.reserve(INITIAL_CHILD_COUNT);

    AddMissingComponents(descriptor._componentMask);
    Attorney::SceneNodeSceneGraph::registerSGNParent(*_node, this);
}

/// If we are destroying the current graph node
SceneGraphNode::~SceneGraphNode()
{
    Console::printfn(Locale::get(_ID("REMOVE_SCENEGRAPH_NODE")), name().c_str(), _node->resourceName().c_str());

    Attorney::SceneGraphSGN::onNodeDestroy(_sceneGraph, *this);
    Attorney::SceneNodeSceneGraph::unregisterSGNParent(*_node, this);
    if (Attorney::SceneNodeSceneGraph::parentCount(*_node) == 0) {
        assert(_node.use_count() <= Attorney::SceneNodeSceneGraph::maxReferenceCount(*_node));

        _node.reset();
    }

    // Bottom up
    for (U32 i = 0; i < getChildCount(); ++i) {
        _sceneGraph.destroySceneGraphNode(_children[i]);
    }
    _children.clear();

    UnregisterAllEventCallbacks();

    _compManager->RemoveAllComponents(GetEntityID());
    _editorComponents.clear();
    _componentMask = 0;
}

ECS::ECSEngine& SceneGraphNode::GetECSEngine() {
    return _sceneGraph.GetECSEngine();
}

void SceneGraphNode::AddMissingComponents(U32 componentMask) {

    for (ComponentType::_integral i = 1; i < ComponentType::COUNT; ++i) {
        const U16 componentBit = 1 << i;

        // Only add new components;
        if (BitCompare(componentMask, to_U32(componentBit)) && !BitCompare(_componentMask, to_U32(componentBit))) {
            _componentMask |= componentBit;
            SGNComponent::construct(ComponentType::_from_integral(componentBit), *this);
        }
    };

}

void SceneGraphNode::setTransformDirty(U32 transformMask) {
    Attorney::SceneGraphSGN::onNodeTransform(_sceneGraph, *this);

    SharedLock r_lock(_childLock);
    U32 childCount = getChildCountLocked();
    for (U32 i = 0; i < childCount; ++i) {
        TransformComponent* tComp = _children[i]->get<TransformComponent>();
        if (tComp) {
            Attorney::TransformComponentSGN::onParentTransformDirty(*tComp, transformMask);
        }
    }
}

void SceneGraphNode::changeUsageContext(const NodeUsageContext& newContext) {
    usageContext(newContext);

    TransformComponent* tComp = get<TransformComponent>();
    if (tComp) {
        Attorney::TransformComponentSGN::onParentUsageChanged(*tComp, _usageContext);
    }
}

/// Change current SceneGraphNode's parent
void SceneGraphNode::setParent(SceneGraphNode& parent) {
    assert(parent.getGUID() != getGUID());
    
    { //Clear old parent
        if (_parent) {
            if (*_parent == parent) {
                return;
            }
            // Remove us from the old parent's children map
            _parent->removeChildNode(*this, false);
        }
    }
    // Set the parent pointer to the new parent
    _parent = &parent;

    {// Add ourselves in the new parent's children map
        {
            UniqueLockShared w_lock(parent._childLock);
            parent._children.push_back(this);
        }
        Attorney::SceneGraphSGN::onNodeAdd(_sceneGraph, *this);
        // That's it. Parent Transforms will be updated in the next render pass;
        parent.invalidateRelationshipCache();
    }
    {// Carry over new parent's flags and settings
        constexpr Flags flags[] = { Flags::SELECTED, Flags::HOVERED, Flags::ACTIVE, Flags::VISIBILITY_LOCKED };
        for (Flags flag : flags) {
            if (parent.hasFlag(flag)) {
                setFlag(flag);
            } else {
                clearFlag(flag);
            }
        }

        usageContext(parent.usageContext());
    }
}

/// Add a new SceneGraphNode to the current node's child list based on a SceneNode
SceneGraphNode* SceneGraphNode::addChildNode(const SceneGraphNodeDescriptor& descriptor) {
    // Create a new SceneGraphNode with the SceneNode's info
    // We need to name the new SceneGraphNode
    // If we did not supply a custom name use the SceneNode's name

    SceneGraphNode* sceneGraphNode = _sceneGraph.createSceneGraphNode(_sceneGraph, descriptor);
    assert(sceneGraphNode != nullptr && sceneGraphNode->_node->getState() != ResourceState::RES_CREATED);

    // Set the current node as the new node's parent
    sceneGraphNode->setParent(*this);

    if (sceneGraphNode->_node->getState() == ResourceState::RES_LOADED) {
        postLoad(*sceneGraphNode->_node, *sceneGraphNode);
    } else if (sceneGraphNode->_node->getState() == ResourceState::RES_LOADING) {
        setFlag(Flags::LOADING);
        sceneGraphNode->_node->setStateCallback(ResourceState::RES_LOADED,
            [this, sceneGraphNode](Resource_wptr res) {
                postLoad(*(std::dynamic_pointer_cast<SceneNode>(res.lock())), *(sceneGraphNode));
                clearFlag(Flags::LOADING);
            }
        );
    }
    // return the newly created node
    return sceneGraphNode;
}

void SceneGraphNode::postLoad(SceneNode& sceneNode, SceneGraphNode& sgn) {
    Attorney::SceneNodeSceneGraph::postLoad(sceneNode, sgn);
    sgn._editorComponents.emplace_back(&Attorney::SceneNodeSceneGraph::getEditorComponent(sceneNode));
}

bool SceneGraphNode::removeNodesByType(SceneNodeType nodeType) {
    // Bottom-up pattern
    U32 removalCount = 0, childRemovalCount = 0;
    forEachChild([nodeType, &childRemovalCount](SceneGraphNode* child, I32 /*childIdx*/) {
        if (child->removeNodesByType(nodeType)) {
            ++childRemovalCount;
        }
    });

    {
        SharedLock r_lock(_childLock);
        for (U32 i = 0; i < getChildCountLocked(); ++i) {
            if (_children[i]->getNode().type() == nodeType) {
                {
                    _sceneGraph.addToDeleteQueue(this, i);
                    ++removalCount;
                }
            }
        }
    }

    if (removalCount > 0) {
        return true;
    }

    return childRemovalCount > 0;
}

bool SceneGraphNode::removeChildNode(const SceneGraphNode& node, bool recursive) {

    I64 targetGUID = node.getGUID();
    {
        SharedLock r_lock(_childLock);
        U32 childCount = getChildCountLocked();
        for (U32 i = 0; i < childCount; ++i) {
            if (_children[i]->getGUID() == targetGUID) {
                {
                    _sceneGraph.addToDeleteQueue(this, i);
                }

                return true;
            }
        }
    }

    // If this didn't finish, it means that we found our node
    return !recursive || 
           !forEachChildInterruptible([&node](SceneGraphNode* child, I32 /*childIdx*/) {
                if (child->removeChildNode(node, true)) {
                    return false;
                }
                return true;
            });
}

void SceneGraphNode::postLoad() {
    SendEvent<EntityPostLoad>(GetEntityID());
}

bool SceneGraphNode::isChildOfType(U16 typeMask, bool ignoreRoot) const {
    if (ignoreRoot) {
        ClearBit(typeMask, to_base(SceneNodeType::TYPE_ROOT));
    }
    SceneGraphNode* parentNode = parent();
    while (parentNode != nullptr) {
        if (BitCompare(typeMask, to_base(parentNode->getNode<>().type()))) {
            return true;
        }
        parentNode = parentNode->parent();
    }

    return false;
}

bool SceneGraphNode::isRelated(const SceneGraphNode& target) const {
    I64 targetGUID = target.getGUID();

    SGNRelationshipCache::RelationshipType type = _relationshipCache.clasifyNode(targetGUID);

    // We also ignore grandparents as this will usually be the root;
    return type != SGNRelationshipCache::RelationshipType::COUNT;
}

bool SceneGraphNode::isChild(const SceneGraphNode& target, bool recursive) const {
    I64 targetGUID = target.getGUID();

    SGNRelationshipCache::RelationshipType type = _relationshipCache.clasifyNode(targetGUID);
    if (type == SGNRelationshipCache::RelationshipType::GRANDCHILD && recursive) {
        return true;
    }

    return type == SGNRelationshipCache::RelationshipType::CHILD;
}

SceneGraphNode* SceneGraphNode::findChild(const stringImpl& name, bool sceneNodeName, bool recursive) const {
    SharedLock r_lock(_childLock);
    for (auto& child : _children) {
        if (sceneNodeName ? child->getNode().resourceName().compare(name) == 0
                          : child->name().compare(name) == 0)
        {
            return child;
        } else {
            if (recursive) {
                SceneGraphNode* recChild = child->findChild(name, sceneNodeName, recursive);
                if (recChild) {
                    return recChild;
                }
            }
        }
    }
    // no children's name matches or there are no more children
    // so return nullptr, indicating that the node was not found yet
    return nullptr;
}

SceneGraphNode* SceneGraphNode::findChild(I64 GUID, bool sceneNodeGuid, bool recursive) const {
    SharedLock r_lock(_childLock);
    for (auto& child : _children) {
        if (sceneNodeGuid ? (child->getNode().getGUID() == GUID) : (child->getGUID() == GUID)) {
            return child;
        } else {
            if (recursive) {
                SceneGraphNode* recChild = child->findChild(GUID, sceneNodeGuid, true);
                if (recChild) {
                    return recChild;
                }
            }
        }
    }
    // no children's name matches or there are no more children
    // so return nullptr, indicating that the node was not found yet
    return nullptr;
}

bool SceneGraphNode::intersect(const Ray& ray, F32 start, F32 end, vector<SGNRayResult>& intersections) const {
    // If we start from the root node, process children only
    if (_sceneGraph.getRoot() == *this) {
        forEachChild([&ray, start, end, &intersections](const SceneGraphNode* child, I32 /*childIdx*/) {
            child->intersect(ray, start, end, intersections);
        });
    } else {
        // If we hit an AABB, we keep going down the graph
        AABBRayResult result = get<BoundsComponent>()->getBoundingBox().intersect(ray, start, end);
        if (std::get<0>(result)) {
            intersections.push_back(std::make_tuple(getGUID(), std::get<1>(result), std::get<2>(result)));
            forEachChild([&ray, start, end, &intersections](const SceneGraphNode* child, I32 /*childIdx*/) {
                child->intersect(ray, start, end, intersections);
            });
        }
    }

    return !intersections.empty();
}

void SceneGraphNode::getOrderedNodeList(vectorEASTL<SceneGraphNode*>& nodeList) {
    // Compute from leaf to root to ensure proper calculations
    {
        SharedLock r_lock(_childLock);
        for (auto& child : _children) {
            child->getOrderedNodeList(nodeList);
        }
    }

    nodeList.push_back(this);
}

void SceneGraphNode::processDeleteQueue(vector<vec_size>& childList) {
    // See if we have any children to delete
    if (!childList.empty()) {
        UniqueLockShared w_lock(_childLock);
        for (vec_size childIdx : childList) {
            _sceneGraph.destroySceneGraphNode(_children[childIdx]);
        }
        EraseIndices(_children, childList);
    }
}

void SceneGraphNode::frameStarted() {
    Attorney::SceneNodeSceneGraph::frameStarted(_node, *this);
}

void SceneGraphNode::frameEnded() {
    Attorney::SceneNodeSceneGraph::frameEnded(_node, *this);
}

/// Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::sceneUpdate(const U64 deltaTimeUS, SceneState& sceneState) {
    // update local time
    _elapsedTimeUS += deltaTimeUS;
    _lastDeltaTimeUS = deltaTimeUS;

    if (!_relationshipCache.isValid()) {
        _relationshipCache.rebuild();
    }

    if (hasFlag(Flags::ACTIVE)) {
        if (_lockToCamera != 0) {
            TransformComponent* tComp = get<TransformComponent>();
            if (tComp) {
                Camera* cam = Camera::findCamera(_lockToCamera);
                if (cam) {
                    cam->updateLookAt();
                    tComp->setOffset(true, cam->getWorldMatrix());
                }
            }
        }

        Attorney::SceneNodeSceneGraph::sceneUpdate(*_node, deltaTimeUS, *this, sceneState);
    }
}

bool SceneGraphNode::preRender(const Camera& camera, RenderStagePass renderStagePass, bool refreshData, bool& rebuildCommandsOut) {
    return _node->preRender(*this, camera, renderStagePass, refreshData, rebuildCommandsOut);
}

bool SceneGraphNode::prepareRender(const Camera& camera, RenderStagePass renderStagePass, bool refreshData) {
    
    RenderingComponent* rComp = get<RenderingComponent>();
    if (rComp != nullptr) {
        AnimationComponent* aComp = get<AnimationComponent>();
        if (aComp) {
            std::pair<vec2<U32>, ShaderBuffer*> data = aComp->getAnimationData();
            if (data.second != nullptr) {
                RenderPackage& pkg = rComp->getDrawPackage(renderStagePass);

                ShaderBufferBinding buffer = {};
                buffer._binding = ShaderBufferLocation::BONE_TRANSFORMS;
                buffer._buffer = data.second;
                buffer._elementRange = data.first;

                pkg.addShaderBuffer(0, buffer);
            }
        }
        rComp->onRender(renderStagePass, refreshData);
    }
    return _node->onRender(*this, camera, renderStagePass, refreshData);
}

void SceneGraphNode::onRefreshNodeData(RenderStagePass renderStagePass, const Camera& camera, bool quick, GFX::CommandBuffer& bufferInOut) {
    _node->onRefreshNodeData(*this, renderStagePass, camera, quick, bufferInOut);
}

bool SceneGraphNode::getDrawState(RenderStagePass stagePass, U8 LoD) const {
    return _node->getDrawState(*this, stagePass, LoD);
}

void SceneGraphNode::onNetworkSend(U32 frameCount) {
    forEachChild([frameCount](SceneGraphNode* child, I32 /*childIdx*/) {
        child->onNetworkSend(frameCount);
    });

    NetworkingComponent* net = get<NetworkingComponent>();
    if (net) {
        net->onNetworkSend(frameCount);
    }
}


bool SceneGraphNode::preCullNode(const NodeCullParams& params,
                                 F32& distanceToClosestPointSQ) const {
    // If the node is still loading, DO NOT RENDER IT. Bad things happen :D
    if (!hasFlag(Flags::LOADING)) {
        // Use the bounding primitives to do camera/frustum checks
        BoundsComponent* bComp = get<BoundsComponent>();
        const BoundingSphere& sphere = bComp->getBoundingSphere();

        // Get camera info
        const vec3<F32>& eye = params._currentCamera->getEye();

        // Check distance to sphere edge (center - radius)
        F32 radius = sphere.getRadius();
        const vec3<F32>& center = sphere.getCenter();
        distanceToClosestPointSQ = center.distanceSquared(eye) - SQUARED(radius);
        if (distanceToClosestPointSQ < params._cullMaxDistanceSq || !getNode().isInView()) {
            if (params._minExtents.lengthSquared() > 0.0f) {
                const BoundingBox& boundingBox = bComp->getBoundingBox();
                vec3<F32> diff(boundingBox.getExtent() - params._minExtents);
                if (diff.x < 0.0f || diff.y < 0.0f || diff.z < 0.0f) {
                    return true;
                }
            }

            RenderingComponent* rComp = get<RenderingComponent>();
            const vec2<F32>& renderRange = rComp->renderRange();
            if (IS_IN_RANGE_INCLUSIVE(distanceToClosestPointSQ, SIGNED_SQUARED(renderRange.min), SQUARED(renderRange.max))) {
                if (params._minLoD > -1 && rComp != nullptr) {
                    U8 lodLevel = rComp->getLoDLevel(eye, params._stage, params._lodThresholds);
                    if (lodLevel > params._minLoD) {
                        return true;
                    }
                }

                return false;
            }
        }
    }

    return true;
}

bool SceneGraphNode::cullNode(const NodeCullParams& params,
                              Frustum::FrustCollision& collisionTypeOut,
                              F32& distanceToClosestPointSQ) const {
    // Some nodes should always render for different reasons (eg, trees are instanced and bound to the parent chunk)
    if (hasFlag(Flags::VISIBILITY_LOCKED)) {
        collisionTypeOut = Frustum::FrustCollision::FRUSTUM_IN;
        return false;
    }

    collisionTypeOut = Frustum::FrustCollision::FRUSTUM_OUT;

    if (preCullNode(params, distanceToClosestPointSQ)) {
        return true;
    }

    STUBBED("ToDo: make this work in a multi-threaded environment -Ionut");
    I8 _frustPlaneCache = -1;

    BoundsComponent* bComp = get<BoundsComponent>();
    const BoundingSphere& sphere = bComp->getBoundingSphere();
    const BoundingBox& boundingBox = bComp->getBoundingBox();
    F32 radius = sphere.getRadius();
    const vec3<F32>& center = sphere.getCenter();

    // Get camera info
    const vec3<F32>& eye = params._currentCamera->getEye();
    // Sphere is in range, so check bounds primitives againts the frustum
    if (!boundingBox.containsPoint(eye)) {
        const Frustum& frust = params._currentCamera->getFrustum();
        // Check if the bounding sphere is in the frustum, as Frustum <-> Sphere check is fast
        collisionTypeOut = frust.ContainsSphere(center, radius, _frustPlaneCache);
        if (collisionTypeOut == Frustum::FrustCollision::FRUSTUM_INTERSECT) {
            // If the sphere is not completely in the frustum, check the AABB
            _frustPlaneCache = -1;
            collisionTypeOut = frust.ContainsBoundingBox(boundingBox, _frustPlaneCache);
        }
    } else {
        // We are inside the AABB. So ... intersect?
        collisionTypeOut = Frustum::FrustCollision::FRUSTUM_INTERSECT;
    }

    return collisionTypeOut == Frustum::FrustCollision::FRUSTUM_OUT;
}

void SceneGraphNode::invalidateRelationshipCache(SceneGraphNode* source) {
    if (source == this || !_relationshipCache.isValid()) {
        return;
    }

    _relationshipCache.invalidate();

    if (_parent && _parent->parent()) {
        _parent->invalidateRelationshipCache(this);

        forEachChild([this, source](SceneGraphNode* child, I32 /*childIdx*/) {
            if (!source || child->getGUID() != source->getGUID()) {
                child->invalidateRelationshipCache(this);
            }
        });
    }
}

void SceneGraphNode::forEachChild(DELEGATE_CBK<void, SceneGraphNode*, I32>&& callback, U32 start, U32 end) {
    SharedLock r_lock(_childLock);
    const U32 childCount = getChildCountLocked();
    if (end > 0u) {
        CLAMP<U32>(end, start, childCount);
    } else {
        end = childCount;
    }
    assert(start <= end);

    for (U32 i = start; i < end; ++i) {
        callback(_children[i], i);
    }
}

void SceneGraphNode::forEachChild(DELEGATE_CBK<void, const SceneGraphNode*, I32>&& callback, U32 start, U32 end) const {
    SharedLock r_lock(_childLock);
    const U32 childCount = getChildCountLocked();
    if (end > 0u) {
        CLAMP<U32>(end, start, childCount);
    } else {
        end = childCount;
    }
    assert(start <= end);

    for (U32 i = start; i < end; ++i) {
        callback(_children[i], i);
    }
}

bool SceneGraphNode::forEachChildInterruptible(DELEGATE_CBK<bool, SceneGraphNode*, I32>&& callback, U32 start, U32 end) {
    SharedLock r_lock(_childLock);
    const U32 childCount = getChildCountLocked();
    if (end > 0u) {
        CLAMP<U32>(end, start, childCount);
    } else {
        end = childCount;
    }
    assert(start <= end);

    for (U32 i = start; i < end; ++i) {
        if (!callback(_children[i], i)) {
            return false;
        }
    }

    return true;
}

bool SceneGraphNode::forEachChildInterruptible(DELEGATE_CBK<bool, const SceneGraphNode*, I32>&& callback, U32 start, U32 end) const {
    SharedLock r_lock(_childLock);
    const U32 childCount = getChildCountLocked();
    if (end > 0u) {
        CLAMP<U32>(end, start, childCount);
    } else {
        end = childCount;
    }
    assert(start <= end);

    for (U32 i = start; i < end; ++i) {
        if (!callback(_children[i], i)) {
            return false;
        }
    }

    return true;
}

bool SceneGraphNode::saveCache(ByteBuffer& outputBuffer) const {
    getNode().saveCache(outputBuffer);

    for (EditorComponent* editorComponent : _editorComponents) {
        if (!Attorney::EditorComponentSceneGraphNode::saveCache(*editorComponent, outputBuffer)) {
            return false;
        }
    }

    if (!_sceneGraph.GetECSManager().saveCache(*this, outputBuffer)) {
        return false;
    }

    return forEachChildInterruptible([&outputBuffer](const SceneGraphNode* child, I32 /*childIdx*/) {
        return child->saveCache(outputBuffer);
    });
}

bool SceneGraphNode::loadCache(ByteBuffer& inputBuffer) {
    getNode().loadCache(inputBuffer);

    for (EditorComponent* editorComponent : _editorComponents) {
        if (!Attorney::EditorComponentSceneGraphNode::loadCache(*editorComponent, inputBuffer)) {
            return false;
        }
    }

    if (!_sceneGraph.GetECSManager().loadCache(*this, inputBuffer)) {
        return false;
    }

    return forEachChildInterruptible([&inputBuffer](SceneGraphNode* child, I32 /*childIdx*/) {
        return child->loadCache(inputBuffer);
    });
}

void SceneGraphNode::saveToXML(const stringImpl& sceneLocation) const {
    if (!serialize()) {
        return;
    }

    static boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);

    boost::property_tree::ptree pt;
    pt.put("static", usageContext() == NodeUsageContext::NODE_STATIC);

    getNode().saveToXML(pt);

    for (EditorComponent* editorComponent : _editorComponents) {
        Attorney::EditorComponentSceneGraphNode::saveToXML(*editorComponent, pt);
    }

    write_xml((sceneLocation + "/nodes/" + parent()->name() + "_" + name() + ".xml").c_str(), pt, std::locale(), settings);

    forEachChild([&sceneLocation](const SceneGraphNode* child, I32 /*childIdx*/){
        child->saveToXML(sceneLocation);
    });
}

void SceneGraphNode::loadFromXML(const boost::property_tree::ptree& pt) {
    if (!serialize()) {
        return;
    }

    usageContext(pt.get("static", false) ? NodeUsageContext::NODE_STATIC : NodeUsageContext::NODE_DYNAMIC);

    U32 componentsToLoad = 0;
    for (U8 i = 1; i < to_U8(ComponentType::COUNT); ++i) {
        ComponentType type = ComponentType::_from_integral(1 << i);
        if (pt.count(type._to_string()) > 0) {
            componentsToLoad |= type._to_integral();
        }
    }

    if (componentsToLoad != 0) {
        AddMissingComponents(componentsToLoad);
    }

    for (EditorComponent* editorComponent : _editorComponents) {
        Attorney::EditorComponentSceneGraphNode::loadFromXML(*editorComponent, pt);
    }
}

void SceneGraphNode::setFlag(Flags flag) {
    SetBit(_nodeFlags, to_U32(flag));
    if (PropagateFlagToChildren(flag)) {
        forEachChild([flag](SceneGraphNode* child, I32 /*childIdx*/) {
            child->setFlag(flag);
        });
    }
    if (flag == Flags::ACTIVE) {
        SendAndDispatchEvent<EntityActiveStateChange>(GetEntityID(), true);
    }
}

void SceneGraphNode::clearFlag(Flags flag) {
    ClearBit(_nodeFlags, to_U32(flag));
    if (PropagateFlagToChildren(flag)) {
        forEachChild([flag](SceneGraphNode* child, I32 /*childIdx*/) {
            child->clearFlag(flag);
        });
    }
    if (flag == Flags::ACTIVE) {
        SendAndDispatchEvent<EntityActiveStateChange>(GetEntityID(), false);
    }
}

bool SceneGraphNode::hasFlag(Flags flag) const {
    return BitCompare(_nodeFlags, to_U32(flag));
}

};