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
#ifndef _UTILITY_TEXT_LABEL_H_
#define _UTILITY_TEXT_LABEL_H_

#include "Core/Math/Headers/MathMatrices.h"
#include "Core/Math/Headers/Dimension.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Hashable.h"

namespace Divide {

namespace Font {
    const static char* DIVIDE_DEFAULT = "DroidSerif-Regular.ttf"; /*"Test.ttf"*/
    const static char* BATANG = "Batang.ttf";
    const static char* DEJA_VU = "DejaVuSans.ttf";
    const static char* DROID_SERIF = "DroidSerif-Regular.ttf";
    const static char* DROID_SERIF_ITALIC = "DroidSerif-Italic.ttf";
    const static char* DROID_SERIF_BOLD = "DroidSerif-Bold.ttf";
};

class TextLabelStyle : public Hashable {
  protected:
    using TextLabelStyleMap = hashMap<size_t, TextLabelStyle>;
    static TextLabelStyleMap s_textLabelStyle;
    static SharedMutex s_textLableStyleMutex;
    static size_t s_defaultCacheValue;

    using FontNameHashMap = hashMap<size_t, Str64>;
    static FontNameHashMap s_fontName;

  public:
    static const TextLabelStyle& get(size_t textLabelStyleHash);
    static const TextLabelStyle& get(size_t textLabelStyleHash, bool& styleFound);
    static const Str64& fontName(size_t fontNameHash);

  public:
   TextLabelStyle(const char* font,
                  const UColour4& colour,
                  U8 fontSize);

    size_t getHash() const override;

    inline size_t font() const noexcept { return _font; }
    inline U8 fontSize() const noexcept { return _fontSize; }
    inline U8 width() const noexcept { return _width; }
    inline F32 blurAmount() const noexcept { return _blurAmount; }
    inline F32 spacing() const noexcept { return _spacing; }
    inline U32 alignFlag() const noexcept { return _alignFlag; }
    inline const UColour4& colour() const noexcept { return _colour; }
    inline bool bold() const noexcept { return _bold; }
    inline bool italic() const noexcept { return _italic; }


    inline void font(size_t font) noexcept { _font = font; _dirty = true; }
    inline void fontSize(U8 fontSize) noexcept { _fontSize = fontSize; _dirty = true; }
    inline void width(U8 width) noexcept { _width = width; _dirty = true; }
    inline void blurAmount(F32 blurAmount) noexcept { _blurAmount = blurAmount; _dirty = true; }
    inline void spacing(F32 spacing) noexcept { _spacing = spacing; _dirty = true; }
    inline void alignFlag(U32 alignFlag) noexcept { _alignFlag = alignFlag; _dirty = true; }
    inline void colour(const UColour4& colour) noexcept { _colour.set(colour); _dirty = true; }
    inline void bold(bool bold) noexcept { _bold = bold; _dirty = true; }
    inline void italic(bool italic) noexcept { _italic = italic; _dirty = true; }

 protected:
    size_t _font;
    U8 _fontSize;
    U8 _width;
    F32 _blurAmount;
    F32 _spacing;
    U32 _alignFlag;  ///< Check font-stash alignment for details
    UColour4 _colour;
    bool _bold;
    bool _italic;

 protected:
    mutable bool _dirty = true;
};

struct TextElement {
    using TextType = vectorEASTL<eastl::string>;

    TextElement(const TextLabelStyle& textLabelStyle,
                const RelativePosition2D& position)
        : TextElement(textLabelStyle.getHash(), position)
    {
    }

    TextElement(size_t textLabelStyleHash,
                const RelativePosition2D& position)
        : _textLabelStyleHash(textLabelStyleHash),
          _position(position)
    {
    }

    inline void text(const char* text, bool multiLine) {
        if (multiLine) {
            Util::Split(text, '\n', _text);
        } else {
            _text = { text };
        }
    }

    inline const TextType& text() const noexcept { return _text; }

    size_t _textLabelStyleHash;
    RelativePosition2D _position;

  private:
    TextType _text;
};

struct TextElementBatch {
    using BatchType = vectorEASTLFast<TextElement>;

    TextElementBatch() noexcept
    {
    }

    TextElementBatch(size_t elementCount)
    {
        _data.reserve(elementCount);
    }

    TextElementBatch(const TextElement& element)
        : _data {element}
    {
    }

    const BatchType& operator()() const noexcept { return _data; }

    inline bool empty() const noexcept { return _data.empty(); }

    BatchType _data;
};

};  // namespace Divide

#endif //_UTILITY_TEXT_LABEL_H_