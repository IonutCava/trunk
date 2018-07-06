#include "stdafx.h"

#include "Headers/SceneGraph.h"

#include "Core/Headers/PlatformContext.h"
#include "Scenes/Headers/SceneState.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"
#include "Environment/Water/Headers/Water.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

SceneGraphNode::SceneGraphNode(SceneGraph& sceneGraph,
                               PhysicsGroup physicsGroup,
                               const SceneNode_ptr& node,
                               const stringImpl& name,
                               U32 componentMask)
    : GUIDWrapper(),
      _frustPlaneCache(-1),
      _sceneGraph(sceneGraph),
      _updateTimer(Time::ElapsedMilliseconds()),
      _elapsedTime(0ULL),
      _node(node),
      _active(true),
      _visibilityLocked(false),
      _isSelectable(false),
      _selectionFlag(SelectionFlag::SELECTION_NONE),
      _usageContext(UsageContext::NODE_DYNAMIC),
      _relationshipCache(*this)
{
    assert(_node != nullptr);
    _children.resize(INITIAL_CHILD_COUNT);
    _childCount = 0;

    for (std::atomic_bool& flag : _updateFlags) {
        flag = false;
    }

    setName(name);
    _components.fill(nullptr);

    if (BitCompare(componentMask, to_U32(SGNComponent::ComponentType::ANIMATION))) {
        setComponent(SGNComponent::ComponentType::ANIMATION, new AnimationComponent(*this));
    }
    if (BitCompare(componentMask, to_U32(SGNComponent::ComponentType::INVERSE_KINEMATICS))) {
        setComponent(SGNComponent::ComponentType::INVERSE_KINEMATICS, new IKComponent(*this));
    }
    if (BitCompare(componentMask, to_U32(SGNComponent::ComponentType::NETWORKING))) {
        LocalClient& client = _sceneGraph.parentScene().platformContext().client();
        setComponent(SGNComponent::ComponentType::NETWORKING, new NetworkingComponent(*this, client));
    }
    if (BitCompare(componentMask, to_U32(SGNComponent::ComponentType::RAGDOLL))) {
        setComponent(SGNComponent::ComponentType::RAGDOLL, new RagdollComponent(*this));
    }
    if (BitCompare(componentMask, to_U32(SGNComponent::ComponentType::NAVIGATION))) {
        setComponent(SGNComponent::ComponentType::NAVIGATION, new NavigationComponent(*this));
    }
    if (BitCompare(componentMask, to_U32(SGNComponent::ComponentType::PHYSICS))) {
        STUBBED("Rigid body physics disabled for now - Ionut");
        physicsGroup = PhysicsGroup::GROUP_IGNORE;
        PXDevice& pxContext = _sceneGraph.parentScene().platformContext().pfx();
        setComponent(SGNComponent::ComponentType::PHYSICS, new PhysicsComponent(*this, physicsGroup, pxContext));

        PhysicsComponent* pComp = get<PhysicsComponent>();
        pComp->addTransformUpdateCbk([this]() { Attorney::SceneGraphSGN::onNodeTransform(_sceneGraph, *this); });
        pComp->addTransformUpdateCbk([this]() { onTransform(); });
    }

    if (BitCompare(componentMask, to_U32(SGNComponent::ComponentType::BOUNDS))) {
        setComponent(SGNComponent::ComponentType::BOUNDS, new BoundsComponent(*this));
    }

    if (BitCompare(componentMask, to_U32(SGNComponent::ComponentType::UNIT))) {
        setComponent(SGNComponent::ComponentType::UNIT, new UnitComponent(*this));
    }
    
    if (BitCompare(componentMask, to_U32(SGNComponent::ComponentType::RENDERING))) {
        GFXDevice& gfxContext = _sceneGraph.parentScene().platformContext().gfx();

        const Material_ptr& materialTpl = _node->getMaterialTpl();
        setComponent(SGNComponent::ComponentType::RENDERING, 
                     new RenderingComponent(gfxContext,
                                            materialTpl ? materialTpl->clone("_instance_" + name)
                                                        : nullptr,
                                            *this));
    }

    Attorney::SceneNodeSceneGraph::registerSGNParent(*_node, getGUID());
}

/// If we are destroying the current graph node
SceneGraphNode::~SceneGraphNode()
{
    if (getParent().lock()) {
        Attorney::SceneGraphSGN::onNodeDestroy(_sceneGraph, *this);
    }
    Console::printfn(Locale::get(_ID("REMOVE_SCENEGRAPH_NODE")),
        getName().c_str(), _node->getName().c_str());

    if (Config::Build::IS_DEBUG_BUILD) {
        for (U32 i = 0; i < getChildCount(); ++i) {
            DIVIDE_ASSERT(_children[i].unique(), "SceneGraphNode::~SceneGraphNode error: child still in use!");
        }
    }

    Attorney::SceneNodeSceneGraph::unregisterSGNParent(*_node, getGUID());

    if (Attorney::SceneNodeSceneGraph::parentCount(*_node) == 0) {
        _node.reset();
    }
    

    for (SGNComponent* component : _components) {
        MemoryManager::DELETE(component);
    }
}

void SceneGraphNode::setComponent(SGNComponent::ComponentType type, SGNComponent* component) {
    // We have a component registered for the specified slot
    if (getComponent(type) != nullptr) {
        if (component != nullptr) {
            // We want to replace the existing entry, so keep the same index
        } else {
            // We want to delete the existing entry, so destroy the index as well
        }
    } else {
        // We are adding a new component type
    }

    MemoryManager::SAFE_UPDATE(_components[getComponentIdx(type)], component);
}

void SceneGraphNode::usageContext(const UsageContext& newContext) {
    Attorney::SceneGraphSGN::onNodeUsageChange(_sceneGraph,
                                               *this,
                                               _usageContext,
                                               newContext);
    _usageContext = newContext;
}

/// Change current SceneGraphNode's parent
void SceneGraphNode::setParent(SceneGraphNode& parent) {
    assert(parent.getGUID() != getGUID());
    SceneGraphNode_ptr parentPtr = _parent.lock();
    if (parentPtr) {
        if (*parentPtr == parent) {
            return;
        }
        // Remove us from the old parent's children map
        parentPtr->removeNode(*this);
    }
    // Set the parent pointer to the new parent
    _parent = parent.shared_from_this();
    // Add ourselves in the new parent's children map
    parent.registerNode(shared_from_this());
    invalidateRelationshipCache();
    // That's it. Parent Transforms will be updated in the next render pass;
}

SceneGraphNode_ptr SceneGraphNode::registerNode(SceneGraphNode_ptr node) {
    // Time to add it to the children vector
    SceneGraphNode_cptr child = findChild(node->getName()).lock();
    if (child) {
        removeNode(*child);
    }
    
    Attorney::SceneGraphSGN::onNodeAdd(_sceneGraph, *node);
    
    WriteLock w_lock(_childLock);
    if (_childCount == _children.size()) {
        _children.push_back(node);
    } else {
        _children[_childCount] = node;
    }
    _childCount = _childCount + 1;

    return node;
}

/// Add a new SceneGraphNode to the current node's child list based on a SceneNode
SceneGraphNode_ptr SceneGraphNode::addNode(const SceneNode_ptr& node, U32 componentMask, PhysicsGroup physicsGroup, const stringImpl& name) {
    assert(node);
    // Create a new SceneGraphNode with the SceneNode's info
    // We need to name the new SceneGraphNode
    // If we did not supply a custom name use the SceneNode's name
    SceneGraphNode_ptr sceneGraphNode = 
        std::make_shared<SceneGraphNode>(_sceneGraph, 
                                         physicsGroup,
                                         node,
                                         name.empty() ? node->getName()
                                                      : name,
                                         componentMask);
    // Set the current node as the new node's parent
    sceneGraphNode->setParent(*this);
    if (node->getState() == ResourceState::RES_LOADED) {
        // Do all the post load operations on the SceneNode
        // Pass a reference to the newly created SceneGraphNode in case we need
        // transforms or bounding boxes
        Attorney::SceneNodeSceneGraph::postLoad(*node, *sceneGraphNode);
        invalidateRelationshipCache();
    } else if (node->getState() == ResourceState::RES_LOADING) {
        setUpdateFlag(UpdateFlag::THREADED_LOAD);

        SceneGraphNode_wptr callbackPtr = sceneGraphNode;

        node->setStateCallback(ResourceState::RES_LOADED,
            [this, callbackPtr](Resource_wptr res) {
                Attorney::SceneNodeSceneGraph::postLoad(*(std::dynamic_pointer_cast<SceneNode>(res.lock())), *(callbackPtr.lock()));
                invalidateRelationshipCache();
                clearUpdateFlag(UpdateFlag::THREADED_LOAD);

            }
        );
    }
    // return the newly created node
    return sceneGraphNode;
}

bool SceneGraphNode::removeNodesByType(SceneNodeType nodeType) {
    // Bottom-up pattern
    U32 removalCount = 0, childRemovalCount = 0;
    forEachChild([nodeType, &childRemovalCount](SceneGraphNode& child) {
        if (child.removeNodesByType(nodeType)) {
            ++childRemovalCount;
        }
    });

    bool stopSearching = false;
    UpgradableReadLock ur_lock(_childLock);
    while (!stopSearching) {
        stopSearching = true;
        for (U32 i = 0; i < _childCount; ++i) {
            if (_children[i]->getNode()->getType() == nodeType) {
                UpgradeToWriteLock w_lock(ur_lock);
                _children[i].reset();
                _childCount = _childCount - 1;
                std::swap(_children[_childCount], _children[i]);
                stopSearching = false;
                ++removalCount;
                break;
            }
        }
    }

    if (removalCount > 0) {
        invalidateRelationshipCache();
        return true;
    }

    return childRemovalCount > 0;
}

bool SceneGraphNode::removeNode(const SceneGraphNode& node) {
    // Beware. Removing a node, does no delete it!
    // Call delete on the SceneGraphNode's pointer to do that

    UpgradableReadLock ur_lock(_childLock);
    
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        if (_children[i]->getGUID() == node.getGUID()) {
            {
                UpgradeToWriteLock w_lock(ur_lock);
                _children[i].reset();
                _childCount = _childCount - 1;
                std::swap(_children[_childCount], _children[i]);
            }
            invalidateRelationshipCache();
            return true;
        }
    }
    

    // If this didn't finish, it means that we found our node
    return !forEachChildInterruptible([&node](SceneGraphNode& child) {
                    if (child.removeNode(node)) {
                        return false;
                    }
                    return true;
                });
}

void SceneGraphNode::postLoad() {
    for (SGNComponent* component : _components) {
        if (component != nullptr) {
            component->postLoad();
        }
    }
}

bool SceneGraphNode::isChildOfType(U32 typeMask, bool ignoreRoot) const {
    if (ignoreRoot) {
        ClearBit(typeMask, to_base(SceneNodeType::TYPE_ROOT));
    }
    SceneGraphNode_ptr parent = getParent().lock();
    while (parent != nullptr) {
        if (BitCompare(typeMask, to_U32(parent->getNode<>()->getType()))) {
            return true;
        }
        parent = parent->getParent().lock();
    }

    return false;
}

bool SceneGraphNode::isRelated(const SceneGraphNode& target) const {
    I64 targetGUID = target.getGUID();

    SGNRelationshipCache::RelationShipType type = _relationshipCache.clasifyNode(targetGUID);

    // We also ignore grandparents as this will usually be the root;
    return type != SGNRelationshipCache::RelationShipType::COUNT;
}

bool SceneGraphNode::isChild(const SceneGraphNode& target, bool recursive) const {
    I64 targetGUID = target.getGUID();

    SGNRelationshipCache::RelationShipType type = _relationshipCache.clasifyNode(targetGUID);
    if (type == SGNRelationshipCache::RelationShipType::GRANDCHILD && recursive) {
        return true;
    }

    return type == SGNRelationshipCache::RelationShipType::CHILD;
}

SceneGraphNode_wptr SceneGraphNode::findChild(I64 GUID, bool recursive) const {
    U32 childCount = getChildCount();
    ReadLock r_lock(_childLock);
    for (U32 i = 0; i < childCount; ++i) {
        SceneGraphNode& child = *_children[i];
        if (child.getGUID() == GUID) {
            return child.shared_from_this();
        } else {
            if (recursive) {
                SceneGraphNode_wptr recChild = child.findChild(GUID, recursive);
                if (recChild.lock()) {
                    return recChild;
                }
            }
        }
    }
    // no children's name matches or there are no more children
    // so return nullptr, indicating that the node was not found yet
    return SceneGraphNode_wptr();
}

SceneGraphNode_wptr SceneGraphNode::findChild(const stringImpl& name, bool sceneNodeName, bool recursive) const {
    U32 childCount = getChildCount();
    ReadLock r_lock(_childLock);
    for (U32 i = 0; i < childCount; ++i) {
        SceneGraphNode& child = *_children[i];
        if (sceneNodeName ? child.getNode()->getName().compare(name) == 0
                          : child.getName().compare(name) == 0)
        {
            return child.shared_from_this();
        } else {
            if (recursive) {
                SceneGraphNode_wptr recChild = child.findChild(name, sceneNodeName, recursive);
                if (recChild.lock()) {
                    return recChild;
                }
            }
        }
    }
    // no children's name matches or there are no more children
    // so return nullptr, indicating that the node was not found yet
    return SceneGraphNode_wptr();
}

void SceneGraphNode::intersect(const Ray& ray,
                               F32 start,
                               F32 end,
                               vectorImpl<I64>& selectionHits,
                               bool recursive) const {

    if (isSelectable()) {
        if (get<BoundsComponent>()->getBoundingBox().intersect(ray, start, end)) {
            selectionHits.push_back(getGUID());
        }
    }

    if (recursive) {
        forEachChild([&ray, start, end, &selectionHits](const SceneGraphNode& child) {
            child.intersect(ray, start, end, selectionHits);
        });
    }
}

void SceneGraphNode::setSelectionFlag(SelectionFlag flag) {
    if (_selectionFlag != flag) {
        _selectionFlag = flag;
        forEachChild([flag](SceneGraphNode& child) {
            child.setSelectionFlag(flag);
        });
    }
}

void SceneGraphNode::setSelectable(const bool state) {
    if (_isSelectable != state) {
        _isSelectable = state;
        forEachChild([state](SceneGraphNode& child) {
            child.setSelectable(state);
        });
    }
}

void SceneGraphNode::lockVisibility(const bool state) {
    if (_visibilityLocked != state) {
        _visibilityLocked = state;
        forEachChild([state](SceneGraphNode& child) {
            child.lockVisibility(state);
        });
    }
}

void SceneGraphNode::setActive(const bool state) {
    if (_active != state) {
        _active = state;
        for (SGNComponent* component : _components) {
            if (component != nullptr) {
                component->setActive(state);
            }
        }

        forEachChild([state](SceneGraphNode& child) {
            child.setActive(state);
        });
    }
}

void SceneGraphNode::getOrderedNodeList(vectorImpl<SceneGraphNode*>& nodeList) {
    // Compute from leaf to root to ensure proper calculations
    forEachChild([&nodeList](SceneGraphNode& child) {
        child.getOrderedNodeList(nodeList);
    });

    const SceneNode_ptr& node = getNode();
    if (!node || (node && node->getState() == ResourceState::RES_LOADED)) {
        nodeList.push_back(this);
    }
}

void SceneGraphNode::sgnUpdate(const U64 deltaTime, SceneState& sceneState) {
    Attorney::SceneNodeSceneGraph::sgnUpdate(*_node, deltaTime, *this, sceneState);
}

/// Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::sceneUpdate(const U64 deltaTime, SceneState& sceneState) {
    assert(_node->getState() == ResourceState::RES_LOADED);

    // update local time
    _elapsedTime += deltaTime;

    // update all of the internal components (animation, physics, etc)
    for (SGNComponent* component : _components) {
        if (component != nullptr) {
            component->update(deltaTime);
         }
    }

    if (!_relationshipCache.isValid()) {
        _relationshipCache.rebuild();
    }

    Attorney::SceneNodeSceneGraph::sceneUpdate(*_node, deltaTime, *this, sceneState);
}

void SceneGraphNode::onTransform() {
    forEachChild([](SceneGraphNode& child) {
        child.onTransform();
    });

    PhysicsComponent* pComp = get<PhysicsComponent>();
    BoundsComponent* bComp = get<BoundsComponent>();
    if (pComp != nullptr && bComp != nullptr) {
        bComp->onTransform(pComp->getWorldMatrix());
    }
}

bool SceneGraphNode::prepareRender(const SceneRenderState& sceneRenderState,
                                   const RenderStagePass& renderStagePass) {
    for (SGNComponent* component : _components) {
        if (component && !component->onRender(sceneRenderState, renderStagePass)) {
            return false;
        }
    }

    return _node->onRender(*this, sceneRenderState, renderStagePass);
}

void SceneGraphNode::onCameraUpdate(const I64 cameraGUID,
                                    const vec3<F32>& posOffset,
                                    const mat4<F32>& rotationOffset) {

    forEachChild([cameraGUID, &posOffset, &rotationOffset](SceneGraphNode& child) {
        child.onCameraUpdate(cameraGUID, posOffset, rotationOffset);
    });

    PhysicsComponent* pComp = get<PhysicsComponent>();
    if (pComp && pComp->ignoreView(cameraGUID)) {
        pComp->setViewOffset(posOffset, rotationOffset);
    }
    
    Attorney::SceneNodeSceneGraph::onCameraUpdate(*this, *_node, cameraGUID, posOffset, rotationOffset);
}

void SceneGraphNode::onCameraChange(const Camera& cam) {
    forEachChild([&cam](SceneGraphNode& child) {
        child.onCameraChange(cam);
    });

    Attorney::SceneNodeSceneGraph::onCameraChange(*this, *_node, cam);
}

void SceneGraphNode::onNetworkSend(U32 frameCount) {
    forEachChild([frameCount](SceneGraphNode& child) {
        child.onNetworkSend(frameCount);
    });

    NetworkingComponent* net = get<NetworkingComponent>();
    if (net) {
        net->onNetworkSend(frameCount);
    }
}

bool SceneGraphNode::filterCollission(const SceneGraphNode& node) {
    // filter by mask, type, etc
    return true;
}

void SceneGraphNode::onCollision(SceneGraphNode_cwptr collider) {
    //handle collision
    if (_collisionCbk) {
        SceneGraphNode_cptr node = collider.lock();
        if (node && filterCollission(*node)) {
            assert(getGUID() != node->getGUID());
            _collisionCbk(node);
        }
    }
}

bool SceneGraphNode::cullNode(const Camera& currentCamera,
                              F32 maxDistanceFromCamera,
                              RenderStage currentStage,
                              Frustum::FrustCollision& collisionTypeOut) const {

    if (getFlag(UpdateFlag::THREADED_LOAD)) {
        return true;
    }

    collisionTypeOut = Frustum::FrustCollision::FRUSTUM_IN;
    if (visibilityLocked()) {
        return false;
    }

    bool distanceCheck = currentStage != RenderStage::SHADOW;
    const BoundingBox& boundingBox = get<BoundsComponent>()->getBoundingBox();
    const BoundingSphere& sphere = get<BoundsComponent>()->getBoundingSphere();

    F32 radius = sphere.getRadius();
    const vec3<F32>& eye = currentCamera.getEye();
    const vec3<F32>& center = sphere.getCenter();
    F32 visibilityDistanceSq = std::pow(maxDistanceFromCamera + radius, 2);

    if (distanceCheck && center.distanceSquared(eye) > visibilityDistanceSq) {
        if (boundingBox.nearestDistanceFromPointSquared(eye) >
            std::min(visibilityDistanceSq, std::pow(currentCamera.getZPlanes().y, 2))) {
            return true;
        }
    }

    if (!boundingBox.containsPoint(eye)) {
        const Frustum& frust = currentCamera.getFrustum();
        collisionTypeOut = frust.ContainsSphere(center, radius, _frustPlaneCache);
        if (collisionTypeOut == Frustum::FrustCollision::FRUSTUM_INTERSECT) {
            collisionTypeOut = frust.ContainsBoundingBox(boundingBox, _frustPlaneCache);
        }
    }

    return collisionTypeOut == Frustum::FrustCollision::FRUSTUM_OUT;
}

void SceneGraphNode::invalidateRelationshipCache() {
    _relationshipCache.invalidate();

    SceneGraphNode_ptr parentPtr = _parent.lock();
    if (parentPtr && parentPtr->getParent().lock()) {
        parentPtr->invalidateRelationshipCache();

        forEachChild([](SceneGraphNode& child) {
            child.invalidateRelationshipCache();
        });
    }
}

void SceneGraphNode::forEachChild(const DELEGATE_CBK<void, SceneGraphNode&>& callback) {
    ReadLock r_lock(_childLock);
    //Using childCount instead of a range-based for loops means that we can skip child null check
    //as childCount should always be updated to reflect the actual number of node children
    U32 childCount = _childCount;
    for (U32 i = 0; i < childCount; ++i) {
        callback(*_children[i]);
    }
}

void SceneGraphNode::forEachChild(const DELEGATE_CBK<void, const SceneGraphNode&>& callback) const {
    ReadLock r_lock(_childLock);
    U32 childCount = _childCount;
    for (U32 i = 0; i < childCount; ++i) {
        callback(*_children[i]);
    }
}

bool SceneGraphNode::forEachChildInterruptible(const DELEGATE_CBK<bool, SceneGraphNode&>& callback) {
    ReadLock r_lock(_childLock);
    U32 childCount = _childCount;
    for (U32 i = 0; i < childCount; ++i) {
        if (!callback(*_children[i])) {
            return false;
        }
    }

    return true;
}

bool SceneGraphNode::forEachChildInterruptible(const DELEGATE_CBK<bool, const SceneGraphNode&>& callback) const {
    ReadLock r_lock(_childLock);
    U32 childCount = _childCount;
    for (U32 i = 0; i < childCount; ++i) {
        if (!callback(*_children[i])) {
            return false;
        }
    }

    return true;
}

void SceneGraphNode::forEachChild(const DELEGATE_CBK<void, SceneGraphNode&, I32>& callback, U32 start, U32 end) {
    ReadLock r_lock(_childLock);
    U32 childCount = _childCount;
    CLAMP<U32>(end, 0, childCount);
    assert(start < end);

    for (U32 i = start; i < end; ++i) {
        callback(*_children[i], i);
    }
}

void SceneGraphNode::forEachChild(const DELEGATE_CBK<void, const SceneGraphNode&, I32>& callback, U32 start, U32 end) const {
    ReadLock r_lock(_childLock);
    U32 childCount = _childCount;
    CLAMP<U32>(end, 0, childCount);
    assert(start < end);

    for (U32 i = start; i < end; ++i) {
        callback(*_children[i], i);
    }
}

bool SceneGraphNode::forEachChildInterruptible(const DELEGATE_CBK<bool, SceneGraphNode&, I32>& callback, U32 start, U32 end) {
    ReadLock r_lock(_childLock);
    U32 childCount = _childCount;
    CLAMP<U32>(end, 0, childCount);
    assert(start < end);

    for (U32 i = start; i < end; ++i) {
        if (!callback(*_children[i], i)) {
            return false;
        }
    }

    return true;
}

bool SceneGraphNode::forEachChildInterruptible(const DELEGATE_CBK<bool, const SceneGraphNode&, I32>& callback, U32 start, U32 end) const {
    ReadLock r_lock(_childLock);
    U32 childCount = _childCount;
    CLAMP<U32>(end, 0, childCount);
    assert(start < end);

    for (U32 i = start; i < end; ++i) {
        if (!callback(*_children[i], i)) {
            return false;
        }
    }

    return true;
}
};