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

bool SceneRoot::computeBoundingBox(SceneGraphNode& sgn) {
    BoundingBox& bb = sgn.getBoundingBox();

    bb.reset();
    U32 childCount = sgn.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        bb.add(sgn.getChild(i, childCount).getBoundingBoxConst());
    }
    return SceneNode::computeBoundingBox(sgn);
}

SceneGraphNode::SceneGraphNode(SceneNode& node, const stringImpl& name)
    : GUIDWrapper(),
      _node(&node),
      _elapsedTime(0ULL),
      _wasActive(true),
      _active(true),
      _isSelectable(false),
      _sorted(false),
      _boundingBoxDirty(true),
      _parent(nullptr),
      _updateTimer(Time::ElapsedMilliseconds()),
      _bbAddExclusionList(0),
      _usageContext(UsageContext::NODE_DYNAMIC),
      _selectionFlag(SelectionFlag::SELECTION_NONE)
{
    assert(_node != nullptr);
    _children.resize(INITIAL_CHILD_COUNT);
    _childCount = 0;
    setName(name);

    _instanceID = (_node->GetRef() - 1);

    _components[to_uint(SGNComponent::ComponentType::PHYSICS)].reset(new PhysicsComponent(*this));

    _components[to_uint(SGNComponent::ComponentType::NAVIGATION)].reset(new NavigationComponent(*this));

    Material* const materialTpl = _node->getMaterialTpl();

    _components[to_uint(SGNComponent::ComponentType::RENDERING)].reset(
        new RenderingComponent(
            materialTpl != nullptr ? materialTpl->clone("_instance_" + name)
                                    : nullptr,
            *this));

    _isVisible.fill(false);
}

void SceneGraphNode::usageContext(const UsageContext& newContext) {
    Attorney::SceneGraphSGN::onNodeUsageChange(GET_ACTIVE_SCENEGRAPH(),
                                               *this,
                                               _usageContext,
                                               newContext);
    _usageContext = newContext;
}

/// If we are destroying the current graph node
SceneGraphNode::~SceneGraphNode()
{
    if (getParent()) {
        Attorney::SceneGraphSGN::onNodeDestroy(GET_ACTIVE_SCENEGRAPH(), *this);
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

void SceneGraphNode::addBoundingBox(const BoundingBox& bb,
                                    const SceneNodeType& type) {
    if (!BitCompare(_bbAddExclusionList, to_uint(type))) {
        _boundingBox.add(bb);
        if (_parent) {
            _parent->getBoundingBox().setComputed(false);
        }
    }
}

void SceneGraphNode::useDefaultTransform(const bool state) {
    getComponent<PhysicsComponent>()->useDefaultTransform(!state);
}

/// Change current SceneGraphNode's parent
void SceneGraphNode::setParent(SceneGraphNode& parent) {
    assert(parent.getGUID() != getGUID());
    if (_parent) {
        if (*_parent == parent) {
            return;
        }
        // Remove us from the old parent's children map
        _parent->removeNode(getName(), false);
    }
    // Set the parent pointer to the new parent
    _parent = &parent;
    // Add ourselves in the new parent's children map
    _parent->addNode(shared_from_this());
    // That's it. Parent Transforms will be updated in the next render pass;
}

SceneGraphNode_ptr SceneGraphNode::addNode(SceneGraphNode_ptr node) {
    // Time to add it to the children vector
    SceneGraphNode_ptr child = findChild(node->getName()).lock();
    if (child) {
        removeNode(*child, true);
    }
    
    Attorney::SceneGraphSGN::onNodeAdd(GET_ACTIVE_SCENEGRAPH(), *node);
    
    if (_childCount == _children.size()) {
        _children.push_back(node);
    } else {
        _children[_childCount] = node;
    }
    _childCount = _childCount + 1;

    return node;
}

SceneGraphNode_ptr SceneGraphNode::addNode(SceneNode& node, const stringImpl& name) {
    STUBBED("SceneGraphNode: This add/create node system is an ugly HACK "
            "so it should probably be removed soon! -Ionut")

    if (Attorney::SceneNodeSceneGraph::hasSGNParent(node)) {
        node.AddRef();
    }

    return createNode(node, name);
}

/// Add a new SceneGraphNode to the current node's child list based on a
/// SceneNode
SceneGraphNode_ptr SceneGraphNode::createNode(SceneNode& node, const stringImpl& name) {
    // Create a new SceneGraphNode with the SceneNode's info
    // We need to name the new SceneGraphNode
    // If we did not supply a custom name use the SceneNode's name
    SceneGraphNode_ptr sceneGraphNode = 
        std::make_shared<SceneGraphNode>(node, name.empty() ? node.getName()
                                                            : name);

    // Set the current node as the new node's parent
    sceneGraphNode->setParent(*this);
    // Do all the post load operations on the SceneNode
    // Pass a reference to the newly created SceneGraphNode in case we need
    // transforms or bounding boxes
    Attorney::SceneNodeSceneGraph::postLoad(node, *sceneGraphNode);
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
                               vectorImpl<SceneGraphNode_wptr>& selectionHits) {
    if (isSelectable() && _boundingBox.intersect(ray, start, end)) {
        selectionHits.push_back(shared_from_this());
    }

    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        getChild(i, childCount).intersect(ray, start, end, selectionHits);
    }
}

void SceneGraphNode::setSelectionFlag(SelectionFlag flag) {
    _selectionFlag = flag;
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        getChild(i, childCount).setSelectionFlag(flag);
    }
}

void SceneGraphNode::setActive(const bool state) {
    _wasActive = _active;
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

void SceneGraphNode::setVisibleState(bool state, RenderStage currentStage) {
    _isVisible[to_uint(currentStage)] = state;
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        getChild(i, childCount).setVisibleState(state, currentStage);
    }
}

void SceneGraphNode::restoreActive() { 
    setActive(_wasActive);
}

bool SceneGraphNode::updateBoundingBoxPosition(const vec3<F32>& position) {
    vec3<F32> diff(position - _boundingSphere.getCenter());
    _boundingBox.translate(diff);
    _boundingSphere.fromBoundingBox(_boundingBox);
    return true;
}

bool SceneGraphNode::updateBoundingBoxTransform(const mat4<F32>& transform) {
    if (_boundingBox.transform(
            _initialBoundingBox, transform,
            !_initialBoundingBox.compare(_initialBoundingBoxCache))) {
        _initialBoundingBoxCache = _initialBoundingBox;
        _boundingSphere.fromBoundingBox(_boundingBox);
        return true;
    }

    return false;
}

void SceneGraphNode::setInitialBoundingBox(const BoundingBox& initialBoundingBox) {
    if (!initialBoundingBox.compare(getInitialBoundingBox())) {
        _initialBoundingBox = initialBoundingBox;
        _initialBoundingBox.setComputed(true);
        _boundingBoxDirty = true;
    }
}

void SceneGraphNode::onCameraUpdate(Camera& camera) {
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        getChild(i, childCount).onCameraUpdate(camera);
    }

    Attorney::SceneNodeSceneGraph::onCameraUpdate(*_node, *this, camera);
}

/// Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::sceneUpdate(const U64 deltaTime, SceneState& sceneState) {
    // Compute from leaf to root to ensure proper calculations
    U32 childCount = getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        getChild(i, childCount).sceneUpdate(deltaTime, sceneState);
    }

    // update local time
    _elapsedTime += deltaTime;

    // update all of the internal components (animation, physics, etc)
    for (U8 i = 0; i < to_uint(SGNComponent::ComponentType::COUNT); ++i) {
        if (_components[i]) {
            _components[i]->update(deltaTime);
        }
    }

    PhysicsComponent* pComp = getComponent<PhysicsComponent>();
    const PhysicsComponent::TransformMask& transformUpdateMask = pComp->transformUpdateMask();
    if (transformUpdateMask.hasSetFlags()) {
        _boundingBoxDirty = true;
        for (U32 i = 0; i < childCount; ++i) {
            getChild(i, childCount).getComponent<PhysicsComponent>()->transformUpdateMask().setFlags(transformUpdateMask);
        }
        pComp->transformUpdateMask().clearAllFlags();
    }

    assert(_node->getState() == ResourceState::RES_LOADED ||
           _node->getState() == ResourceState::RES_SPECIAL);
    // Update order is very important! e.g. Mesh BB is composed of SubMesh BB's.

    // Compute the BoundingBox if it isn't already
    if (!_boundingBox.isComputed()) {
        _node->computeBoundingBox(*this);
        assert(_boundingBox.isComputed());
        _boundingBoxDirty = true;
    }

    if (_boundingBoxDirty) {
        bool bbUpdated = false;
        /*if (transformUpdateMask.getFlag(PhysicsComponent::TransformType::TRANSLATION) &&
            !transformUpdateMask.getFlag(PhysicsComponent::TransformType::ROTATION) &&
            transformUpdateMask.getFlag(PhysicsComponent::TransformType::SCALE)) {
            bbUpdated = updateBoundingBoxPosition(pComp->getPosition());
        }else {*/
              bbUpdated = updateBoundingBoxTransform(pComp->getWorldMatrix());
        //}
        if (bbUpdated) {
            if (_parent) {
                _parent->getBoundingBox().setComputed(false);
            }
        }
        RenderingComponent* renderComp = getComponent<RenderingComponent>();
        if (renderComp) {
            Attorney::RenderingCompSceneGraph::boundingBoxUpdatedCallback(*renderComp);
        }
        _boundingBoxDirty = false;
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

bool SceneGraphNode::cullNode(const SceneRenderState& sceneRenderState,
                              Frustum::FrustCollision& collisionType,
                              RenderStage currentStage) {

    bool shadowPass = currentStage == RenderStage::SHADOW;
    if (shadowPass && !getComponent<RenderingComponent>()->castsShadows()) {
        collisionType = Frustum::FrustCollision::FRUSTUM_OUT;
        return true;
    }

    return !Attorney::SceneNodeSceneGraph::isInView(*getNode(),
                                                    sceneRenderState,
                                                    *this,
                                                    collisionType,
                                                    !shadowPass);
}
};
