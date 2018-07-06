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
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

#include "Editor/Headers/Editor.h"

#include "ECS/Events/Headers/EntityEvents.h"
#include "ECS/Events/Headers/BoundsEvents.h"
#include "ECS/Events/Headers/TransformEvents.h"
#include "ECS/Systems/Headers/ECSManager.h"

namespace Divide {

SceneGraphNode::SceneGraphNode(SceneGraph& sceneGraph, const SceneGraphNodeDescriptor& descriptor)
    : GUIDWrapper(),
      ECS::Event::IEventListener(&sceneGraph.GetECSEngine()),
      _sceneGraph(sceneGraph),
      _node(descriptor._node),
      _componentMask(descriptor._componentMask),
      _usageContext(descriptor._usageContext),
      _selectionFlag(SelectionFlag::SELECTION_NONE),
      _parent(nullptr),
      _frustPlaneCache(-1),
      _elapsedTimeUS(0ULL),
      _updateFlags(0U),
      _active(true),
      _visibilityLocked(false),
      _lockToCamera(0),
      _relationshipCache(*this)
{
    assert(_node != nullptr);
    _children.reserve(INITIAL_CHILD_COUNT);
    RegisterEventCallbacks();
    
    name(descriptor._name.empty() ? Util::StringFormat("%s_SGN_%d", _node->name().c_str(), getGUID()) : descriptor._name);

    if (BitCompare(_componentMask, to_U32(ComponentType::ANIMATION))) {
        AddSGNComponent<AnimationComponent>(*this);
    }

    if (BitCompare(_componentMask, to_U32(ComponentType::INVERSE_KINEMATICS))) {
        AddSGNComponent<IKComponent>(*this);
    }
    if (BitCompare(_componentMask, to_U32(ComponentType::NETWORKING))) {
        LocalClient& client = _sceneGraph.parentScene().context().client();
        AddSGNComponent<NetworkingComponent>(*this, client);
    }
    if (BitCompare(_componentMask, to_U32(ComponentType::RAGDOLL))) {
        AddSGNComponent<RagdollComponent>(*this);
    }
    if (BitCompare(_componentMask, to_U32(ComponentType::NAVIGATION))) {
        AddSGNComponent<NavigationComponent>(*this);
    }
    if (BitCompare(_componentMask, to_U32(ComponentType::RIGID_BODY))) {
        PXDevice& pxContext = _sceneGraph.parentScene().context().pfx();

        STUBBED("Rigid body physics disabled for now - Ionut");
        AddSGNComponent<RigidBodyComponent>(*this, pxContext);
    }
    if (BitCompare(_componentMask, to_U32(ComponentType::TRANSFORM))) {
        AddSGNComponent<TransformComponent>(*this);
    }

    if (BitCompare(_componentMask, to_U32(ComponentType::BOUNDS))) {
        AddSGNComponent<BoundsComponent>(*this);
    }

    if (BitCompare(_componentMask, to_U32(ComponentType::UNIT))) {
        AddSGNComponent<UnitComponent>(*this);
    }
    
    if (BitCompare(_componentMask, to_U32(ComponentType::RENDERING))) {
        GFXDevice& gfxContext = _sceneGraph.parentScene().context().gfx();

        const Material_ptr& materialTpl = _node->getMaterialTpl();
        AddSGNComponent<RenderingComponent>(gfxContext,
                                            materialTpl ? materialTpl->clone("_instance_" + name())
                                                        : nullptr,
                                            *this);
    }

    Attorney::SceneNodeSceneGraph::registerSGNParent(*_node, this);
}

/// If we are destroying the current graph node
SceneGraphNode::~SceneGraphNode()
{
    // Bottom up
    for (U32 i = 0; i < getChildCount(); ++i) {
        parentGraph().destroySceneGraphNode(_children[i]);
    }
    _children.clear();

    UnregisterAllEventCallbacks();

    if (getParent()) {
        Attorney::SceneGraphSGN::onNodeDestroy(_sceneGraph, *this);
    }

    Console::printfn(Locale::get(_ID("REMOVE_SCENEGRAPH_NODE")), name().c_str(), _node->name().c_str());

    RemoveAllSGNComponents();

    Attorney::SceneNodeSceneGraph::unregisterSGNParent(*_node, this);

    if (Attorney::SceneNodeSceneGraph::parentCount(*_node) == 0) {
        _node.reset();
    }
}


void SceneGraphNode::RegisterEventCallbacks()
{
    
}

void SceneGraphNode::onBoundsUpdated() {
    SceneGraphNode* parent = getParent();
    if (parent) {
        parent->onBoundsUpdated();
    }
}

void SceneGraphNode::setTransformDirty(U32 transformMask) {
    Attorney::SceneGraphSGN::onNodeTransform(_sceneGraph, *this);

    ReadLock r_lock(_childLock);
    U32 childCount = getChildCountLocked();
    for (U32 i = 0; i < childCount; ++i) {
        _children[i]->setParentTransformDirty(transformMask);
    }
}

void SceneGraphNode::setParentTransformDirty(U32 transformMask) {
    TransformComponent* tComp = get<TransformComponent>();
    if (tComp) {
        Attorney::TransformComponentSGN::onParentTransformDirty(*tComp, transformMask);
    }
}

void SceneGraphNode::usageContext(const NodeUsageContext& newContext) {
    TransformComponent* tComp = get<TransformComponent>();
    if (tComp) {
        Attorney::TransformComponentSGN::onParentUsageChanged(*tComp, _usageContext);
    }
}

/// Change current SceneGraphNode's parent
void SceneGraphNode::setParent(SceneGraphNode& parent) {
    assert(parent.getGUID() != getGUID());
    
    if (_parent) {
        if (*_parent == parent) {
            return;
        }
        // Remove us from the old parent's children map
        _parent->removeNode(*this);
    }
    // Set the parent pointer to the new parent
    _parent = &parent;
    // Add ourselves in the new parent's children map
    parent.registerNode(this);
    invalidateRelationshipCache();
    // That's it. Parent Transforms will be updated in the next render pass;
}

SceneGraphNode* SceneGraphNode::registerNode(SceneGraphNode* node) {
    // Time to add it to the children vector
    SceneGraphNode* child = findChild(node->name());
    if (child) {
        removeNode(*child);
    }
    
    Attorney::SceneGraphSGN::onNodeAdd(_sceneGraph, *node);
    
    WriteLock w_lock(_childLock);
    _children.push_back(node);

    return _children.back();
}

/// Add a new SceneGraphNode to the current node's child list based on a SceneNode
SceneGraphNode* SceneGraphNode::addNode(const SceneGraphNodeDescriptor& descriptor) {
    assert(descriptor._node);
    // Create a new SceneGraphNode with the SceneNode's info
    // We need to name the new SceneGraphNode
    // If we did not supply a custom name use the SceneNode's name

    SceneGraphNode* sceneGraphNode = parentGraph().createSceneGraphNode(_sceneGraph, descriptor);

    // Set the current node as the new node's parent
    sceneGraphNode->setParent(*this);
    if (sceneGraphNode->_node->getState() == ResourceState::RES_LOADED) {
        // Do all the post load operations on the SceneNode
        // Pass a reference to the newly created SceneGraphNode in case we need
        // transforms or bounding boxes
        Attorney::SceneNodeSceneGraph::postLoad(*sceneGraphNode->_node, *sceneGraphNode);
        invalidateRelationshipCache();
    } else if (sceneGraphNode->_node->getState() == ResourceState::RES_LOADING) {
        setUpdateFlag(UpdateFlag::THREADED_LOAD);

        SceneGraphNode* callbackPtr = sceneGraphNode;

        sceneGraphNode->_node->setStateCallback(ResourceState::RES_LOADED,
            [this, callbackPtr](Resource_wptr res) {
                Attorney::SceneNodeSceneGraph::postLoad(*(std::dynamic_pointer_cast<SceneNode>(res.lock())), *(callbackPtr));
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

    ReadLock r_lock(_childLock);
    for (U32 i = 0; i < getChildCountLocked(); ++i) {
        if (_children[i]->getNode()->type() == nodeType) {
            {
                addToDeleteQueue(i);
                ++removalCount;
            }
        }
    }

    if (removalCount > 0) {
        return true;
    }

    return childRemovalCount > 0;
}

bool SceneGraphNode::removeNode(const SceneGraphNode& node) {

    I64 targetGUID = node.getGUID();
    ReadLock r_lock(_childLock);
    U32 childCount = getChildCountLocked();
    for (U32 i = 0; i < childCount; ++i) {
        if (_children[i]->getGUID() == targetGUID) {
            {
                addToDeleteQueue(i);
            }

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
    SendEvent<EntityPostLoad>(GetEntityID());
}

bool SceneGraphNode::isChildOfType(U32 typeMask, bool ignoreRoot) const {
    if (ignoreRoot) {
        ClearBit(typeMask, to_base(SceneNodeType::TYPE_ROOT));
    }
    SceneGraphNode* parent = getParent();
    while (parent != nullptr) {
        if (BitCompare(typeMask, to_U32(parent->getNode<>()->getType()))) {
            return true;
        }
        parent = parent->getParent();
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

SceneGraphNode* SceneGraphNode::findChild(I64 GUID, bool recursive) const {
    ReadLock r_lock(_childLock);
    for (auto& child : _children) {
        if (child->getGUID() == GUID) {
            return child;
        } else {
            if (recursive) {
                SceneGraphNode* recChild = child->findChild(GUID, recursive);
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

SceneGraphNode* SceneGraphNode::findChild(const stringImpl& name, bool sceneNodeName, bool recursive) const {
    ReadLock r_lock(_childLock);
    for (auto& child : _children) {
        if (sceneNodeName ? child->getNode()->name().compare(name) == 0
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

void SceneGraphNode::intersect(const Ray& ray,
                               F32 start,
                               F32 end,
                               bool force,
                               vector<SGNRayResult>& selectionHits,
                               bool recursive) const {

    SelectionComponent* selComp = get<SelectionComponent>();
    if (force || (selComp && selComp->enabled())) {
        AABBRayResult result = get<BoundsComponent>()->getBoundingBox().intersect(ray, start, end);
        if (std::get<0>(result)) {
            selectionHits.push_back(std::make_tuple(getGUID(), std::get<1>(result), std::get<2>(result)));
        }
    }

    if (recursive) {
        forEachChild([&ray, start, end, force, &selectionHits](const SceneGraphNode& child) {
            child.intersect(ray, start, end, force, selectionHits);
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
        SendAndDispatchEvent<EntityActiveStateChange>(GetEntityID(), _active);

        forEachChild([state](SceneGraphNode& child) {
            child.setActive(state);
        });
    }
}

void SceneGraphNode::getOrderedNodeList(vector<SceneGraphNode*>& nodeList) {
    // Compute from leaf to root to ensure proper calculations
    {
        ReadLock r_lock(_childLock);
        for (auto& child : _children) {
            child->getOrderedNodeList(nodeList);
        }
    }

    nodeList.push_back(this);
}

void SceneGraphNode::processDeleteQueue(vector<vec_size>& childList) {
    // See if we have any children to delete
    if (!childList.empty()) {
        WriteLock w_lock(_childLock);
        for (vec_size childIdx : childList) {
            parentGraph().destroySceneGraphNode(_children[childIdx]);
        }
        _children = erase_indices(_children, childList);
    }
}

void SceneGraphNode::addToDeleteQueue(U32 idx) {
    parentGraph().addToDeleteQueue(this, idx);
}

/// Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::sceneUpdate(const U64 deltaTimeUS, SceneState& sceneState) {
    assert(_node->getState() == ResourceState::RES_LOADED);

    // update local time
    _elapsedTimeUS += deltaTimeUS;

    if (!_relationshipCache.isValid()) {
        _relationshipCache.rebuild();
    }

    if (isActive()) {
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

bool SceneGraphNode::prepareRender(const SceneRenderState& sceneRenderState,
                                   RenderStagePass renderStagePass) {
    
    RenderingComponent* rComp = get<RenderingComponent>();
    if (rComp != nullptr) {
        AnimationComponent* aComp = get<AnimationComponent>();
        if (aComp) {
            std::pair<vec2<U32>, ShaderBuffer*> data = aComp->getAnimationData();
            if (data.second != nullptr) {
                rComp->registerShaderBuffer(ShaderBufferLocation::BONE_TRANSFORMS, data.first, *data.second);
            }
        }
        rComp->onRender(renderStagePass);
    }
    return _node->onRender(*this, sceneRenderState, renderStagePass);
}

void SceneGraphNode::onCameraUpdate(const U64 cameraNameHash,
                                    const vec3<F32>& cameraEye,
                                    const mat4<F32>& cameraView) {

    forEachChild([cameraNameHash, &cameraEye, &cameraView](SceneGraphNode& child) {
        child.onCameraUpdate(cameraNameHash, cameraEye, cameraView);
    });
    
    Attorney::SceneNodeSceneGraph::onCameraUpdate(*this, *_node, cameraNameHash, cameraEye, cameraView);
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


bool SceneGraphNode::cullNode(const Camera& currentCamera,
                              F32 maxDistanceFromCameraSq,
                              RenderStage currentStage,
                              Frustum::FrustCollision& collisionTypeOut,
                              F32& minDistanceSq) const {

    // If the node is still loading, DO NOT RENDER IT. Bad things happen :D
    if (getFlag(UpdateFlag::THREADED_LOAD)) {
        return true;
    }

    // Some nodes should always render for ... reasons
    if (visibilityLocked()) {
        collisionTypeOut = Frustum::FrustCollision::FRUSTUM_IN;
        return false;
    }

    collisionTypeOut = Frustum::FrustCollision::FRUSTUM_OUT;

    // Use the bounding primitives to do camera/frustum checks
    BoundsComponent* bComp = get<BoundsComponent>();
    const BoundingBox& boundingBox = bComp->getBoundingBox();
    const BoundingSphere& sphere = bComp->getBoundingSphere();

    // Get camera info
    F32 radius = sphere.getRadius();
    F32 radiusSq = SQUARED(radius);
    const vec3<F32>& eye = currentCamera.getEye();
    
    // Check distance to sphere edge (center - radius)
    const vec3<F32>& center = sphere.getCenter();
    minDistanceSq = center.distanceSquared(eye) - radiusSq;
    if (minDistanceSq > maxDistanceFromCameraSq) {
        return true;
    }

    // Sphere is in range, so check bounds primitives againts the frustum
    if (!boundingBox.containsPoint(eye)) {
        const Frustum& frust = currentCamera.getFrustum();
        // Check if the bounding sphere is in the frustum, as Frustum <-> Sphere check is fast
        collisionTypeOut = frust.ContainsSphere(center, radius, _frustPlaneCache);
        if (collisionTypeOut == Frustum::FrustCollision::FRUSTUM_INTERSECT) {
            // If the sphere is not completely in the frustum, check the AABB
            collisionTypeOut = frust.ContainsBoundingBox(boundingBox, _frustPlaneCache);
        }
    } else {
        // We are inside the AABB. So ... intersect?
        collisionTypeOut = Frustum::FrustCollision::FRUSTUM_INTERSECT;
    }

    return collisionTypeOut == Frustum::FrustCollision::FRUSTUM_OUT;
}

void SceneGraphNode::invalidateRelationshipCache() {
    _relationshipCache.invalidate();

    if (_parent && _parent->getParent()) {
        _parent->invalidateRelationshipCache();

        forEachChild([](SceneGraphNode& child) {
            child.invalidateRelationshipCache();
        });
    }
}

ECS::ECSEngine& SceneGraphNode::GetECSEngine() {
    return parentGraph().GetECSEngine();
}

const ECS::ECSEngine& SceneGraphNode::GetECSEngine() const {
    return parentGraph().GetECSEngine();
}

ECS::EntityManager* SceneGraphNode::GetEntityManager() {
    return GetECSEngine().GetEntityManager();
}

ECS::EntityManager* SceneGraphNode::GetEntityManager() const {
    return GetECSEngine().GetEntityManager();
}

ECS::ComponentManager* SceneGraphNode::GetComponentManager() {
    return GetECSEngine().GetComponentManager();
}

ECS::ComponentManager* SceneGraphNode::GetComponentManager() const {
    return GetECSEngine().GetComponentManager();
}

void SceneGraphNode::forEachChild(const DELEGATE_CBK<void, SceneGraphNode&>& callback) {
    ReadLock r_lock(_childLock);
    for (auto& child : _children) {
        callback(*child);
    }
}

void SceneGraphNode::forEachChild(const DELEGATE_CBK<void, const SceneGraphNode&>& callback) const {
    ReadLock r_lock(_childLock);
    for (auto& child : _children) {
        callback(*child);
    }
}

bool SceneGraphNode::forEachChildInterruptible(const DELEGATE_CBK<bool, SceneGraphNode&>& callback) {
    ReadLock r_lock(_childLock);
    for (auto& child : _children) {
        if (!callback(*child)) {
            return false;
        }
    }

    return true;
}

bool SceneGraphNode::forEachChildInterruptible(const DELEGATE_CBK<bool, const SceneGraphNode&>& callback) const {
    ReadLock r_lock(_childLock);
    for (auto& child : _children) {
        if (!callback(*child)) {
            return false;
        }
    }

    return true;
}

void SceneGraphNode::forEachChild(const DELEGATE_CBK<void, SceneGraphNode&, I32>& callback, U32 start, U32 end) {
    ReadLock r_lock(_childLock);
    CLAMP<U32>(end, 0, getChildCountLocked());
    assert(start < end);

    for (U32 i = start; i < end; ++i) {
        callback(*_children[i], i);
    }
}

void SceneGraphNode::forEachChild(const DELEGATE_CBK<void, const SceneGraphNode&, I32>& callback, U32 start, U32 end) const {
    ReadLock r_lock(_childLock);
    CLAMP<U32>(end, 0, getChildCountLocked());
    assert(start < end);

    for (U32 i = start; i < end; ++i) {
        callback(*_children[i], i);
    }
}

bool SceneGraphNode::forEachChildInterruptible(const DELEGATE_CBK<bool, SceneGraphNode&, I32>& callback, U32 start, U32 end) {
    ReadLock r_lock(_childLock);
    CLAMP<U32>(end, 0, getChildCountLocked());
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
    CLAMP<U32>(end, 0, getChildCountLocked());
    assert(start < end);

    for (U32 i = start; i < end; ++i) {
        if (!callback(*_children[i], i)) {
            return false;
        }
    }

    return true;
}

bool SceneGraphNode::save(ByteBuffer& outputBuffer) const {
    for (EditorComponent* editorComponent : _editorComponents) {
        if (!editorComponent->save(outputBuffer)) {
            return false;
        }
    }

    if (!parentGraph().GetECSManager().save(*this, outputBuffer)) {
        return false;
    }

    return forEachChildInterruptible([&outputBuffer](const SceneGraphNode& child) {
        return child.save(outputBuffer);
    });
}

bool SceneGraphNode::load(ByteBuffer& inputBuffer) {
    for (EditorComponent* editorComponent : _editorComponents) {
        if (!editorComponent->load(inputBuffer)) {
            return false;
        }
    }

    if (!parentGraph().GetECSManager().load(*this, inputBuffer)) {
        return false;
    }

    return forEachChildInterruptible([&inputBuffer](SceneGraphNode& child) {
        return child.load(inputBuffer);
    });
}

};