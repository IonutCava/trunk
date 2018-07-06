#include "Headers/Octree.h"

namespace Divide {

bool Octree::_treeReady = false;
bool Octree::_treeBuilt = false;
vectorImpl<I64> Octree::_movedObjectsQueue;

Octree::Octree(U32 nodeMask)
    : _nodeMask(nodeMask),
      _maxLifespan(8),
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
                _curLife--;
            }
        }
    } else {
        if (_curLife != -1) {
            if (_maxLifespan <= MAX_LIFE_SPAN_LIMIT) {
                _maxLifespan *= 2;
            }
            _curLife = -1;
        }
    }

    _movedObjects.resize(0);
    //go through and update every object in the current tree node
    for(I64 guid : _movedObjectsQueue) {
        for (SceneGraphNode_wptr node : _objects) {
            if (guid == node.lock()->getGUID()) {
                _movedObjects.push_back(node);
                break;
            }
        }
    }

    _movedObjectsQueue.erase(std::remove_if(std::begin(_movedObjectsQueue),
                                            std::end(_movedObjectsQueue),
                                            [&](I64 guid) -> bool {
                                            for (SceneGraphNode_wptr crtNode : _movedObjects) {
                                                SceneGraphNode_ptr node = crtNode.lock();
                                                if (node && node->getGUID() == guid) {
                                                    return true;
                                                }
                                            }
                                            return false;
                                        }),
                            std::end(_movedObjectsQueue));

    vectorImpl<I64> objToRemove;
    objToRemove.reserve(_objects.size());
    auto removeFunc = [&objToRemove](SceneGraphNode_wptr movedNode) -> bool {
                                        SceneGraphNode_ptr node = movedNode.lock();
                                        if (node) {
                                            I64 crtGUID = node->getGUID();
                                            for (I64 guid : objToRemove) {
                                                if (crtGUID == guid) {
                                                    return true;
                                                }
                                            }
                                        }
                                        return node == nullptr;
                                    };

    // prune any dead objects from the tree
    for (SceneGraphNode_wptr obj : _objects) {
        SceneGraphNode_ptr objPtr = obj.lock();
        assert(objPtr != nullptr);
        if (!objPtr->isActive()) {
            objToRemove.push_back(objPtr->getGUID());
        }
    }

    _movedObjects.erase(std::remove_if(std::begin(_movedObjects),
                                       std::end(_movedObjects),
                                       removeFunc),
                        std::end(_movedObjects));

    _objects.erase(std::remove_if(std::begin(_objects),
                                  std::end(_objects),
                                  removeFunc),
                   std::end(_objects));

    for (U8 i = 0; i < 8; ++i) {
        if (_activeNodes[i]) {
            assert(_childNodes[i]);
            _childNodes[i]->update(deltaTime);
        }
    }

    for (SceneGraphNode_wptr movedObjPtr : _movedObjects) {
        Octree*  current = this;
        SceneGraphNode_ptr movedObj = movedObjPtr.lock();
        if (!movedObj) {
            continue;
        }

        const BoundingBox& bb = movedObj->getBoundingBoxConst();

        if (bb.getExtent() > VECTOR3_ZERO) {
            while(!current->_region.containsBox(bb)) {
                if (current->_parent != nullptr) {
                    current = current->_parent.get();
                } else {
                    break;
                }
            }
        } else {
            while (!current->_region.containsSphere(movedObj->getBoundingSphereConst())) {
                if (current->_parent != nullptr) {
                    current = current->_parent.get();
                } else {
                    break;
                }
            }
        }

        I64 guid = movedObj->getGUID();
        _objects.erase(std::remove_if(std::begin(_objects),
                                      std::end(_objects),
                                      [guid](SceneGraphNode_wptr updatedNode) -> bool {
                                          SceneGraphNode_ptr node = updatedNode.lock();
                                          return node && node->getGUID() == guid;
                                      }),
                       std::end(_objects));

        current->insert(movedObj);
    }

    for (U8 i = 0; i < 8; ++i) {
        if (_activeNodes[i] && _childNodes[i]->_curLife == 0) {
            _childNodes[i].reset();
            _activeNodes[i] = false;
        }
    }

    // root node
    if (_parent == nullptr) {
        /*vectorImpl<IntersectionRecord> irList = getIntersection(vectorImpl<SceneGraphNode_wptr>());

        for(IntersectionRecord ir : irList) {
            if (ir.PhysicalObject != nullptr)
                ir.PhysicalObject.handleIntersection(ir);
            if (ir.OtherPhysicalObject != null)
                ir.OtherPhysicalObject.handleIntersection(ir);
        }*/
    }
}

void Octree::addNode(SceneGraphNode_wptr node) {
    SceneGraphNode_ptr nodePtr = node.lock();
    if (nodePtr &&  !BitCompare(_nodeMask, to_uint(nodePtr->getNode<>()->getType()))) {
        insert(node);
        _treeReady = false;
    }
}

void Octree::addNodes(const vectorImpl<SceneGraphNode_wptr>& nodes) {
    for (SceneGraphNode_wptr node : nodes) {
        addNode(node);
    }
}

void Octree::registerMovedNode(SceneGraphNode& node) {
    if (!BitCompare(_nodeMask, to_uint(node.getNode<>()->getType()))) {
        if (std::find(std::cbegin(_movedObjectsQueue),
                      std::cend(_movedObjectsQueue),
                      node.getGUID()) == std::cend(_movedObjectsQueue))
        {
            _movedObjectsQueue.push_back(node.getGUID());
        }
    }
}

void Octree::insert(SceneGraphNode_wptr object) {
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
    const BoundingSphere& bs = object.lock()->getBoundingSphereConst();

    //First, is the item completely contained within the root bounding box?
    //note2: I shouldn't actually have to compensate for this. If an object is out of our predefined bounds, then we have a problem/error.
    //          Wrong. Our initial bounding box for the terrain is constricting its height to the highest peak. Flying units will be above that.
    //             Fix: I resized the enclosing box to 256x256x256. This should be sufficient.
    if (bb.getExtent() != VECTOR3_ZERO && _region.containsBox(bb)) {
        bool found = false;
        //we will try to place the object into a child node. If we can't fit it in a child node, then we insert it into the current node object list.
        for (U8 i = 0; i < 8; ++i)
        {
            //is the object fully contained within a quadrant?
            if (childOctant[i].containsBox(bb))
            {
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
    } else if (bs.getRadius() > 0 && _region.containsSphere(bs)) {
        bool found = false;
        //we will try to place the object into a child node. If we can't fit it in a child node, then we insert it into the current node object list.
        for (U8 i = 0; i < 8; ++i) {
            //is the object contained within a child quadrant?
            if (childOctant[i].containsSphere(bs)) {
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

    if (dimensions.x < MIN_SIZE || dimensions.y < MIN_SIZE || dimensions.z < MIN_SIZE) {
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

    vectorImpl<SceneGraphNode_wptr> octList[8];
    for (U8 i = 0; i < 8; ++i) {
        octList[i].reserve(8);
    }

    vectorImpl<I64> delist;
    delist.reserve(8);

    for (SceneGraphNode_wptr obj : _objects) {
        SceneGraphNode_ptr objPtr = obj.lock();
        if (objPtr) {
            const BoundingBox& bb = objPtr->getBoundingBoxConst();
            const BoundingSphere& bs = objPtr->getBoundingSphereConst();
            if (bb.getExtent() != VECTOR3_ZERO) {
                for (U8 i = 0; i < 8; ++i) {
                    if (octant[i].containsBox(bb)) {
                        octList[i].push_back(objPtr);
                        delist.push_back(objPtr->getGUID());
                        break;
                    }
                }
            } else if (bs.getRadius() > 0.0f) {
                for (U8 i = 0; i < 8; ++i) {
                    if (octant[i].containsSphere(bs)) {
                        octList[i].push_back(objPtr);
                        delist.push_back(objPtr->getGUID());
                        break;
                    }
                }
            }
        }
    }

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

    for (U8 i = 0; i < 8; ++i) {
        if (!octList[i].empty()) {
            _childNodes[i] = createNode(octant[i], octList[i]);
            _activeNodes[i] = true;
            if (_childNodes[i]) {
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
    vec3<F32> global_min(_region.getMin());
    vec3<F32> global_max(_region.getMax());

    //go through all the objects in the list and find the extremes for their bounding areas.
    for (SceneGraphNode_wptr obj : _objects) {
        SceneGraphNode_ptr objPtr = obj.lock();
        if (objPtr) {
            const BoundingSphere& bs = objPtr->getBoundingSphereConst();
            const vec3<F32>& center = bs.getCenter();
            vec3<F32> radius(bs.getRadius());

            vec3<F32> local_min(center - radius);
            vec3<F32> local_max(center + radius);


            if (local_min.x < global_min.x) global_min.x = local_min.x;
            if (local_min.y < global_min.y) global_min.y = local_min.y;
            if (local_min.z < global_min.z) global_min.z = local_min.z;

            if (local_max.x > global_max.x) global_max.x = local_max.x;
            if (local_max.y > global_max.y) global_max.y = local_max.y;
            if (local_max.z > global_max.z) global_max.z = local_max.z;
        }
    }

    _region.setMin(global_min);
    _region.setMax(global_max);
}

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

    F32 floatHighX = to_float(highX);
    //see if our cube dimension is already at a power of 2. If it is, we don't have to do any work.
    for (I32 bit = 0; bit < 32; ++bit) {
        if (highX == 1 << bit) {
            _region.setMax(floatHighX, floatHighX, floatHighX);
            _region.translate(-offset);
            return;
        }
    }

    //We have a cube with non-power of two dimensions. We want to find the next highest power of two.
    //example: 63 -> 64; 65 -> 128;
    F32 x = std::pow(2.0f, std::ceil(std::log(floatHighX) / std::log(2.0f)));
    _region.setMax(x, x, x);
    _region.translate(-offset);
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

}; //namespace Divide
