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
    for (const SceneGraphNode::NodeChildren::value_type& s :
         sgn.getChildren()) {
        sgn.addBoundingBox(s.second->getBoundingBoxConst(),
                           s.second->getNode()->getType());
    }
    bb.setComputed(true);
    return SceneNode::computeBoundingBox(sgn);
}

SceneGraphNode::SceneGraphNode(SceneNode& node, const stringImpl& name)
    : GUIDWrapper(),
      _node(&node),
      _elapsedTime(0ULL),
      _parent(nullptr),
      _loaded(true),
      _wasActive(true),
      _active(true),
      _inView(false),
      _selected(false),
      _isSelectable(false),
      _sorted(false),
      _silentDispose(false),
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
        .reset(MemoryManager_NEW NavigationComponent(*this));

    _components[to_uint(SGNComponent::ComponentType::PHYSICS)].reset(
        MemoryManager_NEW PhysicsComponent(*this));

    _components[to_uint(SGNComponent::ComponentType::RENDERING)].reset(
        MemoryManager_NEW RenderingComponent(
            materialTpl != nullptr ? materialTpl->clone("_instance_" + name)
                                   : nullptr,
            *this));
}

/// If we are destroying the current graph node
SceneGraphNode::~SceneGraphNode() {
    unload();

    Console::printfn(Locale::get("DELETE_SCENEGRAPH_NODE"), getName().c_str());
    // delete child nodes recursively
    MemoryManager::DELETE_HASHMAP(_children);
}

void SceneGraphNode::addBoundingBox(const BoundingBox& bb,
                                    const SceneNodeType& type) {
    if (!BitCompare(_bbAddExclusionList, to_uint(type))) {
        _boundingBox.Add(bb);
        if (_parent) {
            _parent->getBoundingBox().setComputed(false);
        }
    }
}

void SceneGraphNode::getBBoxes(vectorImpl<BoundingBox>& boxes) const {
    for (const NodeChildren::value_type& it : _children) {
        it.second->getBBoxes(boxes);
    }

    boxes.push_back(_boundingBox);
}

/// When unloading the current graph node
bool SceneGraphNode::unload() {
    if (!_loaded) {
        return true;
    }
    // Unload every sub node recursively
    for (NodeChildren::value_type& it : _children) {
        it.second->unload();
    }
    // Some debug output ...
    if (!_silentDispose && getParent()) {
        Console::printfn(Locale::get("REMOVE_SCENEGRAPH_NODE"),
                         _node->getName().c_str(), getName().c_str());
    }

    // if not root
    if (getParent()) {
        RemoveResource(_node);
    }

    _loaded = false;

    for (DELEGATE_CBK<>& cbk : _deletionCallbacks) {
        cbk();
    }
    return true;
}

/// Change current SceneGraphNode's parent
void SceneGraphNode::setParent(SceneGraphNode& parent) {
    assert(parent.getGUID() != getGUID());

    if (_parent) {
        if (*_parent == parent) {
            return;
        }
        // Remove us from the old parent's children map
        NodeChildren::iterator it = _parent->getChildren().find(getName());
        if (it != std::end(_parent->getChildren())) {
            _parent->getChildren().erase(it);
        }
    }
    // Set the parent pointer to the new parent
    _parent = &parent;
    // Add ourselves in the new parent's children map
    // Time to add it to the children map
    hashAlg::pair<hashMapImpl<stringImpl, SceneGraphNode*>::iterator, bool>
        result;
    // Try and add it to the map
    result = hashAlg::insert(_parent->getChildren(),
                             hashAlg::makePair(getName(), this));
    // If we had a collision (same name?)
    if (!result.second) {
        /// delete the old SceneGraphNode and add this one instead
        MemoryManager::SAFE_UPDATE((result.first)->second, this);
    }
    // That's it. Parent Transforms will be updated in the next render pass;
}

SceneGraphNode& SceneGraphNode::addNode(SceneNode& node,
                                        const stringImpl& name) {
    STUBBED(
        "SceneGraphNode: This add/create node system is an ugly HACK "
        "so it should probably be removed soon! -Ionut")

    if (Attorney::SceneNodeGraph::hasSGNParent(node)) {
        node.AddRef();
    }
    return createNode(node, name);
}

/// Add a new SceneGraphNode to the current node's child list based on a
/// SceneNode
SceneGraphNode& SceneGraphNode::createNode(SceneNode& node,
                                           const stringImpl& name) {
    // Create a new SceneGraphNode with the SceneNode's info
    // We need to name the new SceneGraphNode
    // If we did not supply a custom name use the SceneNode's name
    SceneGraphNode* sceneGraphNode = MemoryManager_NEW SceneGraphNode(
        node, name.empty() ? node.getName() : name);
    DIVIDE_ASSERT(
        sceneGraphNode != nullptr,
        "SceneGraphNode::createNode error: New node allocation failed");
    // Set the current node as the new node's parent
    sceneGraphNode->setParent(*this);
    // Do all the post load operations on the SceneNode
    // Pass a reference to the newly created SceneGraphNode in case we need
    // transforms or bounding boxes
    Attorney::SceneNodeGraph::postLoad(node, *sceneGraphNode);
    // return the newly created node
    return *sceneGraphNode;
}

// Remove a child node from this Node
void SceneGraphNode::removeNode(SceneGraphNode& node) {
    // find the node in the children map
    NodeChildren::iterator it = _children.find(node.getName());
    // If we found the node we are looking for
    if (it != std::end(_children)) {
        // Remove it from the map
        _children.erase(it);
    } else {
        for (U8 i = 0; i < _children.size(); ++i) {
            removeNode(node);
        }
    }
    // Beware. Removing a node, does no unload it!
    // Call delete on the SceneGraphNode's pointer to do that
}

// Finding a node based on the name of the SceneGraphNode or the SceneNode it
// holds
// Switching is done by setting sceneNodeName to false if we pass a
// SceneGraphNode name
// or to true if we search by a SceneNode's name
SceneGraphNode* SceneGraphNode::findNode(const stringImpl& name,
                                         bool sceneNodeName) {
    // Null return value as default
    SceneGraphNode* returnValue = nullptr;
    // Make sure a name exists
    if (!name.empty()) {
        // check if it is the name we are looking for
        if ((sceneNodeName && _node->getName().compare(name) == 0) ||
            getName().compare(name) == 0) {
            // We got the node!
            return this;
        }

        // The current node isn't the one we want, so recursively check all
        // children
        for (const NodeChildren::value_type& it : _children) {
            returnValue = it.second->findNode(name);
            // if it is not nullptr it is the node we are looking for so just
            // pass it through
            if (returnValue != nullptr) {
                return returnValue;
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
                               vectorImpl<SceneGraphNode*>& selectionHits) {
    if (isSelectable() && _boundingBox.Intersect(ray, start, end)) {
        selectionHits.push_back(this);
    }

    for (const NodeChildren::value_type& it : _children) {
        it.second->intersect(ray, start, end, selectionHits);
    }
}

void SceneGraphNode::setSelected(const bool state) {
    _selected = state;
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

void SceneGraphNode::setInitialBoundingBox(
    const BoundingBox& initialBoundingBox) {
    if (!initialBoundingBox.Compare(getInitialBoundingBox())) {
        _initialBoundingBox = initialBoundingBox;
        _initialBoundingBox.setComputed(true);
        _boundingBoxDirty = true;
    }
}

void SceneGraphNode::onCameraUpdate(Camera& camera) {
    for (NodeChildren::value_type& it : _children) {
        it.second->onCameraUpdate(camera);
    }
    Attorney::SceneNodeGraph::onCameraUpdate(*_node, *this, camera);
}

/// Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::sceneUpdate(const U64 deltaTime, SceneState& sceneState) {
    // Compute from leaf to root to ensure proper calculations
    for (NodeChildren::value_type& it : _children) {
        assert(it.second);
        it.second->sceneUpdate(deltaTime, sceneState);
    }
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
        for (NodeChildren::value_type& it : _children) {
            it.second->getComponent<PhysicsComponent>()->transformUpdated(true);
        }
    }

    assert(_node->getState() == ResourceState::RES_LOADED);
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
            if (_parent) {
                _parent->getBoundingBox().setComputed(false);
            }
        }
        _boundingBoxDirty = false;
    }

    getComponent<PhysicsComponent>()->transformUpdated(false);

    Attorney::SceneNodeGraph::sceneUpdate(*_node, deltaTime, *this, sceneState);

    if (_shouldDelete) {
        GET_ACTIVE_SCENEGRAPH().addToDeletionQueue(this);
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

    return Attorney::SceneNodeGraph::isInView(
        *getNode(), sceneRenderState, *this,
        currentStage != RenderStage::SHADOW);
}
};