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
#include "Core/TemplateLibraries/Headers/Vector.h"

#include <MemoryPool/StackAlloc.h>
#include <MemoryPool/C-11/MemoryPool.h>

namespace Divide {
class TextureData {
public:
    TextureData(TextureType type, U32 handle)
      : _textureType(type),
        _textureHandle(handle),
        _samplerHandle(0u)
    {
    }

    TextureData() noexcept = default;
    TextureData(const TextureData& other) = default;
    TextureData& operator=(const TextureData& other) = default;
    TextureData(TextureData&& other) = default;
    TextureData & operator=(TextureData&& other) = default;

    inline U32  getHandle()     const { return _textureHandle; }
    inline void setHandle(U32 handle) { _textureHandle = handle; }

    inline const TextureType& type() const { return _textureType; }

    inline bool operator==(const TextureData& other) const {
        return _textureType == other._textureType &&
               _textureHandle == other._textureHandle &&
               _samplerHandle == other._samplerHandle;
    }

    inline bool operator!=(const TextureData& other) const {
        return _textureType != other._textureType ||
               _textureHandle != other._textureHandle ||
               _samplerHandle != other._samplerHandle;
    }
    
    U32 _samplerHandle = 0u;
    U32 _textureHandle = 0u;
    TextureType _textureType = TextureType::COUNT;
};

class TextureDataContainer {
    public:
      TextureDataContainer() noexcept;
      ~TextureDataContainer();

      bool set(const TextureDataContainer& other);
      bool addTexture(const TextureData& data, U8 binding);
      bool addTexture(const eastl::pair<TextureData, U8 /*binding*/>& textureEntry);
      bool addTextures(const TextureDataContainer& textureEntries);
      bool addTextures(const vectorEASTL<eastl::pair<TextureData, U8 /*binding*/>>& textureEntries);

      bool removeTexture(U8 binding);
      bool removeTexture(const TextureData& data);

      vectorEASTL<eastl::pair<TextureData, U8>>& textures();
      const vectorEASTL<eastl::pair<TextureData, U8>>& textures() const;
      void clear(bool clearMemory = false);

      FORCE_INLINE bool operator==(const TextureDataContainer &other) const {
          return  _textures == other._textures;
      }

      FORCE_INLINE bool operator!=(const TextureDataContainer &other) const {
          return _textures != other._textures;
      }

    private:
      vectorEASTL<eastl::pair<TextureData, U8 /*binding*/>> _textures;
};

}; //namespace Divide

#endif //_TEXTURE_DATA_H_
