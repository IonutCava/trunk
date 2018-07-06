#include "Headers/BoundingSphere.h"

namespace Divide {
BoundingSphere::BoundingSphere()
    : _visibility(false), _dirty(true), _radius(0.0f)
{
    _center.reset();
}

BoundingSphere::BoundingSphere(const vec3<F32>& center, F32 radius)
    : _visibility(false),
      _dirty(true),
      _center(center),
      _radius(radius)
{
}

BoundingSphere::BoundingSphere(const vectorImpl<vec3<F32> >& points)
    : BoundingSphere()
{
    createFromPoints(points);
}

BoundingSphere::BoundingSphere(const BoundingSphere& s) {
    // WriteLock w_lock(_lock);
    this->_visibility = s._visibility;
    this->_dirty = s._dirty;
    this->_center = s._center;
    this->_radius = s._radius;
}

void BoundingSphere::operator=(const BoundingSphere& s) {
    // WriteLock w_lock(_lock);
    this->_visibility = s._visibility;
    this->_dirty = s._dirty;
    this->_center = s._center;
    this->_radius = s._radius;
}

bool BoundingSphere::containsBoundingBox(const BoundingBox& AABB) const {
    bool inside = true;
    std::array<vec3<F32>, 8> points = AABB.getPoints();

    for (U8 i = 0; i < 8; ++i) {
        if (containsPoint(points[i]))
        {
            inside = false;
            break;
        }
    }

    return inside;
    
}

bool BoundingSphere::containsPoint(const vec3<F32>& point) const {
    F32 distanceSQ = _center.distanceSquared(point);
    return distanceSQ <= (_radius * _radius);
}

bool BoundingSphere::collision(const BoundingSphere& sphere2) const {
    F32 radiusSq = _radius + sphere2._radius;
    radiusSq *= radiusSq;
    return _center.distanceSquared(sphere2._center) <= radiusSq;
}

};  // namespace Divide