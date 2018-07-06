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

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {
class glLockManager;
class glTexture final : public Texture,
                        public glObject {
   public:
    explicit glTexture(GFXDevice& context,
                       size_t descriptorHash,
                       const stringImpl& name,
                       const stringImpl& resourceName,
                       const stringImpl& resourceLocation,
                       bool isFlipped,
                       bool asyncLoad,
                       const TextureDescriptor& texDescriptor);
    ~glTexture();

    bool resourceLoadComplete() override;

    bool unload() override;

    void bindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write) override;

    void setMipMapRange(U16 base = 0, U16 max = 1000) override;

    void resize(const bufferPtr ptr,
                const vec2<U16>& dimensions) override;

    void loadData(const TextureLoadInfo& info,
                  const vectorImpl<ImageTools::ImageLayer>& imageLayers) override;

    void loadData(const TextureLoadInfo& info,
                  const bufferPtr data,
                  const vec2<U16>& dimensions) override;

    void copy(const Texture_ptr& other) override;

    void setCurrentSampler(const SamplerDescriptor& descriptor) override;


    void refreshMipMaps(bool immediate) override;

   protected:
    void threadedLoad(DELEGATE_CBK<void, CachedResource_wptr> onLoadCallback) override;
    void reserveStorage();

    void loadDataCompressed(const TextureLoadInfo& info,
                            const vectorImpl<ImageTools::ImageLayer>& imageLayers);

    void loadDataUncompressed(const TextureLoadInfo& info,
                              bufferPtr data);

    void setMipRangeInternal(U16 base, U16 max);

   private:
    GLenum _type;
    std::atomic_bool _allocatedStorage;
    glLockManager* _lockManager;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(glTexture);

};  // namespace Divide

#endif