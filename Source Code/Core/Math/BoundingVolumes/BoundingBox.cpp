#include "stdafx.h"

#include "Headers/BoundingBox.h"
#include "Headers/BoundingSphere.h"

namespace Divide {

BoundingBox::BoundingBox() noexcept
    : _min(std::numeric_limits<F32>::max()),
      _max(std::numeric_limits<F32>::lowest())
{
}

BoundingBox::BoundingBox(const vec3<F32>& min, const vec3<F32>& max) noexcept
    : _min(min), 
      _max(max)
{
}

BoundingBox::BoundingBox(F32 minX, F32 minY, F32 minZ, F32 maxX, F32 maxY, F32 maxZ) noexcept
    : _min(minX, minY, minZ),
      _max(maxX, maxY, maxZ)
{
}

BoundingBox::BoundingBox(const std::vector<vec3<F32>>& points) noexcept
    : BoundingBox()
{
    createFromPoints(points);
}

BoundingBox::BoundingBox(const std::array<vec3<F32>, 8>& points) noexcept
    : BoundingBox()
{
    createFromPoints(points);
}

BoundingBox::~BoundingBox()
{
}

BoundingBox::BoundingBox(const BoundingBox& b) {
    // Lock w_lock(_lock);
    this->_min.set(b._min);
    this->_max.set(b._max);
}

void BoundingBox::operator=(const BoundingBox& b) {
    // Lock w_lock(_lock);
    this->_min.set(b._min);
    this->_max.set(b._max);
}

bool BoundingBox::containsBox(const BoundingBox& AABB2) const noexcept {
    return AABB2._min >= _min && AABB2._max <= _max;
}

bool BoundingBox::containsSphere(const BoundingSphere& bSphere) const noexcept {
    const vec3<F32>& center = bSphere.getCenter();
    const F32 radius = bSphere.getRadius();

    return center.x - _min.x > radius &&
           center.y - _min.y > radius &&
           center.z - _min.z > radius &&
           _max.x - center.x > radius &&
           _max.y - center.y > radius &&
           _max.z - center.z > radius;
}

bool BoundingBox::collision(const BoundingBox& AABB2) const noexcept {
    // SharedLock r_lock(_lock);
    const vec3<F32>& center = this->getCenter();
    const vec3<F32>& halfWidth = this->getHalfExtent();
    const vec3<F32>& otherCenter = AABB2.getCenter();
    const vec3<F32>& otherHalfWidth = AABB2.getHalfExtent();

    return std::abs(center.x - otherCenter.x) <= (halfWidth.x + otherHalfWidth.x) &&
           std::abs(center.y - otherCenter.y) <= (halfWidth.y + otherHalfWidth.y) &&
           std::abs(center.z - otherCenter.z) <= (halfWidth.z + otherHalfWidth.z);
}

bool BoundingBox::collision(const BoundingSphere& bSphere) const noexcept {
    const vec3<F32>& center = bSphere.getCenter();
    const vec3<F32>& min(getMin());
    const vec3<F32>& max(getMax());

    F32 dmin = 0;
    for (U8 i = 0; i < 3; ++i) {
             if (center[i] < min[i]) dmin += SQUARED(center[i] - min[i]);
        else if (center[i] > max[i]) dmin += SQUARED(center[i] - max[i]);
    }

    return dmin <= SQUARED(bSphere.getRadius());
}

/// Optimized method: http://www.cs.utah.edu/~awilliam/box/box.pdf
AABBRayResult BoundingBox::intersect(const Ray& r, F32 t0, F32 t1) const noexcept {
    // SharedLock r_lock(_lock);
    const vec3<F32> bounds[] = {_min, _max};

    F32 t_min = (bounds[r.sign[0]].x - r.origin.x) * r.inv_direction.x;
    F32 t_max = (bounds[1 - r.sign[0]].x - r.origin.x) * r.inv_direction.x;
    const F32 ty_min = (bounds[r.sign[1]].y - r.origin.y) * r.inv_direction.y;
    const F32 ty_max = (bounds[1 - r.sign[1]].y - r.origin.y) * r.inv_direction.y;

    if ((t_min > ty_max) || (ty_min > t_max)) {
        return { false, (t_min >= 0.0f ? t_min : t_max) };
    }

    if (ty_min > t_min) {
         t_min = ty_min;
    }

    if (ty_max < t_max) {
        t_max = ty_max;
    }

    const F32 tz_min = (bounds[r.sign[2]].z - r.origin.z) * r.inv_direction.z;
    const F32 tz_max = (bounds[1 - r.sign[2]].z - r.origin.z) * r.inv_direction.z;

    if ((t_min > tz_max) || (tz_min > t_max)) {
        return { false, (t_min >= 0.0f ? t_min : t_max) };
    }

    if (tz_min > t_min) {
        t_min = tz_min;
    }

    if (tz_max < t_max) {
        t_max = tz_max;
    }

    F32 t = t_min;
    if (t < 0.0f) {
        t = t_max;
    }

    return { t >= 0.0f, t };
}

void BoundingBox::transform(const mat4<F32>& mat) {
    transform(getMin(), getMax(), mat);
}

void BoundingBox::transform(const BoundingBox& initialBoundingBox, const mat4<F32>& mat) {
    transform(initialBoundingBox.getMin(), initialBoundingBox.getMax(), mat);
}

void BoundingBox::transform(vec3<F32> initialMin, vec3<F32> initialMax, const mat4<F32>& mat) {
    // UpgradeToWriteLock uw_lock(ur_lock);
    _min = _max = mat.getTranslation<F32>();

    F32 a = 0.0f, b = 0.0f;
    for (U8 i = 0; i < 3; ++i) {
        F32& min = _min[i];
        F32& max = _max[i];

        for (U8 j = 0; j < 3; ++j) {
            a = mat.m[j][i] * initialMin[j];
            b = mat.m[j][i] * initialMax[j];  // Transforms are usually row major

            if (a < b) {
                min += a;
                max += b;
            } else {
                min += b;
                max += a;
            }
        }
    }
}

F32 BoundingBox::nearestDistanceFromPointSquared(const vec3<F32>& pos) const noexcept {
    const vec3<F32>& center = getCenter();
    const vec3<F32>& hextent = getHalfExtent();
    return vec3<F32>(std::max(0.0f, std::abs(pos.x - center.x) - hextent.x),
                     std::max(0.0f, std::abs(pos.y - center.y) - hextent.y),
                     std::max(0.0f, std::abs(pos.z - center.z) - hextent.z)).lengthSquared();
}

};  // namespace Divide