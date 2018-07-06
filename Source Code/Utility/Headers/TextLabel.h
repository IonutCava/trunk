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

#ifndef _TEXT_LABEL_H_
#define _TEXT_LABEL_H_

#include "Core/Math/Headers/MathMatrices.h"

namespace Divide {
class TextLabel {
   public:
    TextLabel(const stringImpl& string, const stringImpl& font,
              const vec3<F32>& color, U32 textHeight = 16)
        : _width(1),
          _font(font),
          _height(textHeight),
          _color(color),
          _blurAmount(0.0f),
          _spacing(0.0f),
          _alignFlag(0),
          _bold(false),
          _italic(false)
          
    {
        text(string);
    }

    inline void text(const stringImpl& text) {
        _text = text;
        size_t newLinePos = _text.find('\n');
        _multiLine = (newLinePos != stringImpl::npos) &&
                     (newLinePos != _text.length() - 1);
    }

    inline const stringImpl& text() const {
        return _text;
    }

    stringImpl _font;
    U32 _height;
    U32 _width;
    F32 _blurAmount;
    F32 _spacing;
    U32 _alignFlag;  ///< Check font-stash alignment for details
    vec4<F32> _color;
    bool _bold;
    bool _italic;
    bool _multiLine;

private:
    stringImpl _text;
};

};  // namespace Divide

#endif