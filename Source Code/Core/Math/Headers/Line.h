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
#ifndef _CORE_MATH_LINE_H_
#define _CORE_MATH_LINE_H_

#include "MathUtil.h"

namespace Divide {

class Line {
    public:
    Line() noexcept
        : _widthStart(1.0f),
          _widthEnd(1.0f)
    {
    }

    Line(const vec3<F32> &startPoint,
         const vec3<F32> &endPoint,
         const UColour4& colour) noexcept
        : Line(startPoint, endPoint, colour, colour)
    {
    }

    Line(const vec3<F32> &startPoint,
         const vec3<F32> &endPoint,
         const UColour4& colour,
         F32 width)
        : Line(startPoint, endPoint, colour, colour, width)
    {
    }

    Line(const vec3<F32> &startPoint,
         const vec3<F32> &endPoint,
         const UColour4& colourStart,
         const UColour4& colourEnd)
        : Line(startPoint, endPoint, colourStart, colourEnd, 1.0f)
    {
    }

    Line(const vec3<F32> &startPoint,
         const vec3<F32> &endPoint,
         const UColour4& colourStart,
         const UColour4& colourEnd,
         F32 width)
        : Line(startPoint, endPoint, colourStart, colourEnd, width, width)
    {
    }

    Line(const vec3<F32> &startPoint,
         const vec3<F32> &endPoint,
         const UColour4& colourStart,
         const UColour4& colourEnd,
         F32 widthStart,
         F32 widthEnd)
        : _startPoint(startPoint),
          _endPoint(endPoint),
          _colourStart(colourStart),
          _colourEnd(colourEnd),
          _widthStart(widthStart),
          _widthEnd(widthEnd)
    {
    }

    inline void colour(U8 r, U8 g, U8 b, U8 a) {
        colour(r, g, b, a,
              r, g, b, a);
    }

    inline void colour(U8 startR, U8 startG, U8 startB, U8 startA,
                      U8 endR,   U8 endG,   U8 endB,   U8 endA) {
        _colourStart.set(startR, startG, startB, startA);
        _colourEnd.set(endR, endG, endB, endA);
    }

    inline void width(F32 w) {
        width(w, w);
    }

    inline void width(F32 widthStart, F32 widthEnd) {
        _widthStart = widthStart;
        _widthEnd = widthEnd;
    }

    void segment(F32 startX, F32 startY, F32 startZ,
                 F32 endX, F32 endY, F32 endZ) {
        _startPoint.set(startX, startY, startZ);
        _endPoint.set(endX, endY, endZ);
    }

    inline const vec3<F32>& startPoint() const {
        return _startPoint;
    }

    inline const vec3<F32>& endPoint() const {
        return _endPoint;
    }

    inline const UColour4& colourStart() const {
        return _colourStart;
    }

    inline const UColour4& colourEnd() const {
        return _colourEnd;
    }

    inline F32 widthStart() const {
        return _widthStart;
    }

    inline F32 widthEnd() const {
        return _widthEnd;
    }

public:
    vec3<F32> _startPoint;
    vec3<F32> _endPoint;
    UColour4  _colourStart;
    UColour4  _colourEnd;
    F32       _widthStart;
    F32       _widthEnd;
};

}; //namespace Divide

#endif //_CORE_MATH_LINE_H_

