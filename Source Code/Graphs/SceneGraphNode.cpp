#include "Headers/SceneGraph.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/ApplicationTimer.h"
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
                               SceneNode& node,
                               const stringImpl& name,
                               U32 componentMask)
    : GUIDWrapper(),
      _sceneGraph(sceneGraph),
      _updateTimer(Time::ElapsedMilliseconds()),
      _elapsedTime(0ULL),
      _node(&node),
      _active(true),
      _visibilityLocked(false),
      _isSelectable(false),
      _selectionFlag(SelectionFlag::SELECTION_NONE),
      _usageContext(UsageContext::NODE_DYNAMIC)
{
    assert(_node != nullptr);
    _children.resize(INITIAL_CHILD_COUNT);
    _childCount = 0;

    for (std::atomic_bool& flag : _updateFlags) {
        flag = false;
    }

    setName(name);

    if (BitCompare(componentMask, to_uint(SGNComponent::ComponentType::ANIMATION))) {
        setComponent(SGNComponent::ComponentType::ANIMATION, new AnimationComponent(*this));
    }
    if (BitCompare(componentMask, to_uint(SGNComponent::ComponentType::INVERSE_KINEMATICS))) {
        setComponent(SGNComponent::ComponentType::INVERSE_KINEMATICS, new IKComponent(*this));
    }
    if (BitCompare(componentMask, to_uint(SGNComponent::ComponentType::RAGDOLL))) {
        setComponent(SGNComponent::ComponentType::RAGDOLL, new RagdollComponent(*this));
    }
    if (BitCompare(componentMask, to_uint(SGNComponent::ComponentType::NAVIGATION))) {
        setComponent(SGNComponent::ComponentType::NAVIGATION, new NavigationComponent(*this));
    }
    if (BitCompare(componentMask, to_uint(SGNComponent::ComponentType::PHYSICS))) {
        setComponent(SGNComponent::ComponentType::PHYSICS, new PhysicsComponent(*this));

        PhysicsComponent* pComp = get<PhysicsComponent>();
        pComp->addTransformUpdateCbk(DELEGATE_BIND(&Attorney::SceneGraphSGN::onNodeTransform, std::ref(_sceneGraph), std::ref(*this)));
        pComp->addTransformUpdateCbk(DELEGATE_BIND(&SceneGraphNode::onTransform, this));
    }

    if (BitCompare(componentMask, to_uint(SGNComponent::ComponentType::BOUNDS))) {
        setComponent(SGNComponent::ComponentType::BOUNDS, new BoundsComponent(*this));
    }
    
    if (BitCompare(componentMask, to_uint(SGNComponent::ComponentType::RENDERING))) {

        Material* const materialTpl = _node->getMaterialTpl();
        Material* materialInst = nullptr;
        if (materialTpl != nullptr) {
            materialInst = materialTpl->clone("_instance_" + name);
        }
        setComponent(SGNComponent::ComponentType::RENDERING, new RenderingComponent(materialInst, *this));
    }
   
}

void SceneGraphNode::setComponent(SGNComponent::ComponentType type, SGNComponent* component) {
    I8 idx = getComponentIdx(type);
    // We have a component registered for the specified slot
    if (idx != -1) {
        assert(_components[idx] != nullptr);
        if (component != nullptr) {
            // We want to replace the existing entry, so keep the same index
            _components[idx].reset(component);
        } else {
            // We want to delete the existing entry, so destroy the index as well
            _components.erase(std::cbegin(_components) + idx);
            setComponentIdx(type, -1);
        }
    // We are adding a new component type
    } else {
        vectorAlg::emplace_back(_components, component);
        setComponentIdx(type, to_byte(_components.size() - 1));
    }
    
}

void SceneGraphNode::usageContext(const UsageContext& newContext) {
    Attorney::SceneGraphSGN::onNodeUsageChange(_sceneGraph,
                                               *this,
                                               _usageContext,
                                               newContext);
    _usageContext = newContext;
}

/// If we are destroying the current graph node
SceneGraphNode::~SceneGraphNode()
{
    if (getParent().lock()) {
        Attorney::SceneGraphSGN::onNodeDestroy(_sceneGraph, *this);
    }
    Console::printfn(Locale::get(_ID("REMOVE_SCENEGRAPH_NODE")),
                     getName().c_str(), _node->getName().c_str());

#if defined(_DEBUG)
    for (U32 i = 0; i < getChildCount(); ++i) {
        DIVIDE_ASSERT(_children[i].unique(), "SceneGraphNode::~SceneGraphNode error: child still in use!");
    }
#endif

    if (_node->getState() == ResourceState::RES_SPECIAL) {
        MemoryManager::DELETE(_node);
    } else {
        RemoveResource(_node);
    }
}

void SceneGraphNode::useDefaultTransform(const bool state) {
    get<PhysicsComponent>()->useDefaultTransform(!state);
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
        parentPtr->removeNode(getName(), false);
    }
    // Set the parent pointer to the new parent
    _parent = parent.shared_from_this();
    // Add ourselves in the new parent's children map
    parent.registerNode(shared_from_this());
    // That's it. Parent Transforms will be updated in the next render pass;
}

SceneGraphNode_ptr SceneGraphNode::registerNode(SceneGraphNode_ptr node) {
    // Time to add it to the children vector
    SceneGraphNode_ptr child = findChild(node->getName()).lock();
    if (child) {
        removeNode(*child, true);
    }
    
    Attorney::SceneGraphSGN::onNodeAdd(_sceneGraph, *node);
    
    if (_childCount == _children.size()) {
        _children.push_back(node);
    } else {
        _children[_childCount] = node;
    }
    _childCount = _childCount + 1;

    return node;
}

/// Add a new SceneGraphNode to the current node's child list based on a
/// SceneNode
SceneGraphNode_ptr SceneGraphNode::addNode(SceneNode& node, U32 componentMask, const stringImpl& name) {
    // Create a new SceneGraphNode with the SceneNode's info
    // We need to name the new SceneGraphNode
    // If we did not supply a custom name use the SceneNode's name
    SceneGraphNode_ptr sceneGraphNode = 
        std::make_shared<SceneGraphNode>(_sceneGraph, 
                                         node,
                                         name.empty() ? node.getName()
                                                      : name,
                                         componentMask);

    // Set the current node as the new node's parent
    sceneGraphNode->setParent(*this);
    if (node.getState() == ResourceState::RES_LOADED) {
        // Do all the post load operations on the SceneNode
        // Pass a reference to the newly created SceneGraphNode in case we need
        // transforms or bounding boxes
        Attorney::SceneNodeSceneGraph::postLoad(node, *sceneGraphNode);
    } else if (node.getState() == ResourceState::RES_LOADING) {
        node.setStateCallback(ResourceState::RES_LOADED,
            [&node, sceneGraphNode]() {
                Attorney::SceneNodeSceneGraph::postLoad(node, *sceneGraphNode);
            }
        );
    }
    // return the newly created node
    return sceneGraphNode;
}

// Remove a child node from this Node
void SceneGraphNode::removeNode(const stringImpl& nodeName, bool recursive) {
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        if (getChild(i, childCount).getName().compare(nodeName) == 0) {
            _children[i].reset();
            _childCount = _childCount - 1;
            std::swap(_children[_childCount], _children[i]);
            return;
        }
    }
    
    if (recursive) {
        for (U32 i = 0; i < childCount; ++i) {
            removeNode(nodeName);
        }
    }

    // Beware. Removing a node, does no delete it!
    // Call delete on the SceneGraphNode's pointer to do that
}

void SceneGraphNode::postLoad() {

}

bool SceneGraphNode::isChildOfType(U32 typeMask, bool ignoreRoot) const {
    if (ignoreRoot) {
        ClearBit(typeMask, to_const_uint(SceneNodeType::TYPE_ROOT));
    }
    SceneGraphNode_ptr parent = getParent().lock();
    while (parent != nullptr) {
        if (BitCompare(typeMask, to_uint(parent->getNode<>()->getType()))) {
            return true;
        }
        parent = parent->getParent().lock();
    }

    return false;
}

bool SceneGraphNode::isRelated(const SceneGraphNode& target) const {
    I64 targetGUID = target.getGUID();
    //is same node?
    if (getGUID() == targetGUID) {
        return true;
    }
    SceneGraphNode_ptr parent = getParent().lock();
    //is target parent?
    if (parent && parent->getGUID() == target.getGUID()) {
        return true;
    }
    //is sibling
    if (parent && parent->isChild(target, true)) {
        return true;
    }
    //is target child?
    return isChild(target, true);
}

bool SceneGraphNode::isChild(const SceneGraphNode& target, bool recursive) const {
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        const SceneGraphNode& child = getChild(i, childCount);
        if (child.getGUID() == target.getGUID()) {
            return true;
        } else {
            if (recursive) {
                if (child.isChild(target, true)) {  
                    return true;
                }
            }
        }
    }
    return false;
}

SceneGraphNode_wptr SceneGraphNode::findChild(I64 GUID, bool recursive) {
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        SceneGraphNode& child = getChild(i, childCount);
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

SceneGraphNode_wptr SceneGraphNode::findChild(const stringImpl& name, bool sceneNodeName, bool recursive) {
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        SceneGraphNode& child = getChild(i, childCount);
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

// Finding a node based on the name of the SceneGraphNode or the SceneNode it holds
// Switching is done by setting sceneNodeName to false if we pass a SceneGraphNode name
// or to true if we search by a SceneNode's name
SceneGraphNode_wptr SceneGraphNode::findNode(const stringImpl& name,  bool sceneNodeName) {
    // Make sure a name exists
    if (!name.empty()) {
        // check if it is the name we are looking for
        if (sceneNodeName ? _node->getName().compare(name) == 0
                          : getName().compare(name) == 0) {
            // We got the node!
            return shared_from_this();
        }

        // The current node isn't the one we want, so check children
        SceneGraphNode_wptr child = findChild(name, sceneNodeName);
        if (!child.expired()) {
            return child;
        }

        // The node we want isn't one of the children, so recursively check each of them
        U32 childCount = getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            SceneGraphNode_wptr returnValue = getChild(i, childCount).findNode(name, sceneNodeName);
            // if it is not nullptr it is the node we are looking for so just pass it through
            if (!returnValue.expired()) {
                return returnValue;
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
                               vectorImpl<SceneGraphNode_cwptr>& selectionHits,
                               bool recursive) const {

    if (isSelectable()) {
        if (get<BoundsComponent>()->getBoundingBox().intersect(ray, start, end)) {
            selectionHits.push_back(shared_from_this());
        }
    }

    if (recursive) {
        U32 childCount = getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            getChild(i, childCount).intersect(ray, start, end, selectionHits);
        }
    }
}

void SceneGraphNode::setSelectionFlag(SelectionFlag flag) {
    if (_selectionFlag != flag) {
        _selectionFlag = flag;
        U32 childCount = getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            getChild(i, childCount).setSelectionFlag(flag);
        }
    }
}

void SceneGraphNode::setSelectable(const bool state) {
    if (_isSelectable != state) {
        _isSelectable = state;
        U32 childCount = getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            getChild(i, childCount).setSelectable(state);
        }
    }
}

void SceneGraphNode::lockVisibility(const bool state) {
    if (_visibilityLocked != state) {
        _visibilityLocked = state;
        U32 childCount = getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            getChild(i, childCount).lockVisibility(state);
        }
    }
}

void SceneGraphNode::setActive(const bool state) {
    if (_active != state) {
        _active = state;
        for (std::unique_ptr<SGNComponent>& comp : _components) {
            if (comp) {
                comp->setActive(state);
            }
        }

        U32 childCount = getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            getChild(i, childCount).setActive(state);
        }
    }
}

/// Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::sceneUpdate(const U64 deltaTime, SceneState& sceneState) {
    ResourceState nodeState = _node->getState();
    assert(nodeState == ResourceState::RES_LOADED ||
           nodeState == ResourceState::RES_LOADING ||
           nodeState == ResourceState::RES_SPECIAL);

    // Node is not fully loaded. Skip.
    if (nodeState == ResourceState::RES_LOADING) {
        return;
    }

    // Compute from leaf to root to ensure proper calculations
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        getChild(i, childCount).sceneUpdate(deltaTime, sceneState);
    }

    // update local time
    _elapsedTime += deltaTime;

    // update all of the internal components (animation, physics, etc)
    for (std::unique_ptr<SGNComponent>& component : _components) {
        component->update(deltaTime);
    }

    Attorney::SceneNodeSceneGraph::sceneUpdate(*_node, deltaTime, *this, sceneState);
}

void SceneGraphNode::onTransform() {
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        getChild(i, childCount).onTransform();
    }

    PhysicsComponent* pComp = get<PhysicsComponent>();
    BoundsComponent* bComp = get<BoundsComponent>();
    if (pComp != nullptr && bComp != nullptr) {
        bComp->onTransform(pComp->getWorldMatrix());
    }
}

bool SceneGraphNode::prepareDraw(const SceneRenderState& sceneRenderState,
                                 RenderStage renderStage) {
    for (std::unique_ptr<SGNComponent>& comp : _components) {
        if (comp && !comp->onRender(renderStage)) {
            return false;
        }
    }

    return true;
}

void SceneGraphNode::frameEnded() {
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        getChild(i, childCount).frameEnded();
    }
}

void SceneGraphNode::onCameraUpdate(const I64 cameraGUID,
                                    const vec3<F32>& posOffset,
                                    const mat4<F32>& rotationOffset) {

    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        getChild(i, childCount).onCameraUpdate(cameraGUID, posOffset, rotationOffset);
    }

    PhysicsComponent* pComp = get<PhysicsComponent>();
    if (pComp && pComp->ignoreView(cameraGUID)) {
        pComp->setViewOffset(posOffset, rotationOffset);
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
        const Frustum& frust = currentCamera.getFrustumConst();
        collisionTypeOut = frust.ContainsSphere(center, radius);
        if (collisionTypeOut == Frustum::FrustCollision::FRUSTUM_INTERSECT) {
            collisionTypeOut = frust.ContainsBoundingBox(boundingBox);
        }
    }

    return collisionTypeOut == Frustum::FrustCollision::FRUSTUM_OUT;
}

};
