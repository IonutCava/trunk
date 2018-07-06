#include "Headers/IntersectionRecord.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

IntersectionRecord::IntersectionRecord() :
    _distance(std::numeric_limits<D32>::max()),
    _hasHit(false)
{
}

IntersectionRecord::IntersectionRecord(const vec3<F32>& hitPos,
                                       const vec3<F32>& hitNormal,
                                       const Ray& ray,
                                       D32 distance) :
    _position(hitPos),
    _normal(hitNormal),
    _ray(ray),
    _distance(distance),
    _hasHit(true)
{
}

/// Creates a new intersection record indicating whether there was a hit or not and the object which was hit.
IntersectionRecord::IntersectionRecord(SceneGraphNode_wptr hitObject) :
    _intersectedObject1(hitObject),
    _distance(0.0),
    _hasHit(hitObject.lock() != nullptr)
{
}

void IntersectionRecord::reset() 
{
    _ray.identity();
    _hasHit = false;
    _distance = std::numeric_limits<D32>::max();
    _intersectedObject1.reset();
    _intersectedObject2.reset();
}

bool IntersectionRecord::operator==(const IntersectionRecord& otherRecord)
{
    SceneGraphNode_ptr node11 = _intersectedObject1.lock();
    SceneGraphNode_ptr node12 = _intersectedObject2.lock();
    SceneGraphNode_ptr node21 = otherRecord._intersectedObject1.lock();
    SceneGraphNode_ptr node22 = otherRecord._intersectedObject2.lock();

    if (node11 && node12 && node21 && node22) {
        if (node21->getGUID() == node11->getGUID() && node22->getGUID() == node12->getGUID()) {
            return true;
        }
        if (node21->getGUID() == node12->getGUID() && node22->getGUID() == node11->getGUID()) {
            return true;
        }
    }

    return false;

}
};