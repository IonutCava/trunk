/*
Copyright (c) 2017 DIVIDE-Studio
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
#include "Core/TemplateLibraries/Headers/Vector.h"

namespace Divide {
class TextureData {
public:
    TextureData() : TextureData(TextureType::TEXTURE_2D, 0u)
    {
    }

    TextureData(TextureType type, U32 handle)
      : _textureType(type),
        _samplerHash(0),
        _textureHandle(0)
    {
        setHandle(handle);
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

    /// ID
    inline U32 getHandle() const {
        return _textureHandle;
    }

    /// ID
    inline void getHandle(U32& handle) const {
        handle = getHandle();
    }

    inline void setHandle(U32 handle) {
        _textureHandle = handle;
    }

    // No need to cache this as it should already be pretty fast
    size_t getHash() const;

    TextureType _textureType;
    size_t _samplerHash;
private:
    // Texture handle
    U32  _textureHandle;
};

class TextureDataContainer {
    public:
      TextureDataContainer();
      ~TextureDataContainer();

      void set(const TextureDataContainer& other);
      bool addTexture(const TextureData& data, U8 binding);
      bool addTexture(const std::pair<TextureData, U8 /*binding*/>& textureEntry);
      bool removeTexture(U8 binding);
      bool removeTexture(const TextureData& data);

      vectorImpl<std::pair<TextureData, U8>>& textures();
      const vectorImpl<std::pair<TextureData, U8>>& textures() const;
      void clear(bool clearMemory = false);

    private:
      vectorImpl<std::pair<TextureData, U8 /*binding*/>> _textures;
};

}; //namespace Divide

#endif //_TEXTURE_DATA_H_
