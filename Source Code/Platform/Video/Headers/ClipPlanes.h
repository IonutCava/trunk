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

#ifndef _CLIP_PLANES_H_
#define _CLIP_PLANES_H_

#include "Core/Math/Headers/Plane.h"

namespace Divide {
    struct ClipPlaneList {
        ClipPlaneList(U32 size, const Plane<F32>& defaultValue)
            : _planes(size, defaultValue),
              _active(size, false)
        {
        }

        void resize(U32 size, const Plane<F32>& defaultValue) {
            _planes.resize(size, defaultValue);
            _active.resize(size, false);
        }

        void set(U32 index, const Plane<F32>& plane, bool active) {
            assert(index < _planes.size());

            _planes[index].set(plane.getEquation());
            _active[index] = active;
        }

        PlaneList _planes;
        vectorImpl<bool> _active;
    };

}; //namespace Divide

#endif //_CLIP_PLANES_H_