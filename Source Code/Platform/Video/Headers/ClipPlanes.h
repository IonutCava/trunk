/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _CLIP_PLANES_H_
#define _CLIP_PLANES_H_

#include "Core/Math/Headers/Plane.h"

namespace Divide {
    enum class ClipPlaneIndex : U8 {
        CLIP_PLANE_0 = 0,
        CLIP_PLANE_1,
        CLIP_PLANE_2,
        CLIP_PLANE_3,
        CLIP_PLANE_4,
        CLIP_PLANE_5,
        COUNT
    };

    template<size_t N>
    struct ClipPlaneList {
        ClipPlaneList() : ClipPlaneList(Plane<F32>(0.0f, 0.0f, 0.0f, 0.0f))
        {
        }

        ClipPlaneList(const Plane<F32>& defaultValue)
        {
            _planes.fill(defaultValue);
            _active.fill(false);
        }

        void resetAll() {
            _planes.fill(Plane<F32>(0.0f, 0.0f, 0.0f, 0.0f));
            _active.fill(false);
        }

        void set(U32 index, const Plane<F32>& plane, bool active) {
            assert(index < N);

            _planes[index].set(plane.getEquation());
            _active[index] = active;
        }

        PlaneList<N> _planes;
        std::array<bool, N> _active;
    };
    
    typedef ClipPlaneList<to_base(ClipPlaneIndex::COUNT)> FrustumClipPlanes;
}; //namespace Divide

#endif //_CLIP_PLANES_H_