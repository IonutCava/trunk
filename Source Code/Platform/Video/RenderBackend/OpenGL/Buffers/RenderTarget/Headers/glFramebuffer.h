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

#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"
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
    const RTAttachment_ptr& getAttachmentPtr(RTAttachmentType type, U8 index) const override;
    RTAttachment& getAttachment(RTAttachmentType type, U8 index) override;

    void drawToLayer(const DrawLayerParams& params);

    void setMipLevel(U16 writeLevel, bool validate = true);

    void readData(const vec4<U16>& rect,
                  GFXImageFormat imageFormat,
                  GFXDataFormat dataType,
                  bufferPtr outData) override;

    void blitFrom(const RTBlitParams& params) override;

  protected:
    enum class AttachmentState : U8 {
        STATE_DISABLED = 0,
        STATE_ENABLED,
        COUNT
    };

    struct BindingState {
        AttachmentState _attState = AttachmentState::COUNT;
        U16 _writeLevel = 0;
        U16 _writeLayer = 0;
        bool _layeredRendering = false;

        inline bool operator==(const BindingState& other) const {
            return _attState == other._attState &&
                   _writeLevel == other._writeLevel &&
                   _writeLayer == other._writeLayer &&
                   _layeredRendering == other._layeredRendering;
        }

        inline bool operator!=(const BindingState& other) const {
            return _attState != other._attState ||
                   _writeLevel != other._writeLevel ||
                   _writeLayer != other._writeLayer ||
                   _layeredRendering != other._layeredRendering;
        }
    };

    /// Bake in all settings and attachments to prepare it for rendering
    bool create();
    void resolve(bool colours, bool depth);
    void queueCheckStatus();
    bool checkStatus();

    void setBlendState(const RTDrawDescriptor& drawPolicy, const vector<RTAttachment_ptr>& activeAttachments);
    void prepareBuffers(const RTDrawDescriptor& drawPolicy, const vector<RTAttachment_ptr>& activeAttachments);
    void clear(const RTDrawDescriptor& drawPolicy, const vector<RTAttachment_ptr>& activeAttachments) const;

    void initAttachment(RTAttachmentType type, U8 index);

    void toggleAttachment(const RTAttachment_ptr& attachment, AttachmentState state, bool layeredRendering);

    bool hasDepth() const;

    bool hasColour() const;

    void setAttachmentState(GLenum binding, BindingState state);
    BindingState getAttachmentState(GLenum binding) const;

    void setDefaultState(const RTDrawDescriptor& drawPolicy) override;

    void toggleAttachments();


    void fastBlit(GLuint inputFB,
                  GLuint outputFB,
                  const vec2<GLuint>& inputDim,
                  const vec2<GLuint>& outputDim,
                  GLenum colourAttIn,
                  GLenum colourAttOut,
                  ClearBufferMask mask);

   protected:
    void begin(const RTDrawDescriptor& drawPolicy);
    void end();
    void queueMipMapRecomputation();

   protected:
    bool _resolved;
    bool _isLayeredDepth;
    bool _statusCheckQueued;
    Rect<I32> _prevViewport;
    GLuint _framebufferHandle;
    static bool _zWriteEnabled;
    glFramebuffer* _resolveBuffer;
    RTDrawDescriptor _previousPolicy;

    bool _activeDepthBuffer;

    bool _hasMultisampledColourAttachments;

    hashMap<GLenum, BindingState> _attachmentState;
    hashMap<GLenum, std::set<U16, std::greater<U16>>> _attachmentDirtyLayers;
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