#include "Headers/BoundingBox.h"

namespace Divide {
BoundingBox::BoundingBox()
    : GUIDWrapper(), _computed(false), _pointsDirty(true) {
    _min.set(std::numeric_limits<F32>::max());
    _max.set(std::numeric_limits<F32>::min());
}

BoundingBox::BoundingBox(const vec3<F32>& min, const vec3<F32>& max)
    : GUIDWrapper(),
      _computed(false),
      _pointsDirty(true),
      _min(min),
      _max(max) {}

BoundingBox::BoundingBox(vectorImpl<vec3<F32> >& points) : BoundingBox() {
    CreateFromPoints(points);
}

BoundingBox::~BoundingBox() {}

BoundingBox::BoundingBox(const BoundingBox& b) : GUIDWrapper() {
    // WriteLock w_lock(_lock);
    this->_computed = b._computed;
    this->_min = b._min;
    this->_max = b._max;
    this->_oldMatrix = b._oldMatrix;
    for (U8 i = 0; i < 8; ++i) {
        this->_points[i].set(b._points[i]);
    }
    this->_pointsDirty = true;
}

void BoundingBox::operator=(const BoundingBox& b) {
    // WriteLock w_lock(_lock);
    this->_computed = b._computed;
    this->_min = b._min;
    this->_max = b._max;
    this->_oldMatrix = b._oldMatrix;
    this->_pointsDirty = true;
    memcpy(_points, b._points, sizeof(vec3<F32>) * 8);
}

bool BoundingBox::Collision(const BoundingBox& AABB2) const {
    // ReadLock r_lock(_lock);
    const vec3<F32>& center = this->getCenter();
    const vec3<F32>& halfWidth = this->getHalfExtent();
    const vec3<F32>& otherCenter = AABB2.getCenter();
    const vec3<F32>& otherHalfWidth = AABB2.getHalfExtent();

    bool x = std::abs(center[0] - otherCenter[0]) <=
             (halfWidth[0] + otherHalfWidth[0]);
    bool y = std::abs(center[1] - otherCenter[1]) <=
             (halfWidth[1] + otherHalfWidth[1]);
    bool z = std::abs(center[2] - otherCenter[2]) <=
             (halfWidth[2] + otherHalfWidth[2]);

    return x && y && z;
}

/// Optimized method
bool BoundingBox::Intersect(const Ray& r, F32 t0, F32 t1) const {
    // ReadLock r_lock(_lock);
    vec3<F32> bounds[] = {_min, _max};

    F32 t_min = (bounds[r.sign[0]].x - r.origin.x) * r.inv_direction.x;
    F32 t_max = (bounds[1 - r.sign[0]].x - r.origin.x) * r.inv_direction.x;
    F32 ty_min = (bounds[r.sign[1]].y - r.origin.y) * r.inv_direction.y;
    F32 ty_max = (bounds[1 - r.sign[1]].y - r.origin.y) * r.inv_direction.y;

    if ((t_min > ty_max) || (ty_min > t_max)) return false;

    if (ty_min > t_min) t_min = ty_min;
    if (ty_max < t_max) t_max = ty_max;

    F32 tz_min = (bounds[r.sign[2]].z - r.origin.z) * r.inv_direction.z;
    F32 tz_max = (bounds[1 - r.sign[2]].z - r.origin.z) * r.inv_direction.z;

    if ((t_min > tz_max) || (tz_min > t_max)) return false;

    if (tz_min > t_min) t_min = tz_min;
    if (tz_max < t_max) t_max = tz_max;

    return ((t_min < t1) && (t_max > t0));
}

bool BoundingBox::Transform(const BoundingBox& initialBoundingBox,
                            const mat4<F32>& mat, bool force) {
    // UpgradableReadLock ur_lock(_lock);
    if (!force && _oldMatrix == mat) {
        return false;
    }

    _oldMatrix.set(mat);

    const F32* oldMin = &initialBoundingBox._min[0];
    const F32* oldMax = &initialBoundingBox._max[0];

    // UpgradeToWriteLock uw_lock(ur_lock);
    _min.set(mat[12], mat[13], mat[14]);
    _max.set(mat[12], mat[13], mat[14]);

    F32 a, b;
    for (U8 i = 0; i < 3; ++i) {
        F32& min = _min[i];
        F32& max = _max[i];
        for (U8 j = 0; j < 3; ++j) {
            a = mat.m[j][i] * oldMin[j];
            b = mat.m[j][i] * oldMax[j];  // Transforms are usually row major

            if (a < b) {
                min += a;
                max += b;
            } else {
                min += b;
                max += a;
            }
        }
    }
    _pointsDirty = true;
    return true;
}

F32 BoundingBox::nearestDistanceFromPointSquared(const vec3<F32>& pos) const {
    const vec3<F32>& center = getCenter();
    const vec3<F32>& hextent = getHalfExtent();
    _cacheVector.set(std::max(0.0f, fabsf(pos.x - center.x) - hextent.x),
                     std::max(0.0f, fabsf(pos.y - center.y) - hextent.y),
                     std::max(0.0f, fabsf(pos.z - center.z) - hextent.z));
    return _cacheVector.lengthSquared();
}

};  // namespace Divide