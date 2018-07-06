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
                               const stringImpl& name)
    : GUIDWrapper(),
      _sceneGraph(sceneGraph),
      _updateTimer(Time::ElapsedMilliseconds()),
      _elapsedTime(0ULL),
      _node(&node),
      _active(true),
      _visibilityLocked(false),
      _isSelectable(false),
      _selectionFlag(SelectionFlag::SELECTION_NONE),
      _boundingBoxDirty(true),
      _lockBBTransforms(false),
      _usageContext(UsageContext::NODE_DYNAMIC)
{
    assert(_node != nullptr);
    _children.resize(INITIAL_CHILD_COUNT);
    _childCount = 0;
    setName(name);

    _components[to_const_uint(SGNComponent::ComponentType::PHYSICS)].reset(new PhysicsComponent(*this));
    _components[to_const_uint(SGNComponent::ComponentType::NAVIGATION)].reset(new NavigationComponent(*this));

    Material* const materialTpl = _node->getMaterialTpl();

    _components[to_const_uint(SGNComponent::ComponentType::RENDERING)].reset(
        new RenderingComponent(
            materialTpl != nullptr ? materialTpl->clone("_instance_" + name)
                                    : nullptr,
            *this));
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
    getComponent<PhysicsComponent>()->useDefaultTransform(!state);
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
    parent.addNode(shared_from_this());
    // That's it. Parent Transforms will be updated in the next render pass;
}

SceneGraphNode_ptr SceneGraphNode::addNode(SceneGraphNode_ptr node) {
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
SceneGraphNode_ptr SceneGraphNode::addNode(SceneNode& node, const stringImpl& name) {
    // Create a new SceneGraphNode with the SceneNode's info
    // We need to name the new SceneGraphNode
    // If we did not supply a custom name use the SceneNode's name
    SceneGraphNode_ptr sceneGraphNode = 
        std::make_shared<SceneGraphNode>(_sceneGraph, 
                                         node,
                                         name.empty() ? node.getName()
                                                      : name);

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

SceneGraphNode_wptr SceneGraphNode::findChild(const stringImpl& name, bool sceneNodeName) {
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        SceneGraphNode& child = getChild(i, childCount);
        if (sceneNodeName ? child.getNode()->getName().compare(name) == 0
                          : child.getName().compare(name) == 0) {
            return child.shared_from_this();
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

    if (isSelectable() && _boundingBox.intersect(ray, start, end)) {
        selectionHits.push_back(shared_from_this());
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
    for (U8 i = 0; i < to_const_uint(SGNComponent::ComponentType::COUNT); ++i) {
        if (_components[i]) {
            _components[i]->update(deltaTime);
        }
    }

    PhysicsComponent* pComp = getComponent<PhysicsComponent>();
    const PhysicsComponent::TransformMask& transformUpdateMask = pComp->transformUpdateMask();
    if (transformUpdateMask.hasSetFlags()) {
        Attorney::SceneGraphSGN::onNodeTransform(_sceneGraph, *this);
        _boundingBoxDirty = true;
        for (U32 i = 0; i < childCount; ++i) {
            getChild(i, childCount).getComponent<PhysicsComponent>()->transformUpdateMask().setFlags(transformUpdateMask);
        }
        pComp->transformUpdateMask().clearAllFlags();
    }

    SceneNode::BoundingBoxPair& pair =
        Attorney::SceneNodeSceneGraph::getBoundingBox(*_node, *this);

    if (_boundingBoxDirty || pair.second) {
        SceneGraphNode_ptr parent = getParent().lock();
        if (parent) {
            parent->_boundingBoxDirty = true;
        }

        _boundingBox.set(pair.first);
        for (U32 i = 0; i < childCount; ++i) {
            _boundingBox.add(getChild(i, childCount).getBoundingBoxConst());
        }
        if (!_lockBBTransforms) {
            _boundingBox.transform(getComponent<PhysicsComponent>()->getWorldMatrix());
        }
        _boundingSphere.fromBoundingBox(_boundingBox);
        _boundingBoxDirty = false;
        pair.second = false;
    }

    Attorney::SceneNodeSceneGraph::sceneUpdate(*_node,
    		                                   deltaTime,
    		                                   *this,
    		                                   sceneState);
}

bool SceneGraphNode::prepareDraw(const SceneRenderState& sceneRenderState,
                                 RenderStage renderStage) {
    for (std::unique_ptr<SGNComponent>& comp : _components) {
        if (comp && !comp->onDraw(renderStage)) {
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

    PhysicsComponent* pComp = getComponent<PhysicsComponent>();
    if (pComp && pComp->ignoreView(cameraGUID)) {
        U32 childCount2 = getChildCount();
        for (U32 i = 0; i < childCount2; ++i) {
            getChild(i, childCount2)._boundingBoxDirty = true;
        }
        pComp->setViewOffset(posOffset, rotationOffset);
    }
    
}

bool SceneGraphNode::cullNode(const Camera& currentCamera,
                              Frustum::FrustCollision& collisionType,
                              RenderStage currentStage) const {
    if (visibilityLocked()) {
        collisionType = Frustum::FrustCollision::FRUSTUM_IN;
        return false;
    }

    bool distanceCheck = currentStage != RenderStage::SHADOW;
    const BoundingBox& boundingBox = getBoundingBoxConst();
    const BoundingSphere& sphere = getBoundingSphereConst();

    const vec3<F32>& eye = currentCamera.getEye();
    const Frustum& frust = currentCamera.getFrustumConst();
    F32 radius = sphere.getRadius();
    const vec3<F32>& center = sphere.getCenter();
    F32 cameraDistanceSq = center.distanceSquared(eye);
    F32 visibilityDistanceSq = GET_ACTIVE_SCENE().state().generalVisibility() + radius;
    visibilityDistanceSq *= visibilityDistanceSq;

    if (distanceCheck && cameraDistanceSq > visibilityDistanceSq) {
        if (boundingBox.nearestDistanceFromPointSquared(eye) >
            std::min(visibilityDistanceSq, currentCamera.getZPlanes().y)) {
            return true;
        }
    }

    if (!boundingBox.containsPoint(eye)) {
        collisionType = frust.ContainsSphere(center, radius);
        switch (collisionType) {
            case Frustum::FrustCollision::FRUSTUM_OUT: {
                return true;
            } break;
            case Frustum::FrustCollision::FRUSTUM_INTERSECT: {
                collisionType = frust.ContainsBoundingBox(boundingBox);
                if (collisionType == Frustum::FrustCollision::FRUSTUM_OUT) {
                    return true;
                }
            } break;
        }
    }

    return false;
}
};
