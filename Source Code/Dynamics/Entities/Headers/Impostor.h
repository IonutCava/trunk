/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _IMPOSTOR_H_
#define _IMPOSTOR_H_

#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

namespace Divide {

/// Renders a sphere at the parent node's position using the desired radius;
class ImpostorSphere : public Sphere3D {
    template <typename T>
    friend class ImplResourceLoader;
    SET_DELETE_FRIEND
   protected:
    ImpostorSphere(const stringImpl& name, F32 radius = 1.0f);
    ~ImpostorSphere();
};

/// Renders a box at the parent node's position using the desired radius;
class ImpostorBox : public Box3D {
    template <typename T>
    friend class ImplResourceLoader;
    SET_DELETE_FRIEND
   protected:
    ImpostorBox(const stringImpl& name, const vec3<F32>& size = vec3<F32>());
    ~ImpostorBox();
};
};  // namespace Divide

#endif
