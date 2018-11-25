#include "stdafx.h"

#include "Headers/TextLabel.h"

namespace Divide {

TextLabelStyle::TextLabelStyleMap TextLabelStyle::s_textLabelStyleMap;
SharedMutex TextLabelStyle::s_textLableStyleMapMutex;
size_t TextLabelStyle::s_defaultCacheValue = 0;

TextLabelStyle::FontNameHashMap TextLabelStyle::s_fontNameMap;

TextLabelStyle::TextLabelStyle(const char* font,
                               const UColour& colour,
                               U8 fontSize)
  : _width(1),
    _font(_ID(font)),
    _fontSize(fontSize),
    _colour(colour),
    _blurAmount(0.0f),
    _spacing(0.0f),
    _alignFlag(0),
    _bold(false),
    _italic(false),
    _dirty(true)
{
    // First one found
    if (s_defaultCacheValue == 0) {
        s_defaultCacheValue = getHash();

        s_fontNameMap[_ID(Font::DIVIDE_DEFAULT)] = Font::DIVIDE_DEFAULT;
        s_fontNameMap[_ID(Font::BATANG)] = Font::BATANG;
        s_fontNameMap[_ID(Font::DEJA_VU)] = Font::DEJA_VU;
        s_fontNameMap[_ID(Font::DROID_SERIF)] = Font::DROID_SERIF;
        s_fontNameMap[_ID(Font::DROID_SERIF_ITALIC)] = Font::DROID_SERIF_ITALIC;
        s_fontNameMap[_ID(Font::DROID_SERIF_BOLD)] = Font::DROID_SERIF_BOLD;
    }
}

size_t TextLabelStyle::getHash() const {
    if (_dirty) {
        size_t previousCache = Hashable::getHash();

        _hash = 17;
        Util::Hash_combine(_hash, _font);
        Util::Hash_combine(_hash, _fontSize);
        Util::Hash_combine(_hash, _width);
        Util::Hash_combine(_hash, _blurAmount);
        Util::Hash_combine(_hash, _spacing);
        Util::Hash_combine(_hash, _alignFlag);
        Util::Hash_combine(_hash, _colour.r);
        Util::Hash_combine(_hash, _colour.g);
        Util::Hash_combine(_hash, _colour.b);
        Util::Hash_combine(_hash, _colour.a);
        Util::Hash_combine(_hash, _bold);
        Util::Hash_combine(_hash, _italic);

        if (previousCache != _hash) {
            UniqueLockShared w_lock(s_textLableStyleMapMutex);
            hashAlg::insert(s_textLabelStyleMap, _hash, *this);
        }
        _dirty = false;
    }

    return Hashable::getHash();
}

const TextLabelStyle& TextLabelStyle::get(size_t textLabelStyleHash) {
    bool styleFound = false;
    const TextLabelStyle& style = get(textLabelStyleHash, styleFound);
    // Return the state block's descriptor
    return style;
}

const TextLabelStyle& TextLabelStyle::get(size_t textLabelStyleHash, bool& styleFound) {
    styleFound = false;

    SharedLock r_lock(s_textLableStyleMapMutex);
    // Find the render state block associated with the received hash value
    TextLabelStyleMap::const_iterator it = s_textLabelStyleMap.find(textLabelStyleHash);
    if (it != std::cend(s_textLabelStyleMap)) {
        styleFound = true;
        return it->second;
    }

    return s_textLabelStyleMap.find(s_defaultCacheValue)->second;
}

const stringImpl& TextLabelStyle::fontName(size_t fontNameHash) {
    return s_fontNameMap[fontNameHash];
}

}; //namespace Divide