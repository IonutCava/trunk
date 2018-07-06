#include "Headers/SceneGraph.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"
#include "Environment/Water/Headers/Water.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Hardware/Video/Shaders/Headers/ShaderProgram.h"

#pragma message("TODO (Prio 1): - Bake transforms inside vbs for STATIC nodes")
#pragma message("               - assert if 'getTransform()' is called for such nodes. Use default identity matrix for static nodes")
#pragma message("               - premultiply positions AND normals AND tangents AND bitangets to avoid artifacts")
#pragma message("               - Ionut")

SceneGraphNode::SceneGraphNode(SceneGraph* const sg, SceneNode* const node) : GUIDWrapper(),
                                                  _sceneGraph(sg),
                                                  _node(node),
                                                  _elapsedTime(0ULL),
                                                  _parent(nullptr),
                                                  _transform(nullptr),
                                                  _prevTransformValues(nullptr),
                                                  _loaded(true),
                                                  _wasActive(true),
                                                  _active(true),
                                                  _inView(false),
                                                  _selected(false),
                                                  _isSelectable(false),
                                                  _noDefaultTransform(false),
                                                  _sorted(false),
                                                  _silentDispose(false),
                                                  _boundingBoxDirty(true),
                                                  _shouldDelete(false),
                                                  _isReady(false),
                                                  _castsShadows(true),
                                                  _receiveShadows(true),
                                                  _updateTimer(GETMSTIME()),
                                                  _childQueue(0),
                                                  _bbAddExclusionList(0),
                                                  _usageContext(NODE_DYNAMIC)
{
    assert(_node != nullptr);

    _components[SGNComponent::SGN_COMP_ANIMATION]  = nullptr;
    _components[SGNComponent::SGN_COMP_NAVIGATION] = New NavigationComponent(this);
    _components[SGNComponent::SGN_COMP_PHYSICS]    = New PhysicsComponent(this);
    _instanceID = node->getReferenceCount();
}

///If we are destroying the current graph node
SceneGraphNode::~SceneGraphNode(){
    unload();

    PRINT_FN(Locale::get("DELETE_SCENEGRAPH_NODE"), getName().c_str());
    //delete children nodes recursively
    FOR_EACH(NodeChildren::value_type it, _children){
        SAFE_DELETE(it.second);
    }
    FOR_EACH(NodeComponents::value_type it, _components){
        SAFE_DELETE(it.second);
    }
    //and delete the transform bound to this node
    SAFE_DELETE(_transform);
    SAFE_DELETE(_prevTransformValues);
    _children.clear();
    _components.clear();
}

void SceneGraphNode::addBoundingBox(const BoundingBox& bb, const SceneNodeType& type) {
    if(!bitCompare(_bbAddExclusionList, type)){
        _boundingBox.Add(bb);
        if(_parent) _parent->getBoundingBox().setComputed(false);
    }
}

SceneGraphNode* SceneGraphNode::getRoot() const {
    return _sceneGraph->getRoot();
}

void SceneGraphNode::getBBoxes(vectorImpl<BoundingBox >& boxes ) const {
    FOR_EACH(const NodeChildren::value_type& it, _children){
        it.second->getBBoxes(boxes);
    }

    boxes.push_back(_boundingBox);
}

void SceneGraphNode::getShadowCastersAndReceivers(vectorImpl<const SceneGraphNode* >& casters, vectorImpl<const SceneGraphNode* >& receivers, bool visibleOnly) const {
   FOR_EACH(const NodeChildren::value_type& it, _children){
       it.second->getShadowCastersAndReceivers(casters, receivers, visibleOnly);
    }

    if (!visibleOnly || visibleOnly && _inView) {
        if (getCastsShadows())    casters.push_back(this);
        if (getReceivesShadows()) receivers.push_back(this);
    }
}

///When unloading the current graph node
bool SceneGraphNode::unload(){
    if (!_loaded) return true;

    //Unload every sub node recursively
    FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->unload();
    }
    //Some debug output ...
    if(!_silentDispose && getParent()){
        PRINT_FN(Locale::get("REMOVE_SCENEGRAPH_NODE"),_node->getName().c_str(), getName().c_str());
    }
    //if not root
    if (getParent()){
        _node->decReferenceCount();
        if(_node->getReferenceCount() == 0) {
            RemoveResource(_node);
        }
    }
    _loaded = false;
    return true;
}

///Change current SceneGraphNode's parent
void SceneGraphNode::setParent(SceneGraphNode* const parent) {
    assert(parent != nullptr);
    assert(parent->getGUID() != getGUID());

    if(_parent){
        if (_parent->getGUID() == parent->getGUID())
            return;
        //Remove us from the old parent's children map
        NodeChildren::iterator it = _parent->getChildren().find(getName());
        _parent->getChildren().erase(it);
    }
    //Set the parent pointer to the new parent
    _parent = parent;
    //Add ourselves in the new parent's children map
    //Time to add it to the children map
    std::pair<Unordered_map<std::string, SceneGraphNode*>::iterator, bool > result;
    //Try and add it to the map
    result = _parent->getChildren().insert(std::make_pair(getName(),this));
    //If we had a collision (same name?)
    if(!result.second){
        ///delete the old SceneGraphNode and add this one instead
        SAFE_UPDATE((result.first)->second,this);
    }
    //That's it. Parent Transforms will be updated in the next render pass;
}

///Add a new SceneGraphNode to the current node's child list based on a SceneNode
SceneGraphNode* SceneGraphNode::addNode(SceneNode* const node,const std::string& name){
    //Create a new SceneGraphNode with the SceneNode's info
    SceneGraphNode* sceneGraphNode = New SceneGraphNode(_sceneGraph, node);
    //Validate it to be safe
    assert(sceneGraphNode);
    //We need to name the new SceneGraphNode
    //If we did not supply a custom name use the SceneNode's name
    std::string sgName(name.empty() ? node->getName() : name);
    //Name the new SceneGraphNode
    sceneGraphNode->setName(sgName);
     //Get the new node's transform
    Transform* nodeTransform = sceneGraphNode->getTransform();
    //If the current node and the new node have transforms,
    //Update the relationship between the 2
    Transform* currentTransform = getTransform();
    if(nodeTransform && currentTransform){
        //The child node's parent transform is our current transform matrix
        nodeTransform->setParentTransform(currentTransform);
    }
    //Set the current node as the new node's parent
    sceneGraphNode->setParent(this);
    node->incReferenceCount();
    //Do all the post load operations on the SceneNode
    //Pass a reference to the newly created SceneGraphNode in case we need transforms or bounding boxes
    node->postLoad(sceneGraphNode);
    //return the newly created node
    return sceneGraphNode;
}

//Remove a child node from this Node
void SceneGraphNode::removeNode(SceneGraphNode* node){
    //find the node in the children map
    NodeChildren::iterator it = _children.find(node->getName());
    //If we found the node we are looking for
    if(it != _children.end()) {
        //Remove it from the map
        _children.erase(it);
    }else{
        for(U8 i = 0; i < _children.size(); ++i)
            removeNode(node);
    }
    //Beware. Removing a node, does no unload it!
    //Call delete on the SceneGraphNode's pointer to do that
}

//Finding a node based on the name of the SceneGraphNode or the SceneNode it holds
//Switching is done by setting sceneNodeName to false if we pass a SceneGraphNode name
//or to true if we search by a SceneNode's name
SceneGraphNode* SceneGraphNode::findNode(const std::string& name, bool sceneNodeName){
    //Null return value as default
    SceneGraphNode* returnValue = nullptr;
     //Make sure a name exists
    if (!name.empty()){
        //check if it is the name we are looking for
        if ((sceneNodeName && _node->getName().compare(name) == 0) || getName().compare(name) == 0){
            // We got the node!
            return this;
        }

        //The current node isn't the one we want, so recursively check all children
        FOR_EACH(NodeChildren::value_type& it, _children){
            returnValue = it.second->findNode(name);
            // if it is not nullptr it is the node we are looking for so just pass it through
            if(returnValue != nullptr)
                return returnValue;
        }
    }

    // no children's name matches or there are no more children
    // so return nullptr, indicating that the node was not found yet
    return nullptr;
}

void SceneGraphNode::Intersect(const Ray& ray, F32 start, F32 end, vectorImpl<SceneGraphNode* >& selectionHits){

    if (isSelectable() && _boundingBox.Intersect(ray, start, end))
        selectionHits.push_back(this);

    FOR_EACH(NodeChildren::value_type& it, _children)
        it.second->Intersect(ray,start,end,selectionHits);
}

//This updates the SceneGraphNode's transform by deleting the old one first
void SceneGraphNode::setTransform(Transform* const t) {
    SAFE_UPDATE(_transform, t);
    // Update children
    FOR_EACH(NodeChildren::value_type& it, _children){
        Transform* nodeTransform = it.second->getTransform();
        if(nodeTransform)
            nodeTransform->setParentTransform(_transform);
    }
}

//Get the node's transform
Transform* const SceneGraphNode::getTransform(){
    //A node does not necessarily have a transform. If this is the case, we can either create a default one or return nullptr.
    //When creating a node we can specify if we do not want a default transform
    if(!_noDefaultTransform && !_transform){
        _transform = New Transform();
        assert(_transform);
        _isReady = true;
    }
    return _transform;
}

const mat4<F32>& SceneGraphNode::getWorldMatrix(D32 interpolationFactor){
    if(getTransform()){
        if(!_prevTransformValues) {
            _prevTransformValues = New TransformValues();
            _transform->getValues(*_prevTransformValues);
        }
        _worldMatrixInterp.set(_transform->interpolate(*_prevTransformValues, interpolationFactor));
        _transform->getValues(*_prevTransformValues);
    }

    return _worldMatrixInterp;
}
