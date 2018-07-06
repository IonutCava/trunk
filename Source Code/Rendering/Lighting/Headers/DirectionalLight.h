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

#ifndef _DIRECTIONAL_LIGHT_H_
#define _DIRECTIONAL_LIGHT_H_

#include "Light.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

class DirectionalLight : public Light {
public:
	DirectionalLight(U8 slot);
	~DirectionalLight();

    inline U8   csmSplitCount()               const { return _csmSplitCount; }
    inline void csmSplitCount(U8 splitCount)        { _csmSplitCount = splitCount; }

    inline F32  csmNearClipOffset()           const { return _csmNearClipOffset; }
    inline void csmNearClipOffset(F32 offset)       { _csmNearClipOffset = offset; }

    inline F32  csmSplitLogFactor()           const { return _csmSplitLogFactor; }
    inline void csmSplitLogFactor(F32 factor)       { _csmSplitLogFactor = factor; }

    inline bool csmStabilize()                const { return _csmStabilize; }
    inline void csmStabilize(bool state)            { _csmStabilize = state; }

protected:
    ///CSM split count
    U8   _csmSplitCount;
    ///CSM split weight
    F32  _csmSplitLogFactor;
    ///CSM extra back up distance for light position
    F32  _csmNearClipOffset;
    ///Try to stabilize shadow maps by using a bounding sphere for the frustum. If false, a bounding box is used instead
    bool _csmStabilize;
};

}; //namespace Divide

#endif