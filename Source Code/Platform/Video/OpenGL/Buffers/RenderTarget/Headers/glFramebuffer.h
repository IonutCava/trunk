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

#ifndef _GL_FRAME_BUFFER_OBJECT_H_
#define _GL_FRAME_BUFFER_OBJECT_H_

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RTAttachment.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

class glFramebuffer : public RenderTarget {
    USE_CUSTOM_ALLOCATOR
   public:
    /// if resolveBuffer is not null, we add all of our attachments to it and
    /// initialize it with this buffer
    glFramebuffer(GFXDevice& context, const stringImpl& name);
    ~glFramebuffer();

    void copy(const RenderTarget& other) override;

    bool create(U16 width, U16 height) override;

    const RTAttachment& getAttachment(RTAttachment::Type type,
                                      U8 index,
                                      bool flushStateOnRequest = true) override;

    const RTAttachment& getPrevFrameAttachment(RTAttachment::Type type,
                                               U8 index) const override;

    void drawToLayer(RTAttachment::Type type,
                     U8 index,
                     U16 layer,
                     bool includeDepth = true) override;

    void setMipLevel(U16 writeLevel) override;

    void begin(const RTDrawDescriptor& drawPolicy)  override;
    void end()  override;

    void bind(U8 unit,
              RTAttachment::Type type,
              U8 index,
              bool flushStateOnRequest = true) override;

    void readData(const vec4<U16>& rect,
                  GFXImageFormat imageFormat,
                  GFXDataFormat dataType,
                  bufferPtr outData) override;

    void blitFrom(RenderTarget* inputFB,
                  bool blitColour = true,
                  bool blitDepth = false) override;

    void blitFrom(RenderTarget* inputFB,
                  U8 index,
                  bool blitColour = true,
                  bool blitDepth = false) override;

  protected:
    enum class AttachmentState : U8 {
        STATE_DISABLED = 0,
        STATE_ENABLED,
        STATE_LAYERED,
        COUNT
    };

    void resolve();
    void clear(const RTDrawDescriptor& drawPolicy) const override;
    bool checkStatus() const;
    void resetAttachments();
    void prepareBuffers(const RTDrawDescriptor& drawPolicy);

    void initAttachment(RTAttachment::Type type, U8 index);
    void initAttachment(const RTAttachment_ptr& attachment, RTAttachment::Type type, U8 index, U8 copyCount);

    void updateDescriptor(RTAttachment::Type type, U8 index);

    void toggleAttachment(const RTAttachment_ptr& attachment, AttachmentState state);

    bool hasDepth() const;

    bool hasColour() const;

    void setAttachmentState(GLenum binding, AttachmentState state);
    AttachmentState getAttachmentState(GLenum binding) const;

   protected:
    bool _resolved;
    bool _isLayeredDepth;
    GLuint _framebufferHandle;
    static bool _bufferBound;
    static bool _zWriteEnabled;
    glFramebuffer* _resolveBuffer;
    RTDrawDescriptor _previousPolicy;

    bool _activeDepthBuffer;
    vectorImpl<GLenum> _activeColourBuffers;

    vectorImpl<RTAttachment_ptr> _activeAttachmentsCache;

    hashMapImpl<GLenum, AttachmentState> _attachmentState;
};

};  // namespace Divide

#endif