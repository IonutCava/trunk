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
#ifndef _GL_FRAME_BUFFER_OBJECT_H_
#define _GL_FRAME_BUFFER_OBJECT_H_

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RTAttachment.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

class GL_API;

namespace Attorney {
    class GLAPIRenderTarget;
};

class glFramebuffer : public RenderTarget,
                      public glObject {

    USE_CUSTOM_ALLOCATOR
    friend class Attorney::GLAPIRenderTarget;
   public:
    /// if resolveBuffer is not null, we add all of our attachments to it and
    /// initialize it with this buffer
    explicit glFramebuffer(GFXDevice& context, const RenderTargetDescriptor& descriptor);
    ~glFramebuffer();

    bool resize(U16 width, U16 height) override;

    const RTAttachment& getAttachment(RTAttachmentType type, U8 index) const override;

    void drawToLayer(RTAttachmentType type,
                     U8 index,
                     U16 layer,
                     bool includeDepth = true) override;

    void setMipLevel(U16 writeLevel) override;

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
        COUNT
    };

    struct BindingState {
        AttachmentState _attState = AttachmentState::COUNT;
        GLint _writeLevel = -1;
        GLint _writeLayer = -1;

        inline bool operator==(const BindingState& other) const {
            return _attState == other._attState &&
                   _writeLevel == other._writeLevel &&
                   _writeLayer == other._writeLayer;
        }

        inline bool operator!=(const BindingState& other) const {
            return _attState != other._attState ||
                   _writeLevel != other._writeLevel ||
                   _writeLayer != other._writeLayer;
        }
    };

    /// Bake in all settings and attachments to prepare it for rendering
    bool create();
    void resolve();
    bool checkStatus() const;
    
    void setBlendState(const RTDrawDescriptor& drawPolicy, const vector<RTAttachment_ptr>& activeAttachments);
    void prepareBuffers(const RTDrawDescriptor& drawPolicy, const vector<RTAttachment_ptr>& activeAttachments);
    void clear(const RTDrawDescriptor& drawPolicy, const vector<RTAttachment_ptr>& activeAttachments) const;

    void initAttachment(RTAttachmentType type, U8 index);

    void toggleAttachment(const RTAttachment_ptr& attachment, AttachmentState state);

    bool hasDepth() const;

    bool hasColour() const;

    void setAttachmentState(GLenum binding, BindingState state);
    BindingState getAttachmentState(GLenum binding) const;

    void setDefaultState(const RTDrawDescriptor& drawPolicy);

    void toggleAttachments(const RTDrawDescriptor& drawPolicy);

   protected:
    void begin(const RTDrawDescriptor& drawPolicy);
    void end();

   protected:
    bool _resolved;
    bool _isLayeredDepth;
    Rect<I32> _prevViewport;
    GLuint _framebufferHandle;
    static bool _bufferBound;
    static bool _zWriteEnabled;
    glFramebuffer* _resolveBuffer;
    RTDrawDescriptor _previousPolicy;

    bool _activeDepthBuffer;

    bool _hasMultisampledColourAttachments;

    hashMap<GLenum, BindingState> _attachmentState;
};

namespace Attorney {
    class GLAPIRenderTarget {
        private:
        static void begin(glFramebuffer& buffer, const RTDrawDescriptor& drawPolicy) {
            buffer.begin(drawPolicy);
        }
        static void end(glFramebuffer& buffer) {
            buffer.end();
        }
        friend class Divide::GL_API;
    };
};  // namespace Attorney
};  // namespace Divide

#endif