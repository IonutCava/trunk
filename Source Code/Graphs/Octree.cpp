#include "Headers/Octree.h"

namespace Divide {

bool Octree::_treeReady = false;
bool Octree::_treeBuilt = false;
std::queue<SceneGraphNode_ptr> Octree::_pendingInsertion;

Octree::Octree() : _maxLifespan(8),
                   _curLife(-1),
                   _hasChildren(false)
{
    _region.set(VECTOR3_ZERO, VECTOR3_ZERO);
    _activeNodes.fill(false);
}

Octree::Octree(const BoundingBox& rootAABB)
    : Octree()
{
    _region.set(rootAABB);
}

Octree::Octree(const BoundingBox& rootAABB,
               vectorImpl<SceneGraphNode_wptr> objects)
     : Octree(rootAABB)
{
    _objects.insert(std::cend(_objects),
                    std::cbegin(objects),
                    std::cend(objects));
}

Octree::~Octree()
{
}

void Octree::update(const U64 deltaTime) {
    if (_treeBuilt) {
        if (_objects.empty()) {
            if (!_hasChildren) {
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
    }

    typedef hashMapImpl<I64, SceneGraphNode_wptr> MovedMap;

    size_t listSize = _objects.size();
    for (size_t i = 0; i < listSize; ++i) {
        SceneGraphNode_ptr obj = _objects[i].lock();
        if (obj && !obj->isActive()) {
            MovedMap::const_iterator it = _movedObjects.find(obj->getGUID());
            if (it != std::cend(_movedObjects)) {
                _movedObjects.erase(it);
            }
            _objects.erase(std::cbegin(_objects) + i--);
            listSize--;
        }
    }

    for (U8 i = 0; i < 8; ++i) {
        if (_activeNodes[i]) {
            assert(_childNodes[i]);
            _childNodes[i]->update(deltaTime);
        }
    }

    for (MovedMap::value_type& it : _movedObjects) {
        Octree*  current = this;
        SceneGraphNode_ptr movedObj = it.second.lock();
        if (!movedObj) {
            continue;
        }

        const BoundingBox& bb = movedObj->getBoundingBoxConst();
        const BoundingSphere& bs = movedObj->getBoundingSphereConst();

        if (bb.getExtent() > VECTOR3_ZERO) {
            while(!current->_region.containsBox(bb)) {
                if (current->_parent != nullptr) {
                    current = current->_parent.get();
                } else {
                    break;
                }
            }
        } else {
            while (!current->_region.containsSphere(bs)) {
                if (current->_parent != nullptr) {
                    current = current->_parent.get();
                } else {
                    break;
                }
            }
        }

        for (size_t i = 0; i < _objects.size(); ++i) {
            if (_objects[i].lock()->getGUID() == movedObj->getGUID()) {
                _objects.erase(std::cbegin(_objects) + i);
                break;
            }
        }
        current->insert(movedObj);
    }

    for (U8 i = 0; i < 8; ++i) {
        if (_activeNodes[i] && _childNodes[i]->_curLife == 0) {
            _childNodes[i].reset();
            _activeNodes[i] = false;
        }
    }

    /*// root node
    if (_parent == nullptr) {
        vectorImpl<IntersectionRecord> irList = getIntersection(vectorImpl<SceneGraphNode_wptr>());

        for(IntersectionRecord ir : irList) {
            if (ir.PhysicalObject != nullptr)
                ir.PhysicalObject.handleIntersection(ir);
            if (ir.OtherPhysicalObject != null)
                ir.OtherPhysicalObject.handleIntersection(ir);
        }
    }*/
}

void Octree::registerMovedNode(SceneGraphNode& node) {
    I64 guid = node.getGUID();
    if (_movedObjects.find(guid) == std::cend(_movedObjects)) {
        _movedObjects[guid] = node.shared_from_this();
    }
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

    vec3<F32> center(_region.getCenter());
    vec3<F32> regionMin(_region.getMin());
    vec3<F32> regionMax(_region.getMax());
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
        octList[i].resize(8);
    }

    vectorImpl<U8> delist;
    delist.reserve(8);
    for (SceneGraphNode_wptr obj : _objects) {
        SceneGraphNode_ptr objPtr = obj.lock();
        if (objPtr) {
            const BoundingBox& bb = objPtr->getBoundingBoxConst();
            const BoundingSphere& bs = objPtr->getBoundingSphereConst();
            if (bb.getExtent() > VECTOR3_ZERO) {
                for (U8 i = 0; i < 8; ++i) {
                    if (octant[i].containsBox(bb)) {
                        octList[i].push_back(objPtr);
                        delist.push_back(i);
                        break;
                    }
                }
            } else if (bs.getRadius() > 0.0f) {
                for (U8 i = 0; i < 8; ++i) {
                    if (octant[i].containsSphere(bs)) {
                        octList[i].push_back(objPtr);
                        delist.push_back(i);
                        break;
                    }
                }
            }
        }
    }
    for (U8 i : delist) {
        _objects.erase(std::cbegin(_objects) + i);
    }
    for (U8 i = 0; i < 8; ++i) {
        if (!octList[i].empty()) {
            _childNodes[i] = createNode(octant[i], octList[i]);
            _activeNodes[i] = true;
            if (_childNodes[i]) {
                _childNodes[i]->buildTree();
            }
            _hasChildren = true;
        }
    }
    _treeBuilt = true;
    _treeReady = true;
}

void Octree::insert(SceneGraphNode_wptr object) {
    if (_objects.size() <= 1) {
        bool valid = true;
        for (U8 i = 0; i < 8; ++i) {
            if (_activeNodes[i]) {
                valid = false;
                break;
            }
        }
        if (valid) {
            _objects.push_back(object);
            return;
        }
    }

    vec3<F32> dimensions(_region.getExtent());
    //Check to see if the dimensions of the box are greater than the minimum dimensions
    if (dimensions.x <= MIN_SIZE && dimensions.y <= MIN_SIZE && dimensions.z <= MIN_SIZE)
    {
        _objects.push_back(object);
        return;
    }

    vec3<F32> center(_region.getCenter());

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
    } else if (bs .getRadius() > 0 && _region.containsSphere(bs)) {
        bool found = false;
        //we will try to place the object into a child node. If we can't fit it in a child node, then we insert it into the current node object list.
        for (U8  i = 0; i < 8; ++i) {
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

void Octree::findEnclosingCube() {
    //we can't guarantee that all bounding regions will be relative to the origin, so to keep the math
    //simple, we're going to translate the existing region to be oriented off the origin and remember the translation.
    //find the min offset from (0,0,0) and translate by it for a short while
    vec3<F32> offset(VECTOR3_ZERO - _region.getMin());
    _region.setMin(_region.getMin() + offset);
    _region.setMax(_region.getMax() + offset);

    //A 3D rectangle has a length, height, and width. Of those three dimensions, we want to find the largest dimension.
    //the largest dimension will be the minimum dimensions of the cube we're creating.
    F32 highX = std::ceil(std::max(std::max(_region.getMax().x, _region.getMax().y), _region.getMax().z));

    //see if our cube dimension is already at a power of 2. If it is, we don't have to do any work.
    for (I32 bit = 0; bit < 32; ++bit) {
        if (highX == to_float(1 << bit)) {
            _region.setMax(highX, highX, highX);
            _region.setMin(_region.getMin() - offset);
            _region.setMax(_region.getMax() - offset);
        }
    }

    //We have a cube with non-power of two dimensions. We want to find the next highest power of two.
    //example: 63 -> 64; 65 -> 128;
    F32 x = std::pow(2.0f, std::ceil(std::log(highX) / std::log(2.0f)));
    _region.setMax(x, x, x);
    _region.setMin(_region.getMin() - offset);
    _region.setMax(_region.getMax() - offset);
}

std::shared_ptr<Octree>
Octree::createNode(const BoundingBox& region,
                   vectorImpl<SceneGraphNode_wptr> objects) {
    if (objects.empty()) {
        return nullptr;
    }
    std::shared_ptr<Octree> ret = std::make_shared<Octree>(region, objects);
    ret->_parent = shared_from_this();
    return ret;
}

std::shared_ptr<Octree>
Octree::createNode(const BoundingBox& region, SceneGraphNode_wptr object) {
    vectorImpl<SceneGraphNode_wptr> objList;
    objList.push_back(object);
    return createNode(region, objList);
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

}; //namespace Divide
