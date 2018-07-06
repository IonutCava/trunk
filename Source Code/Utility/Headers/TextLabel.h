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

#ifndef _UTILITY_TEXT_LABEL_H_
#define _UTILITY_TEXT_LABEL_H_

#include "Core/Math/Headers/MathMatrices.h"
#include "Core/Headers/StringHelper.h"

namespace Divide {
class TextLabel {
   public:
    TextLabel(const stringImpl& string,
              const stringImpl& font,
              const vec3<U8>& colour,
              U32 fontSize)
        : _width(1),
          _font(font),
          _fontSize(fontSize),
          _colour(colour),
          _blurAmount(0.0f),
          _spacing(0.0f),
          _alignFlag(0),
          _bold(false),
          _italic(false)
          
    {
        text(string);
    }

    inline void text(const stringImpl& text) {
        Util::Split(text, '\n', _text);
    }

    inline const vectorImpl<stringImpl>& text() const {
        return _text;
    }

    stringImpl _font;
    U32 _fontSize;
    U32 _width;
    F32 _blurAmount;
    F32 _spacing;
    U32 _alignFlag;  ///< Check font-stash alignment for details
    vec4<U8> _colour;
    bool _bold;
    bool _italic;

private:
    vectorImpl<stringImpl> _text;
};

};  // namespace Divide

#endif //_UTILITY_TEXT_LABEL_H_