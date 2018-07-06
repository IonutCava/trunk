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

    const vectorImpl<BoundingBox>& bBoxes = 
        GET_ACTIVE_SCENEGRAPH().getBBoxes();

    for (const BoundingBox& bBox : bBoxes) {
        bb.Add(bBox);
    }

    bb.setComputed(true);
    return SceneNode::computeBoundingBox(sgn);
}

SceneGraphNode::SceneGraphNode(SceneNode& node, const stringImpl& name)
    : GUIDWrapper(),
      _node(&node),
      _elapsedTime(0ULL),
      _wasActive(true),
      _active(true),
      _inView(false),
      _selected(false),
      _isSelectable(false),
      _sorted(false),
      _boundingBoxDirty(true),
      _shouldDelete(false),
      _updateTimer(Time::ElapsedMilliseconds()),
      _childQueue(0),
      _bbAddExclusionList(0),
      _usageContext(UsageContext::NODE_DYNAMIC)
{
    assert(_node != nullptr);

    setName(name);

    _instanceID = (_node->GetRef() - 1);
    Material* const materialTpl = _node->getMaterialTpl();

    _components[to_uint(SGNComponent::ComponentType::ANIMATION)].reset(
        nullptr);

    _components[to_uint(SGNComponent::ComponentType::NAVIGATION)]
        .reset(new NavigationComponent(*this));

    _components[to_uint(SGNComponent::ComponentType::PHYSICS)].reset(
        new PhysicsComponent(*this));

    _components[to_uint(SGNComponent::ComponentType::RENDERING)].reset(
        new RenderingComponent(
            materialTpl != nullptr ? materialTpl->clone("_instance_" + name)
                                   : nullptr,
            *this));
}

/// If we are destroying the current graph node
SceneGraphNode::~SceneGraphNode()
{
    Attorney::SceneGraphSGN::onNodeDestroy(GET_ACTIVE_SCENEGRAPH(), *this);

    Console::printfn(Locale::get("REMOVE_SCENEGRAPH_NODE"),
                     getName().c_str(), _node->getName().c_str());

    // delete child nodes recursively
    WriteLock w_lock(_childrenLock);
     if (!_children.empty()) {
        for (NodeChildren::value_type& iter : _children) {
            assert(iter.second.unique());
        }
        _children.clear();
    }
    w_lock.unlock();

    if (_node->getState() == ResourceState::RES_SPECIAL) {
        MemoryManager::DELETE(_node);
    } else {
        RemoveResource(_node);
    }
}

void SceneGraphNode::addBoundingBox(const BoundingBox& bb,
                                    const SceneNodeType& type) {
    if (!BitCompare(_bbAddExclusionList, to_uint(type))) {
        _boundingBox.Add(bb);
        SceneGraphNode_ptr parent = _parent.lock();
        if (parent) {
            parent->getBoundingBox().setComputed(false);
        }
    }
}

void SceneGraphNode::getBBoxes(vectorImpl<BoundingBox>& boxes) const {
    ReadLock r_lock(_childrenLock);
    for (const NodeChildren::value_type& it : _children) {
        it.second->getBBoxes(boxes);
    }
    r_lock.unlock();

    boxes.push_back(_boundingBox);
}

void SceneGraphNode::useDefaultTransform(const bool state) {
    getComponent<PhysicsComponent>()->useDefaultTransform(!state);
}

/// Change current SceneGraphNode's parent
void SceneGraphNode::setParent(SceneGraphNode_ptr parent) {
    assert(parent->getGUID() != getGUID());
    SceneGraphNode_ptr parentSGN = _parent.lock();
    if (parentSGN) {
        if (*parentSGN.get() == *parent.get()) {
            return;
        }
        // Remove us from the old parent's children map
        parentSGN->removeNode(getName(), false);
    }
    // Set the parent pointer to the new parent
    _parent = parent;
    // Add ourselves in the new parent's children map
    parent->addNode(shared_from_this());
    // That's it. Parent Transforms will be updated in the next render pass;
}

SceneGraphNode_ptr SceneGraphNode::addNode(SceneGraphNode_ptr node) {
    // Time to add it to the children map
    std::pair<NodeChildren::iterator, bool> result;
    // Try and add it to the map
    WriteLock w_lock(_childrenLock);
    result = hashAlg::insert(_children, std::make_pair(node->getName(), node));
    // If we had a collision (same name?)
    if (!result.second) {
        /// delete the old SceneGraphNode and add this one instead
        (result.first)->second = node;
    }

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
    SceneGraphNode_ptr sceneGraphNode = std::make_shared<SceneGraphNode>(
        node, name.empty() ? node.getName() : name);

    // Set the current node as the new node's parent
    sceneGraphNode->setParent(shared_from_this());
    // Do all the post load operations on the SceneNode
    // Pass a reference to the newly created SceneGraphNode in case we need
    // transforms or bounding boxes
    Attorney::SceneNodeSceneGraph::postLoad(node, *sceneGraphNode);
    // return the newly created node
    return sceneGraphNode;
}

// Remove a child node from this Node
void SceneGraphNode::removeNode(const stringImpl& nodeName, bool recursive) {
    UpgradableReadLock url(_childrenLock);
    // find the node in the children map
    NodeChildren::iterator it = _children.find(nodeName);
    // If we found the node we are looking for
    if (it != std::end(_children)) {
        UpgradeToWriteLock uwl(url);
        // Remove it from the map
        _children.erase(it);
    } else {
        if (recursive) {
            for (U8 i = 0; i < _children.size(); ++i) {
                removeNode(nodeName);
            }
        }
    }
    // Beware. Removing a node, does no delete it!
    // Call delete on the SceneGraphNode's pointer to do that
}

// Finding a node based on the name of the SceneGraphNode or the SceneNode it
// holds
// Switching is done by setting sceneNodeName to false if we pass a
// SceneGraphNode name
// or to true if we search by a SceneNode's name
std::weak_ptr<SceneGraphNode> SceneGraphNode::findNode(const stringImpl& name,
                                                       bool sceneNodeName) {
    // Make sure a name exists
    if (!name.empty()) {
        // check if it is the name we are looking for
        if ((sceneNodeName && _node->getName().compare(name) == 0) ||
            getName().compare(name) == 0) {
            // We got the node!
            return shared_from_this();
        }

        // The current node isn't the one we want,
        // so recursively check all children
        ReadLock r_lock(_childrenLock);
        for (const NodeChildren::value_type& it : _children) {
            std::weak_ptr<SceneGraphNode> returnValue = it.second->findNode(name);
            // if it is not nullptr it is the node we are looking for so just
            // pass it through
            if (returnValue.lock()) {
                return returnValue;
            }
        }
    }

    // no children's name matches or there are no more children
    // so return nullptr, indicating that the node was not found yet
    return std::weak_ptr<SceneGraphNode>();
}

void SceneGraphNode::intersect(const Ray& ray,
                               F32 start,
                               F32 end,
                               vectorImpl<std::weak_ptr<SceneGraphNode>>& selectionHits) {
    if (isSelectable() && _boundingBox.Intersect(ray, start, end)) {
        selectionHits.push_back(shared_from_this());
    }

    ReadLock r_lock(_childrenLock);
    for (const NodeChildren::value_type& it : _children) {
        it.second->intersect(ray, start, end, selectionHits);
    }
}

void SceneGraphNode::setSelected(const bool state) {
    _selected = state;
    ReadLock r_lock(_childrenLock);
    for (NodeChildren::value_type& it : _children) {
        it.second->setSelected(_selected);
    }
}

void SceneGraphNode::setActive(const bool state) {
    _wasActive = _active;
    _active = state;
    for (U8 i = 0; i < to_uint(SGNComponent::ComponentType::COUNT); ++i) {
        if (_components[i]) {
            _components[i]->setActive(state);
        }
    }

    ReadLock r_lock(_childrenLock);
    for (NodeChildren::value_type& it : _children) {
        it.second->setActive(state);
    }
}

void SceneGraphNode::restoreActive() { 
    setActive(_wasActive);
}

bool SceneGraphNode::updateBoundingBoxTransform(const mat4<F32>& transform) {
    if (_boundingBox.Transform(
            _initialBoundingBox, transform,
            !_initialBoundingBox.Compare(_initialBoundingBoxCache))) {
        _initialBoundingBoxCache = _initialBoundingBox;
        _boundingSphere.fromBoundingBox(_boundingBox);
        return true;
    }

    return false;
}

void SceneGraphNode::setInitialBoundingBox(const BoundingBox& initialBoundingBox) {
    if (!initialBoundingBox.Compare(getInitialBoundingBox())) {
        _initialBoundingBox = initialBoundingBox;
        _initialBoundingBox.setComputed(true);
        _boundingBoxDirty = true;
    }
}

void SceneGraphNode::onCameraUpdate(Camera& camera) {
    ReadLock r_lock(_childrenLock);
    for (NodeChildren::value_type& it : _children) {
        it.second->onCameraUpdate(camera);
    }
    r_lock.unlock();

    Attorney::SceneNodeSceneGraph::onCameraUpdate(*_node,
                                                  *this,
                                                  camera);
}

/// Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::sceneUpdate(const U64 deltaTime, SceneState& sceneState) {
    // Compute from leaf to root to ensure proper calculations
    ReadLock r_lock(_childrenLock);
    for (NodeChildren::value_type& it : _children) {
        assert(it.second);
        it.second->sceneUpdate(deltaTime, sceneState);
    }
    r_lock.unlock();

    // update local time
    _elapsedTime += deltaTime;

    // update all of the internal components (animation, physics, etc)
    for (U8 i = 0; i < to_uint(SGNComponent::ComponentType::COUNT); ++i) {
        if (_components[i]) {
            _components[i]->update(deltaTime);
        }
    }

    if (getComponent<PhysicsComponent>()->transformUpdated()) {
        _boundingBoxDirty = true;
        r_lock.lock();
        for (NodeChildren::value_type& it : _children) {
            it.second->getComponent<PhysicsComponent>()->transformUpdated(true);
        }
        r_lock.unlock();
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
        if (updateBoundingBoxTransform(
                getComponent<PhysicsComponent>()->getWorldMatrix())) {
            SceneGraphNode_ptr parent = _parent.lock();
            if (parent) {
                parent->getBoundingBox().setComputed(false);
            }
        }
        RenderingComponent* renderComp = getComponent<RenderingComponent>();
        if (renderComp) {
            Attorney::RenderingCompSceneGraph::boundingBoxUpdatedCallback(*renderComp);
        }
        _boundingBoxDirty = false;
    }

    getComponent<PhysicsComponent>()->transformUpdated(false);

    Attorney::SceneNodeSceneGraph::sceneUpdate(*_node,
    		                                   deltaTime,
    		                                   *this,
    		                                   sceneState);

    if (_shouldDelete) {
        GET_ACTIVE_SCENEGRAPH().deleteNode(shared_from_this(), false);
    }
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

void SceneGraphNode::setInView(const bool state) {
    _inView = state;
    if (state) {
        Attorney::RenderingCompSceneGraph::inViewCallback(
            *getComponent<RenderingComponent>());
    }
}

bool SceneGraphNode::canDraw(const SceneRenderState& sceneRenderState,
                             RenderStage currentStage) {

    if ((currentStage == RenderStage::SHADOW &&
         !getComponent<RenderingComponent>()->castsShadows())) {
        return false;
    }

    return Attorney::SceneNodeSceneGraph::isInView(
        *getNode(), sceneRenderState, *this,
        currentStage != RenderStage::SHADOW);
}
};
