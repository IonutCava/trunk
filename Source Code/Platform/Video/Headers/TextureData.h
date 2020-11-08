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
#ifndef _TEXTURE_DATA_H_
#define _TEXTURE_DATA_H_

#include "RenderAPIEnums.h"

namespace Divide {

struct CopyTexParams {
    U8 _sourceMipLevel = 0;
    U8 _targetMipLevel = 0;
    vec3<U32> _sourceCoords;
    vec3<U32> _targetCoords;
    vec3<U16> _dimensions; //width, height, numlayers
};

struct TextureData {
    U32 _textureHandle = 0u;
    TextureType _textureType = TextureType::COUNT;
};

FORCE_INLINE bool IsValid(const TextureData& data) noexcept {
    return data._textureHandle != 0u && data._textureType != TextureType::COUNT;
}

FORCE_INLINE bool operator==(const TextureData& lhs, const TextureData& rhs) noexcept {
    return lhs._textureHandle == rhs._textureHandle &&
           lhs._textureType == rhs._textureType;
}

FORCE_INLINE bool operator!=(const TextureData& lhs, const TextureData& rhs) noexcept {
    return lhs._textureHandle != rhs._textureHandle ||
           lhs._textureType != rhs._textureType;
}

inline size_t GetHash(const TextureData& data) noexcept {
    size_t ret = 11;
    Util::Hash_combine(ret, data._textureHandle);
    Util::Hash_combine(ret, to_base(data._textureType));
    return ret;
}

enum class TextureUpdateState : U8 {
    ADDED = 0,
    REPLACED,
    NOTHING,
    COUNT
};

using TextureEntry = std::tuple<U8/*binding*/, TextureData, size_t/*sampler*/>;

template<size_t Size = to_size(TextureUsage::COUNT)>
struct TextureDataContainer {
    static constexpr U8 INVALID_BINDING = std::numeric_limits<U8>::max();
    static constexpr TextureEntry DefaultEntry = { INVALID_BINDING, {}, 0 };
    using DataEntries = std::array<TextureEntry, Size>;
    static constexpr size_t ContainerSize() noexcept { return Size; }

    bool set(const TextureDataContainer& other);
    TextureUpdateState setTextures(const TextureDataContainer& textureEntries);
    TextureUpdateState setTextures(const DataEntries& textureEntries);
    TextureUpdateState setTexture(const TextureData& data, size_t samplerHash, U8 binding);
    TextureUpdateState setTexture(const TextureData& data, size_t samplerHash, TextureUsage binding);

    bool removeTexture(U8 binding);
    bool removeTexture(const TextureData& data);
    void clear();

    PROPERTY_RW(DataEntries, textures, create_array<Size>(DefaultEntry));
    PROPERTY_RW(U8, count, 0u);

protected:
    TextureUpdateState setTextureInternal(const TextureData& data, size_t samplerHash, U8 binding);

XALLOCATOR
};

template<size_t Size>
bool operator==(const TextureDataContainer<Size> & lhs, const TextureDataContainer<Size> & rhs) noexcept;

template<size_t Size>
bool operator!=(const TextureDataContainer<Size> & lhs, const TextureDataContainer<Size> & rhs) noexcept;
}; //namespace Divide

#endif //_TEXTURE_DATA_H_

#include "TextureData.inl"
