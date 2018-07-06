/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include "Core/Math/Headers/MathMatrices.h"
#include "Core/Math/Headers/Plane.h"

namespace Divide {

class Camera;
class BoundingBox;
class Frustum {

protected:
    friend class Camera;
    Frustum(Camera& parentCamera);
    void Extract();

public:
    enum FrustCollision {
        FRUSTUM_OUT		  = 0,
        FRUSTUM_IN		  =	1,
        FRUSTUM_INTERSECT =	2
    };

    FrustCollision ContainsPoint(const vec3<F32>& point) const;
    FrustCollision ContainsBoundingBox(const BoundingBox& bbox) const;
    FrustCollision ContainsSphere(const vec3<F32>& center, F32 radius) const;

    // Get the frustum corners in WorldSpace. cornerWS must be a vector with at least 8 allocated slots
    void getCornersWorldSpace(vectorImpl<vec3<F32> >& cornersWS);
    // Get the frustum corners in ViewSpace. cornerVS must be a vector with at least 8 allocated slots
    void getCornersViewSpace(vectorImpl<vec3<F32> >& cornersVS);

private:
    /// Get the point where the 3 specified planes intersect
    void intersectionPoint(const Plane<F32> & a, 
                           const Plane<F32> & b, 
                           const Plane<F32> & c, 
                           vec3<F32>& outResult);
    void updatePoints();

private:
    Camera&    _parentCamera;
    bool       _pointsDirty;
    vec3<F32>  _frustumPoints[8];
    Plane<F32> _frustumPlanes[6];
    mat4<F32>  _viewProjectionMatrixCache;
    mat4<F32>  _viewProjectionMatrixCacheOld;

};

}; //namespace Divide

#endif
