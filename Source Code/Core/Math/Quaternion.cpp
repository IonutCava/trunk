#include "Headers/Quaternion.h"


///From Ogre3D: http://www.ogre3d.org/docs/api/1.9/OgreVector3_8h_source.html#l00655
Quaternion<F32> rotationFromVToU(const vec3<F32>& v, const vec3<F32>& u, const vec3<F32> fallbackAxis) {
    // Based on Stan Melax's article in Game Programming Gems
    Quaternion<F32> q;
    // Copy, since cannot modify local
    vec3<F32> v0 = v;
    vec3<F32> v1 = u;
    v0.normalize();
    v1.normalize();
 
    F32 d = v0.dot(v1);
    // If dot == 1, vectors are the same
    if (d >= 1.0f) {
         return q;
    } else if (d < (1e-6f - 1.0f)) {
        if(!fallbackAxis.compare(VECTOR3_ZERO)) {
            // rotate 180 degrees about the fallback axis
            q.fromAxisAngle(fallbackAxis,RADIANS(M_PI));
        } else {
            // Generate an axis
            vec3<F32> axis;
            axis.cross(WORLD_X_AXIS, v);

            if (axis.isZeroLength()) // pick another if colinear
                axis.cross(WORLD_Y_AXIS, v);

            axis.normalize();
            q.fromAxisAngle(axis,RADIANS(M_PI));
        }
    } else {
        F32 s = square_root_tpl( (1+d)*2 );
        F32 invs = 1 / s;
 
        vec3<F32> c; c.cross(v0, v1);
        q.set(c.x * invs, c.y * invs, c.z * invs, s * 0.5f);
        q.normalize();
    }

    return q;
}
