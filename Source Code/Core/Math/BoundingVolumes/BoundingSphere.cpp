#include "Headers/BoundingSphere.h"

namespace Divide {
BoundingSphere::BoundingSphere() : _computed(false),
                                   _visibility(false),
                                   _dirty(true),
                                   _radius(0.0f)
{
    _center.reset();
}

BoundingSphere::BoundingSphere(const vec3<F32>& center, F32 radius) : _computed(false),
                                                                      _visibility(false),
                                                                      _dirty(true),
                                                                      _center(center),
                                                                      _radius(radius)
{
}

BoundingSphere::BoundingSphere(vectorImpl<vec3<F32> >& points) : BoundingSphere()
{
    CreateFromPoints(points);
}

BoundingSphere::BoundingSphere(const BoundingSphere& s) {
    //WriteLock w_lock(_lock);
    this->_computed = s._computed;
    this->_visibility = s._visibility;
    this->_dirty = s._dirty;
    this->_center = s._center;
    this->_radius = s._radius;
}

void BoundingSphere::operator=(const BoundingSphere& s) {
    //WriteLock w_lock(_lock);
    this->_computed = s._computed;
    this->_visibility = s._visibility;
    this->_dirty = s._dirty;
    this->_center = s._center;
    this->_radius = s._radius;
}

}; //namespace Divide