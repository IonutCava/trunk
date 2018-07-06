#include "Headers/Octree.h"

namespace Divide {

namespace {
    /// Minimum cube size is 1x1x1
    const F32 MIN_SIZE = 1.0f;
    const I32 MAX_LIFE_SPAN_LIMIT = 64;
};

bool Octree::_treeReady = false;
bool Octree::_treeBuilt = false;
std::queue<SceneGraphNode_wptr> Octree::_pendingInsertion;

Octree::Octree(U32 nodeMask)
    : _nodeMask(nodeMask),
      _maxLifespan(MAX_LIFE_SPAN_LIMIT / 8),
      _curLife(-1)
{
    _region.set(VECTOR3_ZERO, VECTOR3_ZERO);
    _activeNodes.fill(false);
}

Octree::Octree(U32 nodeMask, const BoundingBox& rootAABB)
    : Octree(nodeMask)
{
    _region.set(rootAABB);
}

Octree::Octree(U32 nodeMask,
               const BoundingBox& rootAABB,
               const vectorImpl<SceneGraphNode_wptr>& nodes)
    :  Octree(nodeMask, rootAABB)
{
    _objects.reserve(nodes.size());
    _objects.insert(std::cend(_objects), std::cbegin(nodes), std::cend(nodes));
}

Octree::~Octree()
{
}

void Octree::update(const U64 deltaTime) {
    if (!_treeBuilt) {
        buildTree();
        return;
    }
    if (_objects.empty()) {
        if (activeNodes() == 0) {
            if (_curLife == -1) {
                _curLife = _maxLifespan;
            } else if (_curLife > 0) {
                if (_maxLifespan <= MAX_LIFE_SPAN_LIMIT) {
                    _maxLifespan *= 2;
                }
                _curLife--;
            }
        }
    } else {
        if (_curLife != -1) {
            _curLife = -1;
        }
    }

    // prune any dead objects from the tree
    _objects.erase(std::remove_if(std::begin(_objects),
                                  std::end(_objects),
                                  [](SceneGraphNode_wptr crtNode) -> bool {
                                      SceneGraphNode_ptr node = crtNode.lock();
                                      return !node || (node && !node->isActive());
                                  }),
                   std::end(_objects));

    //go through and update every object in the current tree node
    _movedObjects.resize(0);
    for (SceneGraphNode_wptr crtNode : _objects) {
        SceneGraphNode_ptr node = crtNode.lock();
        //we should figure out if an object actually moved so that we know whether we need to update this node in the tree.
        if (node->getSpatialPartitionFlag()) {
            _movedObjects.push_back(crtNode);
            node->clearSpatialPartitionFlag();
        }
    }

    //recursively update any child nodes.
    for (U8 i = 0; i < 8; ++i) {
        if (_activeNodes[i]) {
            _childNodes[i]->update(deltaTime);
        }
    }

    //If an object moved, we can insert it into the parent and that will insert it into the correct tree node.
    //note that we have to do this last so that we don't accidentally update the same object more than once per frame.
    for (SceneGraphNode_wptr movedObjPtr : _movedObjects) {
        Octree*  current = this;

        //figure out how far up the tree we need to go to reinsert our moved object
        //try to move the object into an enclosing parent node until we've got full containment
        SceneGraphNode_ptr movedObj = movedObjPtr.lock();
        assert(movedObj);

        const BoundingBox& bb = movedObj->getBoundingBoxConst();
        while(!current->_region.containsBox(bb)) {
            if (current->_parent != nullptr) {
                current = current->_parent.get();
            } else {
                break; //prevent infinite loops when we go out of bounds of the root node region
            }
        }

        //now, remove the object from the current node and insert it into the current containing node.
        I64 guid = movedObj->getGUID();
        _objects.erase(std::remove_if(std::begin(_objects),
                                      std::end(_objects),
                                      [guid](SceneGraphNode_wptr updatedNode) -> bool {
                                          SceneGraphNode_ptr node = updatedNode.lock();
                                          return node && node->getGUID() == guid;
                                      }),
                       std::end(_objects));

        //this will try to insert the object as deep into the tree as we can go.
        current->insert(movedObj);
    }

    //prune out any dead branches in the tree
    for (U8 i = 0; i < 8; ++i) {
        if (_activeNodes[i] && _childNodes[i]->_curLife == 0) {
            _childNodes[i].reset();
            _activeNodes[i] = false;
        }
    }

    // root node
    if (_parent == nullptr) {
        vectorImpl<IntersectionRecord> irList = getIntersection(vectorImpl<SceneGraphNode_wptr>());

        for(IntersectionRecord ir : irList) {
            SceneGraphNode_ptr obj1 = ir._intersectedObject1.lock();
            SceneGraphNode_ptr obj2 = ir._intersectedObject2.lock();
            if (obj1 != nullptr) {
                handleIntersection(obj1, ir);
            }
            if (obj2 != nullptr) {
                handleIntersection(obj2, ir);
            }
        }
    }
}

bool Octree::addNode(SceneGraphNode_wptr node) {
    SceneGraphNode_ptr nodePtr = node.lock();
    if (nodePtr &&  !BitCompare(_nodeMask, to_uint(nodePtr->getNode<>()->getType()))) {
        _pendingInsertion.push(node);
        _treeReady = false;
        return true;
    }

    return false;
}

bool Octree::addNodes(const vectorImpl<SceneGraphNode_wptr>& nodes) {
    bool changed = false;
    for (SceneGraphNode_wptr node : nodes) {
        if (addNode(node)) {
            changed = true;
        }
    }

    return changed;
}

/// A tree has already been created, so we're going to try to insert an item into the tree without rebuilding the whole thing
void Octree::insert(SceneGraphNode_wptr object) {
    /*make sure we're not inserting an object any deeper into the tree than we have to.
    -if the current node is an empty leaf node, just insert and leave it.*/
    if (_objects.size() <= 1 && activeNodes() == 0) {
        _objects.push_back(object);
        return;
    }

    vec3<F32> dimensions(_region.getExtent());
    //Check to see if the dimensions of the box are greater than the minimum dimensions
    if (dimensions.x <= MIN_SIZE && dimensions.y <= MIN_SIZE && dimensions.z <= MIN_SIZE)
    {
        _objects.push_back(object);
        return;
    }

    vec3<F32> half(dimensions * 0.5f);
    vec3<F32> center(_region.getMin() + half);

    //Find or create subdivided regions for each octant in the current region
    BoundingBox childOctant[8];
    childOctant[0].set((_childNodes[0] != nullptr) ? _childNodes[0]->_region : BoundingBox(_region.getMin(), center));
    childOctant[1].set((_childNodes[1] != nullptr) ? _childNodes[1]->_region : BoundingBox(vec3<F32>(center.x, _region.getMin().y, _region.getMin().z), vec3<F32>(_region.getMax().x, center.y, center.z)));
    childOctant[2].set((_childNodes[2] != nullptr) ? _childNodes[2]->_region : BoundingBox(vec3<F32>(center.x, _region.getMin().y, center.z), vec3<F32>(_region.getMax().x, center.y, _region.getMax().z)));
    childOctant[3].set((_childNodes[3] != nullptr) ? _childNodes[3]->_region : BoundingBox(vec3<F32>(_region.getMin().x, _region.getMin().y, center.z), vec3<F32>(center.x, center.y, _region.getMax().z)));
    childOctant[4].set((_childNodes[4] != nullptr) ? _childNodes[4]->_region : BoundingBox(vec3<F32>(_region.getMin().x, center.y, _region.getMin().z), vec3<F32>(center.x, _region.getMax().y, center.z)));
    childOctant[5].set((_childNodes[5] != nullptr) ? _childNodes[5]->_region : BoundingBox(vec3<F32>(center.x, center.y, _region.getMin().z), vec3<F32>(_region.getMax().x, _region.getMax().y, center.z)));
    childOctant[6].set((_childNodes[6] != nullptr) ? _childNodes[6]->_region : BoundingBox(center, _region.getMax()));
    childOctant[7].set((_childNodes[7] != nullptr) ? _childNodes[7]->_region : BoundingBox(vec3<F32>(_region.getMin().x, center.y, center.z), vec3<F32>(center.x, _region.getMax().y, _region.getMax().z)));

    const BoundingBox& bb = object.lock()->getBoundingBoxConst();

    //First, is the item completely contained within the root bounding box?
    //note2: I shouldn't actually have to compensate for this. If an object is out of our predefined bounds, then we have a problem/error.
    //          Wrong. Our initial bounding box for the terrain is constricting its height to the highest peak. Flying units will be above that.
    //             Fix: I resized the enclosing box to 256x256x256. This should be sufficient.
    if (_region.containsBox(bb)) {
        bool found = false;
        //we will try to place the object into a child node. If we can't fit it in a child node, then we insert it into the current node object list.
        for (U8 i = 0; i < 8; ++i)  {
            //is the object fully contained within a quadrant?
            if (childOctant[i].containsBox(bb)) {
                if (_childNodes[i] != nullptr) {
                    _childNodes[i]->insert(object);   //Add the item into that tree and let the child tree figure out what to do with it
                } else {
                    _childNodes[i] = createNode(childOctant[i], object);   //create a new tree node with the item
                    _activeNodes[i] = true;
                }
                found = true;
            }
        }
        if (!found) {
            _objects.push_back(object);
        }
    } else {
        //either the item lies outside of the enclosed bounding box or it is intersecting it. Either way, we need to rebuild
        //the entire tree by enlarging the containing bounding box
        buildTree();
    }
}

/// Naively builds an oct tree from scratch.
void Octree::buildTree() {
    // terminate the recursion if we're a leaf node
    if (_objects.size() <= 1) {
        return;
    }

    vec3<F32> dimensions(_region.getExtent());

    if (dimensions == VECTOR3_ZERO) {
        findEnclosingCube();
        dimensions.set(_region.getExtent());
    }

    if (dimensions.x <= MIN_SIZE || dimensions.y <= MIN_SIZE || dimensions.z <= MIN_SIZE) {
        return;
    }

    vec3<F32> half(dimensions * 0.5f);
    vec3<F32> regionMin(_region.getMin());
    vec3<F32> regionMax(_region.getMax());
    vec3<F32> center(regionMin + half);

    BoundingBox octant[8];
    octant[0].set(regionMin, center);
    octant[1].set(vec3<F32>(center.x, regionMin.y, regionMin.z), vec3<F32>(regionMax.x, center.y, center.z));
    octant[2].set(vec3<F32>(center.x, regionMin.y, center.z), vec3<F32>(regionMax.x, center.y, regionMax.z));
    octant[3].set(vec3<F32>(regionMin.x, regionMin.y, center.z), vec3<F32>(center.x, center.y, regionMax.z));
    octant[4].set(vec3<F32>(regionMin.x, center.y, regionMin.z), vec3<F32>(center.x, regionMax.y, center.z));
    octant[5].set(vec3<F32>(center.x, center.y, regionMin.z), vec3<F32>(regionMax.x, regionMax.y, center.z));
    octant[6].set(center, regionMax);
    octant[7].set(vec3<F32>(regionMin.x, center.y, center.z), vec3<F32>(center.x, regionMax.y, regionMax.z));

    //This will contain all of our objects which fit within each respective octant.
    vectorImpl<SceneGraphNode_wptr> octList[8];
    for (U8 i = 0; i < 8; ++i) {
        octList[i].reserve(8);
    }

    //this list contains all of the objects which got moved down the tree and can be delisted from this node.
    vectorImpl<I64> delist;
    delist.reserve(8);

    for (SceneGraphNode_wptr obj : _objects) {
        SceneGraphNode_ptr objPtr = obj.lock();
        if (objPtr) {
            const BoundingBox& bb = objPtr->getBoundingBoxConst();
            for (U8 i = 0; i < 8; ++i) {
                if (octant[i].containsBox(bb)) {
                    octList[i].push_back(objPtr);
                    delist.push_back(objPtr->getGUID());
                    break;
                }
            }
            
        }
    }

    //delist every moved object from this node.
    _objects.erase(std::remove_if(std::begin(_objects),
                                  std::end(_objects),
                                  [&delist](SceneGraphNode_wptr movedNode) -> bool {
                                      SceneGraphNode_ptr node = movedNode.lock();
                                      if (node) {
                                          for (I64 guid : delist) {
                                              if (guid == node->getGUID()) {
                                                  return true;
                                              }
                                          }
                                      }
                                      return false;
                                    }),
        std::end(_objects));

    //Create child nodes where there are items contained in the bounding region
    for (U8 i = 0; i < 8; ++i) {
        if (!octList[i].empty()) {
            _childNodes[i] = createNode(octant[i], octList[i]);
            if (_childNodes[i]) {
                _activeNodes[i] = true;
                _childNodes[i]->buildTree();
            }
        }
    }
    _treeBuilt = true;
    _treeReady = true;
}

std::shared_ptr<Octree>
Octree::createNode(const BoundingBox& region,
                   const vectorImpl<SceneGraphNode_wptr>& objects) {
    if (objects.empty()) {
        return nullptr;
    }
    std::shared_ptr<Octree> ret = std::make_shared<Octree>(_nodeMask, region, objects);
    ret->_parent = shared_from_this();
    return ret;
}

std::shared_ptr<Octree>
Octree::createNode(const BoundingBox& region, SceneGraphNode_wptr object) {
    vectorImpl<SceneGraphNode_wptr> objList;
    objList.push_back(object);
    return createNode(region, objList);
}

void Octree::findEnclosingBox()
{
    vec3<F32> globalMin(_region.getMin());
    vec3<F32> globalMax(_region.getMax());

    //go through all the objects in the list and find the extremes for their bounding areas.
    for (SceneGraphNode_wptr obj : _objects) {
        SceneGraphNode_ptr objPtr = obj.lock();
        if (objPtr) {
            const BoundingBox& bb = objPtr->getBoundingBoxConst();
            const vec3<F32>& localMin = bb.getMin();
            const vec3<F32>& localMax = bb.getMax();

            if (localMin.x < globalMin.x) {
                globalMin.x = localMin.x;
            }

            if (localMin.y < globalMin.y) {
                globalMin.y = localMin.y;
            }

            if (localMin.z < globalMin.z) {
                globalMin.z = localMin.z;
            }

            if (localMax.x > globalMax.x) {
                globalMax.x = localMax.x;
            }

            if (localMax.y > globalMax.y) {
                globalMax.y = localMax.y;
            }

            if (localMax.z > globalMax.z) {
                globalMax.z = localMax.z;
            }
        }
    }

    _region.setMin(globalMin);
    _region.setMax(globalMax);
}

/// This finds the smallest enclosing cube which is a power of 2, for all objects in the list.
void Octree::findEnclosingCube() {
    findEnclosingBox();

    //we can't guarantee that all bounding regions will be relative to the origin, so to keep the math
    //simple, we're going to translate the existing region to be oriented off the origin and remember the translation.
    //find the min offset from (0,0,0) and translate by it for a short while
    vec3<F32> offset(_region.getMin() - VECTOR3_ZERO);
    _region.translate(offset);
    const vec3<F32>& regionMax = _region.getMax();
    //A 3D rectangle has a length, height, and width. Of those three dimensions, we want to find the largest dimension.
    //the largest dimension will be the minimum dimensions of the cube we're creating.
    I32 highX = to_int(std::floor(std::max(std::max(regionMax.x, regionMax.y), regionMax.z)));

    //see if our cube dimension is already at a power of 2. If it is, we don't have to do any work.
    for (I32 bit = 0; bit < 32; ++bit) {
        if (highX == 1 << bit) {
            _region.setMax(to_float(highX));
            _region.translate(-offset);
            return;
        }
    }

    //We have a cube with non-power of two dimensions. We want to find the next highest power of two.
    //example: 63 -> 64; 65 -> 128;
    _region.setMax(to_float(nextPOW2(highX)));
    _region.translate(-offset);
}

void Octree::updateTree() {
    if (!_treeBuilt) {
        while (!_pendingInsertion.empty()) {
            _objects.push_back(_pendingInsertion.front());
            _pendingInsertion.pop();
        }
        buildTree();

    } else {
        while (!_pendingInsertion.empty()) {
            insert(_pendingInsertion.front());
            _pendingInsertion.pop();
        }
    }
    _treeReady = true;
}

void Octree::getAllRegions(vectorImpl<BoundingBox>& regionsOut) const {
    for (U8 i = 0; i < 8; ++i) {
        if (_activeNodes[i]) {
            assert(_childNodes[i]);
            _childNodes[i]->getAllRegions(regionsOut);
        }
    }
    
    vectorAlg::emplace_back(regionsOut, getRegion().getMin(), getRegion().getMax());
}

U8 Octree::activeNodes() const {
    U8 ret = 0;
    for (U8 i = 0; i < 8; ++i) {
        if (_activeNodes[i]) {
            ret++;
        }
    }
    return ret;
}

vectorImpl<IntersectionRecord> Octree::getIntersection(const vectorImpl<SceneGraphNode_wptr>& parentObjects) {
    return vectorImpl<IntersectionRecord>();
}

void Octree::handleIntersection(SceneGraphNode_ptr node, IntersectionRecord intersection) {
}

}; //namespace Divide
