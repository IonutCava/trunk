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

bool SceneRoot::computeBoundingBox(SceneGraphNode* const sgn) {
    BoundingBox& bb = sgn->getBoundingBox();
    bb.reset();
    for (const SceneGraphNode::NodeChildren::value_type& s :
         sgn->getChildren()) {
        sgn->addBoundingBox(s.second->getBoundingBoxConst(),
                            s.second->getNode()->getType());
    }
    bb.setComputed(true);
    return SceneNode::computeBoundingBox(sgn);
}

SceneGraphNode::SceneGraphNode(SceneNode* const node, const stringImpl& name)
    : GUIDWrapper(),
      _node(node),
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
      _usageContext(NODE_DYNAMIC) {
    assert(_node != nullptr);

    setName(name);
    _instanceID = (node->GetRef() - 1);
    Material* const materialTpl = _node->getMaterialTpl();
    _components[SGNComponent::SGN_COMP_ANIMATION] = nullptr;
    _components[SGNComponent::SGN_COMP_NAVIGATION] =
        MemoryManager_NEW NavigationComponent(this);
    _components[SGNComponent::SGN_COMP_PHYSICS] =
        MemoryManager_NEW PhysicsComponent(this);
    _components[SGNComponent::SGN_COMP_RENDERING] =
        MemoryManager_NEW RenderingComponent(
            materialTpl != nullptr ? materialTpl->clone("_instance_" + name)
                                   : nullptr,
            this);
}

/// If we are destroying the current graph node
SceneGraphNode::~SceneGraphNode() {
    unload();

    Console::printfn(Locale::get("DELETE_SCENEGRAPH_NODE"), getName().c_str());
    // delete child nodes recursively
    MemoryManager::DELETE_HASHMAP(_children);

    for (SGNComponent*& component : _components) {
        MemoryManager::DELETE(component);
    }
}

void SceneGraphNode::addBoundingBox(const BoundingBox& bb,
                                    const SceneNodeType& type) {
    if (!bitCompare(_bbAddExclusionList, type)) {
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
void SceneGraphNode::setParent(SceneGraphNode* const parent) {
    DIVIDE_ASSERT(parent != nullptr,
                  "SceneGraphNode error: Can't add a new node to a null "
                  "parent. Top level allowed is ROOT");
    assert(parent->getGUID() != getGUID());

    if (_parent) {
        if (*_parent == *parent) {
            return;
        }
        // Remove us from the old parent's children map
        NodeChildren::iterator it = _parent->getChildren().find(getName());
        if (it != std::end(_parent->getChildren())) {
            _parent->getChildren().erase(it);
        }
    }
    // Set the parent pointer to the new parent
    _parent = parent;
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

SceneGraphNode* SceneGraphNode::addNode(SceneNode* const node,
                                        const stringImpl& name) {
    STUBBED(
        "SceneGraphNode: This add/create node system is an ugly HACK so it "
        "should probably be removed soon! -Ionut")

    if (SceneNodeGraphAttorney::hasSGNParent(*node)) {
        node->AddRef();
    }
    return createNode(node, name);
}

/// Add a new SceneGraphNode to the current node's child list based on a
/// SceneNode
SceneGraphNode* SceneGraphNode::createNode(SceneNode* const node,
                                           const stringImpl& name) {
    // assert(node != nullptr && FindResourceImpl<Resource>(node->getName()) !=
    // nullptr);
    // Create a new SceneGraphNode with the SceneNode's info
    // We need to name the new SceneGraphNode
    // If we did not supply a custom name use the SceneNode's name
    SceneGraphNode* sceneGraphNode = MemoryManager_NEW SceneGraphNode(
        node, name.empty() ? node->getName() : name);
    // Set the current node as the new node's parent
    sceneGraphNode->setParent(this);
    // Do all the post load operations on the SceneNode
    // Pass a reference to the newly created SceneGraphNode in case we need
    // transforms or bounding boxes
    SceneNodeGraphAttorney::postLoad(*node, sceneGraphNode);
    // return the newly created node
    return sceneGraphNode;
}

// Remove a child node from this Node
void SceneGraphNode::removeNode(SceneGraphNode* node) {
    // find the node in the children map
    NodeChildren::iterator it = _children.find(node->getName());
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

void SceneGraphNode::intersect(const Ray& ray, F32 start, F32 end,
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

void SceneGraphNode::onCameraChange() {
    for (NodeChildren::value_type& it : _children) {
        it.second->onCameraChange();
    }
    SceneNodeGraphAttorney::onCameraChange(*_node, this);
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
    for (U8 i = 0; i < SGNComponent::ComponentType_PLACEHOLDER; ++i) {
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
    assert(_node->getState() == RES_LOADED);
    // Update order is very important! e.g. Mesh BB is composed of SubMesh BB's.

    // Compute the BoundingBox if it isn't already
    if (!_boundingBox.isComputed()) {
        _node->computeBoundingBox(this);
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

    SceneNodeGraphAttorney::sceneUpdate(*_node, deltaTime, this, sceneState);

    if (_shouldDelete) {
        GET_ACTIVE_SCENEGRAPH().addToDeletionQueue(this);
    }
}

bool SceneGraphNode::prepareDraw(const SceneRenderState& sceneRenderState,
                                 const RenderStage& renderStage) {
    if (_reset[renderStage]) {
        _reset[renderStage] = false;
        if (getParent() && !GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE)) {
            for (SceneGraphNode::NodeChildren::value_type& it :
                 getParent()->getChildren()) {
                if (it.second->getComponent<AnimationComponent>()) {
                    it.second->getComponent<AnimationComponent>()
                        ->resetTimers();
                }
            }
        }
    }

    for (U8 i = 0; i < SGNComponent::ComponentType_PLACEHOLDER; ++i) {
        if (_components[i]) {
            if (!_components[i]->onDraw(renderStage)) {
                return false;
            }
        }
    }

    return true;
}

void SceneGraphNode::inView(const bool state) {
    _inView = state;
    if (state && getComponent<RenderingComponent>()) {
        getComponent<RenderingComponent>()->inViewCallback();
    }
}
};