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

#ifndef _D3D_FRAME_BUFFER_OBJECT_H_
#define _D3D_FRAME_BUFFER_OBJECT_H_

#include "Platform/Video/Buffers/Framebuffer/Headers/Framebuffer.h"

namespace Divide {
    
class d3dRenderTarget : public Framebuffer {
    DECLARE_ALLOCATOR
   public:
    d3dRenderTarget(GFXDevice& context, bool multisampled);
    ~d3dRenderTarget();

    bool create(U16 width, U16 height);

    void destroy();
    void drawToLayer(TextureDescriptor::AttachmentType slot, U32 layer,
                     bool includeDepth = true);
    void setMipLevel(U16 mipLevel, U16 mipMaxLevel, U16 writeLevel,
                     TextureDescriptor::AttachmentType slot);
    void resetMipLevel(TextureDescriptor::AttachmentType slot);
    void begin(const FramebufferTarget& drawPolicy);
    void end();

    void bind(U8 unit = 0,
              TextureDescriptor::AttachmentType slot =
                TextureDescriptor::AttachmentType::Color0,
              bool flushStateOnRequest = true);
    void readData(const vec4<U16>& rect, GFXImageFormat imageFormat,
                  GFXDataFormat dataType, void* outData);
    void blitFrom(Framebuffer* inputFB,
                  TextureDescriptor::AttachmentType slot =
                      TextureDescriptor::AttachmentType::Color0,
                  bool blitColor = true, bool blitDepth = false);
    void clear(const FramebufferTarget& drawPolicy) const override;

   protected:
    bool checkStatus() const;
};

};  // namespace Divide
#endif