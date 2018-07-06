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

#ifndef _TEXT_LABEL_H_
#define _TEXT_LABEL_H_

#include "Core/Math/Headers/MathClasses.h"

class TextLabel {

public:
    TextLabel(const std::string& text,
              const std::string& font,
              const vec3<F32>& color,
              U32 textHeight = 16) : _width(1.0f),
                                     _text(text),
                                     _font(font),
                                     _height(textHeight),
                                     _color(color),
                                     _blurAmount(0.0f),
                                     _spacing(0.0f),
                                     _alignFlag(0),
                                     _bold(false),
                                     _italic(false)
    {
    }

    std::string _text;
    std::string _font;
    U32         _height;
    U32         _width;
    F32         _blurAmount;
    F32         _spacing;
    U32         _alignFlag; ///< Check font-stash alignment for details
    vec4<F32>   _color;
    bool        _bold;
    bool        _italic;

};

#endif