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

#include "Core/Math/Headers/MathClasses.h"
#include "Core/Headers/Singleton.h"
#include "Core/Math/Headers/Plane.h"

#define FRUSTUM_OUT			0
#define FRUSTUM_IN			1
#define FRUSTUM_INTERSECT	2

class BoundingBox;
DEFINE_SINGLETON( Frustum )

public:
    void Extract(const vec3<F32>& eye);
    inline void Extract() { Extract(_eyePos); }

    bool ContainsPoint(const vec3<F32>& point) const;
    I8  ContainsBoundingBox(const BoundingBox& bbox) const;
    I8  ContainsSphere(const vec3<F32>& center, F32 radius) const;

    inline const vec3<F32>&  getEyePos()		const { return _eyePos; }
    inline const vec2<F32>&  getZPlanes()       const { return _zPlanes; }
    inline const F32         getVerticalFoV()   const { return _verticalFoV; }
    inline const F32         getHorizontalFoV() const { return Util::yfov_to_xfov(_verticalFoV, _aspectRatio); }
    inline const F32         getAspectRatio()   const { return _aspectRatio; }
    inline const vec3<F32>&  getPoint(U8 index)       { updatePoints(); return _frustumPoints[index]; }

    inline void setAspectRatio(F32 ratio)  { _aspectRatio = ratio;}
    inline void setVerticalFoV(F32 vFoV)   { _verticalFoV = vFoV; }
    inline void setHorizontalFoV(F32 hFoV) { _verticalFoV = Util::xfov_to_yfov(hFoV, _aspectRatio); }
    
    void setProjection(F32 aspectRatio, F32 verticalFoV, const vec2<F32>& zPlanes, bool updateGFX = true);

    inline void setOrtho(const vec4<F32>& rect, const vec2<F32>& zPlanes) {
        _orthoRect = rect;
        _zPlanes = zPlanes;
    }

    inline void lock() {
        _eyePosLock = _eyePos;
        _zPlanesLock = _zPlanes;
        _verticalFoVLock = _verticalFoV;
        _aspectRatioLock = _aspectRatio;
        _orthoRectLock = _orthoRect;
    }

    inline void unlock(bool extract = false) {
        _eyePos = _eyePosLock;
        _zPlanes = _zPlanesLock;
        _aspectRatio = _aspectRatioLock;
        _verticalFoV = _verticalFoVLock;
        _orthoRect = _orthoRectLock;
        if (extract)
            Extract(_eyePos);
    }
protected:
    Frustum();

private:
    void intersectionPoint(const Plane<F32> & a, const Plane<F32> & b, const Plane<F32> & c, vec3<F32>& outResult);
    void updatePoints();

private:
    F32        _verticalFoV, _verticalFoVLock;
    F32        _aspectRatio, _aspectRatioLock;
    bool       _pointsDirty;
    vec3<F32>  _eyePos,  _eyePosLock;
    vec2<F32>  _zPlanes, _zPlanesLock;
    vec4<F32>  _orthoRect, _orthoRectLock;
    vec3<F32>  _frustumPoints[8];
    Plane<F32> _frustumPlanes[6];

END_SINGLETON

#endif
