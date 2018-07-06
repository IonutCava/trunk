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

#ifndef _CORE_MATH_LINE_H_
#define _CORE_MATH_LINE_H_

#include "MathUtil.h"

namespace Divide {
    struct Line {
        vec3<F32> _startPoint;
        vec3<F32> _endPoint;
        vec4<U8> _colorStart;
        vec4<U8> _colorEnd;
        F32      _widthStart;
        F32      _widthEnd;

        Line()
        {
        }

        Line(const vec3<F32> &startPoint, const vec3<F32> &endPoint,
            const vec4<U8> &color)
            : Line(startPoint, endPoint, color, color)
        {
        }

        Line(const vec3<F32> &startPoint, const vec3<F32> &endPoint,
            const vec4<U8> &color, F32 width)
            : Line(startPoint, endPoint, color, color, width)
        {
        }

        Line(const vec3<F32> &startPoint, const vec3<F32> &endPoint,
            const vec4<U8> &colorStart, const vec4<U8>& colorEnd,
            F32 width)
            : Line(startPoint, endPoint, colorStart, colorEnd)
        {
            _widthStart = _widthEnd = width;
        }

        Line(const vec3<F32> &startPoint, const vec3<F32> &endPoint,
            const vec4<U8> &colorStart, const vec4<U8>& colorEnd)
            : _startPoint(startPoint),
            _endPoint(endPoint),
            _colorStart(colorStart),
            _colorEnd(colorEnd),
            _widthStart(1.0f),
            _widthEnd(1.0f)
        {
        }

        void color(U8 r, U8 g, U8 b, U8 a) {
            _colorStart.set(r, g, b, a);
            _colorEnd.set(r, g, b, a);
        }

        void width(F32 width) {
            _widthStart = _widthEnd = width;
        }

        void segment(F32 startX, F32 startY, F32 startZ,
            F32 endX, F32 endY, F32 endZ) {
            _startPoint.set(startX, startY, startZ);
            _endPoint.set(endX, endY, endZ);
        }
    };
}; //namespace Divide

#endif //_CORE_MATH_LINE_H_