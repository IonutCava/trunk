/*
 * Ray class, for use with the optimized ray-box
 * intersection test described in:
 *
 *      Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
 *      "An Efficient and Robust Ray-Box Intersection Algorithm"
 *      Journal of graphics tools, 10(1):49-54, 2005
 *
 */

/*
   Copyright (c) 2017 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _RAY_H_
#define _RAY_H_

#include "Core/Math/Headers/MathMatrices.h"

namespace Divide {

class Ray {
   public:
    Ray() : Ray(VECTOR3_ZERO, WORLD_Y_AXIS)
    {
    }

    Ray(const vec3<F32> &o, const vec3<F32> &d)
        : origin(o),
          direction(d),
          inv_direction(1.0f / d.x, 1.0f / d.y, 1.0f / d.z),
          sign{(inv_direction.x < 0.0f),
               (inv_direction.y < 0.0f),
               (inv_direction.z < 0.0f)}
    {
    }

    Ray(const Ray &r)
    {
        origin.set(r.origin);
        direction.set(r.direction);
        inv_direction.set(r.inv_direction);
        memcpy(sign, r.sign, 3 * sizeof(I32));
    }

    Ray& operator=(const Ray &r) {
        origin.set(r.origin);
        direction.set(r.direction);
        inv_direction.set(r.inv_direction);
        memcpy(sign, r.sign, 3 * sizeof(I32));
        return *this;
    }

    inline void identity() {
        set(VECTOR3_ZERO, WORLD_Y_AXIS);
    }

    inline void set(const vec3<F32> &o, const vec3<F32> &d) {
        origin.set(o);
        direction.set(d);
        inv_direction.set(1.0f / d.x, 1.0f / d.y, 1.0f / d.z);
        sign[0] = (inv_direction.x < 0.0f);
        sign[1] = (inv_direction.y < 0.0f);
        sign[2] = (inv_direction.z < 0.0f);
    }

    vec3<F32> origin;
    vec3<F32> direction;
    vec3<F32> inv_direction;
    I32 sign[3];
};

};  // namespace Divide

#endif