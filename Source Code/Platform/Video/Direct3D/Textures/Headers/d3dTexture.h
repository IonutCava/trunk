/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _D3D_TEXTURE_H_
#define _D3D_TEXTURE_H_

#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

class d3dTexture final : public Texture {
   public:
    d3dTexture(TextureType type);
    ~d3dTexture() {}

    bool generateHWResource(const stringImpl& name) override { return Texture::generateHWResource(name); }

    bool unload() override { return true; }

    void Bind(U8 unit, bool flushStateOnRequest = true) override {}
    
    void BindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write, bool flushStateOnRequest = true) override {};

    void setMipMapRange(U16 base = 0, U16 max = 1000) override {}

    void resize(const U8* const ptr,
                const vec2<U16>& dimensions,
                const vec2<U16>& mipLevels) override {}

    void updateMipMaps() override {}

    bool flushTextureState() override { return true; }

    void loadData(const TextureLoadInfo& info,
                  const U8* const ptr,
                  const vec2<U16>& dimensions,
                  const vec2<U16>& mipLevels,
                  GFXImageFormat format,
                  GFXImageFormat internalFormat) override {}

   private:
    U32 _type;
};

};  // namespace Divide
#endif