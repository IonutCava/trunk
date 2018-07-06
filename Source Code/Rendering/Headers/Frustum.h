/*
   Copyright (c) 2013 DIVIDE-Studio
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

#define FRUSTUM_OUT			0
#define FRUSTUM_IN			1
#define FRUSTUM_INTERSECT	2

class BoundingBox;
DEFINE_SINGLETON( Frustum )

public:
    void Extract(const vec3<F32>& eye);
    bool ContainsPoint(const vec3<F32>& point) const;
    I8  ContainsBoundingBox(const BoundingBox& bbox) const;
    I8  ContainsSphere(const vec3<F32>& center, F32 radius) const;

    inline void setZPlanes(const vec2<F32>& zPlanes)       {_zPlanes = zPlanes;}
    inline const vec3<F32>& getEyePos()				 const {return _eyePos;}
    inline const vec2<F32>& getZPlanes()             const {return _zPlanes;}

private:
    vec3<F32> _eyePos;
    vec2<F32> _zPlanes;
    vec4<F32> _frustumPlanes[6];
    mat4<F32> _viewProjectionMatrixCache;

END_SINGLETON

#endif
