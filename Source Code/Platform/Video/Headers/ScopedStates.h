/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _HARDWARE_VIDEO_SCOPED_STATE_H_
#define _HARDWARE_VIDEO_SCOPED_STATE_H_

#include "Core/Headers/NonCopyable.h"
#include "Core/Math/Headers/MathVectors.h"
#include "Platform/DataTypes/Headers/PlatformDefines.h"

namespace Divide {
namespace GFX {
class ScopedLineWidth : private NonCopyable {
   public:
    explicit ScopedLineWidth(F32 width);
    ~ScopedLineWidth();
};

class ScopedRasterizer : private NonCopyable {
   public:
    explicit ScopedRasterizer(bool state);
    ~ScopedRasterizer();

   private:
    bool _rasterizerState;
};

class Scoped2DRendering : private NonCopyable {
   public:
    explicit Scoped2DRendering(bool state);
    ~Scoped2DRendering();

   private:
    bool _2dRenderingState;
};

class ScopedViewport : private NonCopyable {
   public:
    explicit ScopedViewport(const vec4<I32>& viewport);
    explicit ScopedViewport(I32 x, I32 y, I32 width, I32 height) 
        : ScopedViewport(vec4<I32>(x, y, width, height))
    {
    }
    ~ScopedViewport();
};

};  // namespace GFX
};  // namespace Divide

#endif  //_HARDWARE_VIDEO_SCOPED_STATE_H_