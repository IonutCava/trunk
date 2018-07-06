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

#ifndef _GL_FRAME_BUFFER_OBJECT_H
#define _GL_FRAME_BUFFER_OBJECT_H

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/Buffers/Framebuffer/Headers/Framebuffer.h"

namespace Divide {

class glFramebuffer : public Framebuffer {
   public:
    /// if resolveBuffer is not null, we add all of our attachments to it and
    /// initialize it with this buffer
    glFramebuffer(GFXDevice& context, bool useResolveBuffer = false);
    ~glFramebuffer();

    bool create(U16 width, U16 height);
    void destroy();

    Texture* getAttachment(TextureDescriptor::AttachmentType slot = TextureDescriptor::AttachmentType::Color0,
                           bool flushStateOnRequest = true);

    void drawToLayer(TextureDescriptor::AttachmentType slot, U32 layer,
                     bool includeDepth = true);
    void setMipLevel(U16 mipLevel, U16 mipMaxLevel, U16 writeLevel,
                     TextureDescriptor::AttachmentType slot);
    void resetMipLevel(TextureDescriptor::AttachmentType slot);
    void addDepthBuffer();
    void removeDepthBuffer();
    void begin(const FramebufferTarget& drawPolicy);
    void end();

    void bind(U8 unit = 0,
              TextureDescriptor::AttachmentType slot =
                         TextureDescriptor::AttachmentType::Color0,
              bool flushStateOnRequest = true);
    void readData(const vec4<U16>& rect, GFXImageFormat imageFormat,
                  GFXDataFormat dataType, void* outData);
    void blitFrom(Framebuffer* inputFB, TextureDescriptor::AttachmentType slot =
                                            TextureDescriptor::AttachmentType::Color0,
                  bool blitColor = true, bool blitDepth = false);

   protected:
    void resolve();
    void clear() const override;
    bool checkStatus() const;
    void setInitialAttachments();
    void initAttachment(TextureDescriptor::AttachmentType type,
                        const TextureDescriptor& texDescriptor,
                        bool resize);
    void resetMipMaps(FramebufferTarget::BufferMask mask);

    void toggleAttachment(TextureDescriptor::AttachmentType type, bool state);

   protected:
    bool _hasDepth;
    bool _hasColor;
    bool _resolved;
    bool _isCreated;
    bool _isLayeredDepth;
    
    static bool _viewportChanged;
    static bool _bufferBound;
    vectorImpl<GLenum> _colorBuffers;
    const std::unique_ptr<glFramebuffer> _resolveBuffer;

    using AttType = TextureDescriptor::AttachmentType;
    std::array<I32, to_const_uint(AttType::COUNT)> _attOffset;
    std::array<bool, to_const_uint(AttType::COUNT)> _attDirty;
    std::array<std::pair<GLenum, U32>, to_const_uint(AttType::COUNT)> _attachments;
    std::array<vec2<U16>, to_const_uint(AttType::COUNT)> _mipMapLevel;
    FramebufferTarget::BufferMask _previousMask;
};

};  // namespace Divide

#endif
