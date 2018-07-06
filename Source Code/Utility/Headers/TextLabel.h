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
    typedef hashMap<size_t, TextLabelStyle> TextLabelStyleMap;
    static TextLabelStyleMap s_textLabelStyleMap;
    static SharedLock s_textLableStyleMapMutex;
    static size_t s_defaultCacheValue;

    typedef hashMap<size_t, stringImpl> FontNameHashMap;
    static FontNameHashMap s_fontNameMap;

  public:
    static const TextLabelStyle& get(size_t textLabelStyleHash);
    static const TextLabelStyle& get(size_t textLabelStyleHash, bool& styleFound);
    static const stringImpl& fontName(size_t fontNameHash);

  public:
   TextLabelStyle(const char* font,
                  const UColour& colour,
                  U8 fontSize);

    size_t getHash() const override;

    inline size_t font() const { return _font; }
    inline U8 fontSize() const { return _fontSize; }
    inline U8 width() const { return _width; }
    inline F32 blurAmount() const { return _blurAmount; }
    inline F32 spacing() const { return _spacing; }
    inline U32 alignFlag() const { return _alignFlag; }
    inline const UColour& colour() const { return _colour; }
    inline bool bold() const { return _bold; }
    inline bool italic() const { return _italic; }


    inline void font(size_t font) { _font = font; _dirty = true; }
    inline void fontSize(U8 fontSize) { _fontSize = fontSize; _dirty = true; }
    inline void width(U8 width) { _width = width; _dirty = true; }
    inline void blurAmount(F32 blurAmount) { _blurAmount = blurAmount; _dirty = true; }
    inline void spacing(F32 spacing) { _spacing = spacing; _dirty = true; }
    inline void alignFlag(U32 alignFlag) { _alignFlag = alignFlag; _dirty = true; }
    inline void colour(const UColour& colour) { _colour.set(colour); _dirty = true; }
    inline void bold(bool bold) { _bold = bold; _dirty = true; }
    inline void italic(bool italic) { _italic = italic; _dirty = true; }

 protected:
    size_t _font;
    U8 _fontSize;
    U8 _width;
    F32 _blurAmount;
    F32 _spacing;
    U32 _alignFlag;  ///< Check font-stash alignment for details
    UColour _colour;
    bool _bold;
    bool _italic;

 protected:
    mutable bool _dirty = true;
};

struct TextElement {
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

    inline void text(const stringImpl& text) {
        Util::Split(text, '\n', _text);
    }

    inline const vectorFast<stringImpl>& text() const {
        return _text;
    }

    size_t _textLabelStyleHash;
    RelativePosition2D _position;

  private:
    vectorFast<stringImpl> _text;
};

struct TextElementBatch {
    typedef vectorFast<TextElement> BatchType;

    TextElementBatch()
    {
    }

    TextElementBatch(const TextElement& element)
        : _data {element}
    {
    }

    const BatchType& operator()() const {
        return _data;
    }

    BatchType _data;
};

};  // namespace Divide

#endif //_UTILITY_TEXT_LABEL_H_