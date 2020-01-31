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
#include "Core/Headers/Hashable.h"
#include "Core/Math/Headers/MathVectors.h"

namespace Divide {

struct CopyTexParams {
    U8 _sourceMipLevel = 0;
    U8 _targetMipLevel = 0;
    vec3<U32> _sourceCoords;
    vec3<U32> _targetCoords;
    vec3<U16> _dimensions; //width, height, numlayers
};

struct TextureData {
    TextureData() noexcept : TextureData(0u, 0u, TextureType::COUNT)
    {}

    TextureData(U32 texHandle, U32 samplerHandle, TextureType type) noexcept
        : _textureHandle(texHandle), _samplerHandle(samplerHandle), _textureType(type)
    {}

    inline U32 textureHandle() const noexcept { return _textureHandle; }
    inline U32 samplerHandle() const noexcept { return _samplerHandle; }
    inline TextureType  type() const noexcept { return _textureType;   }

    FORCE_INLINE bool operator==(const TextureData& other) const noexcept {
        return _textureHandle == other._textureHandle &&
               _samplerHandle == other._samplerHandle &&
               _textureType == other._textureType;
    }

    FORCE_INLINE bool operator!=(const TextureData& other) const noexcept {
        return _textureHandle != other._textureHandle ||
               _samplerHandle != other._samplerHandle ||
               _textureType != other._textureType;
    }

private:
    friend class Texture;
    U32 _textureHandle = 0u;
    U32 _samplerHandle = 0u;
    TextureType _textureType = TextureType::COUNT;
};

class TextureDataContainer {
    public:
      enum class UpdateState : U8 {
          ADDED = 0,
          REPLACED,
          NOTHING,
          COUNT
      };

      TextureDataContainer();
      ~TextureDataContainer();

      using DataEntries = eastl::vector_map<U8/*binding*/, TextureData, eastl::less<U8>, eastl::dvd_eastl_allocator>;

      bool set(const TextureDataContainer& other);

      UpdateState setTextures(const TextureDataContainer& textureEntries, bool force = false);
      UpdateState setTextures(const DataEntries& textureEntries, bool force = false);
      UpdateState setTexture(const TextureData& data, U8 binding, bool force = false);

      bool removeTexture(U8 binding);
      bool removeTexture(const TextureData& data);

      inline void clear() { _textures.clear(); }

      inline bool empty() const noexcept { return _textures.empty(); }

      inline DataEntries& textures() noexcept { return _textures; }
      inline const DataEntries& textures() const noexcept { return _textures; }

      inline bool operator==(const TextureDataContainer &other) const { return _textures == other._textures; }
      inline bool operator!=(const TextureDataContainer &other) const { return _textures != other._textures; }

    protected:
        inline UpdateState setTextureInternal(const TextureData& data, U8 binding, bool force) {
            OPTICK_EVENT();

            const auto& result = _textures.emplace(binding, data);
            if (result.second) {
                return UpdateState::ADDED;
            }

            if (result.first->second != data || force) {
                result.first->second = data;
                return UpdateState::REPLACED;
            }

            return UpdateState::NOTHING;
        }

    private:
        DataEntries _textures;

    XALLOCATOR
};

}; //namespace Divide

#endif //_TEXTURE_DATA_H_
