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
#ifndef _GL_TEXTURE_H_
#define _GL_TEXTURE_H_

#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {
class glLockManager;
class glTexture final : public Texture,
                        public glObject {
   public:
    explicit glTexture(GFXDevice& context,
                       size_t descriptorHash,
                       const Str256& name,
                       const ResourcePath& resourceName,
                       const ResourcePath& resourceLocation,
                       bool isFlipped,
                       bool asyncLoad,
                       const TextureDescriptor& texDescriptor);
    ~glTexture();

    bool unload() final;

    void bindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write) final;

    void setMipMapRange(U16 base = 0, U16 max = 1000) noexcept final;

    void resize(const std::pair<Byte*, size_t>& ptr, const vec2<U16>& dimensions) final;

    void loadData(const ImageTools::ImageData& imageData) final;

    void loadData(const std::pair<Byte*, size_t>& data, const vec2<U16>& dimensions) final;

    static void copy(const TextureData& source, const TextureData& destination, const CopyTexParams& params);

   protected:
    void threadedLoad() final;
    void reserveStorage() const;

    void processTextureType() noexcept;
    void validateDescriptor() final;
    void loadDataCompressed(const ImageTools::ImageData& imageData);

    void loadDataUncompressed(const ImageTools::ImageData& imageData) const;

    void setMipRangeInternal(U16 base, U16 max) const noexcept;

   private:
    GLenum _type;
    std::atomic_bool _allocatedStorage;
    TextureData _loadingData;
    glLockManager* _lockManager;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(glTexture);

};  // namespace Divide

#endif