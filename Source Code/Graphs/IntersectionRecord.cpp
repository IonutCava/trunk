#include "stdafx.h"

#include "Headers/IntersectionRecord.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

IntersectionRecord::IntersectionRecord() :
    _distance(std::numeric_limits<D64>::max()),
    _hasHit(false)
{
}

IntersectionRecord::IntersectionRecord(const vec3<F32>& hitPos,
                                       const vec3<F32>& hitNormal,
                                       const Ray& ray,
                                       D64 distance) :
    _position(hitPos),
    _normal(hitNormal),
    _ray(ray),
    _distance(distance),
    _hasHit(true)
{
}

/// Creates a new intersection record indicating whether there was a hit or not and the object which was hit.
IntersectionRecord::IntersectionRecord(SceneGraphNode* hitObject) :
    _intersectedObject1(hitObject),
    _distance(0.0),
    _hasHit(hitObject != nullptr)
{
}

void IntersectionRecord::reset() 
{
    _ray.identity();
    _hasHit = false;
    _distance = std::numeric_limits<D64>::max();
    _intersectedObject1 = nullptr;
    _intersectedObject2 = nullptr;
}

bool IntersectionRecord::operator==(const IntersectionRecord& otherRecord)
{
    SceneGraphNode* node11 = _intersectedObject1;
    SceneGraphNode* node12 = _intersectedObject2;
    SceneGraphNode* node21 = otherRecord._intersectedObject1;
    SceneGraphNode* node22 = otherRecord._intersectedObject2;

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