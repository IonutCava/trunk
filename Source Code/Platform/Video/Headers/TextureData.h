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

#ifndef _TEXTURE_DATA_H_
#define _TEXTURE_DATA_H_

#include "RenderAPIEnums.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {
class TextureData {
public:
    TextureData() : TextureData(TextureType::TEXTURE_2D, 0, 0)
    {
    }

    TextureData(TextureType type, U32 handle, U8 slot)
        : _textureType(type),
        _samplerHash(0),
        _textureHandle(0)
    {
        setHandleHigh(handle);
        setHandleLow(to_uint(slot));
    }

    TextureData(const TextureData& other)
        : _textureType(other._textureType),
        _samplerHash(other._samplerHash),
        _textureHandle(other._textureHandle)
    {
    }

    TextureData& operator=(const TextureData& other) {
        _textureType = other._textureType;
        _samplerHash = other._samplerHash;
        _textureHandle = other._textureHandle;

        return *this;
    }

    inline void set(const TextureData& other) {
        _textureHandle = other._textureHandle;
        _textureType = other._textureType;
        _samplerHash = other._samplerHash;
    }

    inline void setHandleHigh(U32 handle) {
        _textureHandle = (U64)handle << 32 | getHandleLow();
    }

    inline U32 getHandleHigh() const {
        return to_uint(_textureHandle >> 32);
    }

    inline void getHandleHigh(U32& handle) const {
        handle = getHandleHigh();
    }

    inline void setHandleLow(U32 handle) {
        _textureHandle |= handle;
    }

    inline U32 getHandleLow() const {
        return to_uint(_textureHandle);
    }

    inline void getHandleLow(U32& handle) const {
        handle = getHandleLow();
    }

    inline void setHandle(U64 handle) {
        _textureHandle = handle;
    }

    inline void getHandle(U64& handle) const {
        handle = _textureHandle;
    }

    // No need to cache this as it should already be pretty fast
    size_t getHash() const;

    TextureType _textureType;
    size_t _samplerHash;
private:
    // High: texture handle  Low: slot
    U64  _textureHandle;
};

class TextureDataContainer {
    public:
      TextureDataContainer();
      ~TextureDataContainer();

      void set(const TextureDataContainer& other);
      bool addTexture(const TextureData& data);
      bool removeTexture(const TextureData& data);

      vectorImpl<TextureData>& textures();
      const vectorImpl<TextureData>& textures() const;
      void clear(bool clearMemory);

    private:
      vectorImpl<TextureData> _textures;
};

}; //namespace Divide

#endif //_TEXTURE_DATA_H_
