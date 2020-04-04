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

    friend class Attorney::GLAPIRenderTarget;

  public:
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

        inline bool operator==(const BindingState& other) const noexcept {
            return _attState == other._attState &&
                   _writeLevel == other._writeLevel &&
                   _writeLayer == other._writeLayer &&
                   _layeredRendering == other._layeredRendering;
        }

        inline bool operator!=(const BindingState& other) const noexcept {
            return _attState != other._attState ||
                   _writeLevel != other._writeLevel ||
                   _writeLayer != other._writeLayer ||
                   _layeredRendering != other._layeredRendering;
        }
    };

   public:
    explicit glFramebuffer(GFXDevice& context, const RenderTargetDescriptor& descriptor);
    ~glFramebuffer();

    bool resize(U16 width, U16 height) final;

    void drawToLayer(const DrawLayerParams& params);

    void setMipLevel(U16 writeLevel, bool validate = true);

    void readData(const vec4<U16>& rect,
                  GFXImageFormat imageFormat,
                  GFXDataFormat dataType,
                  bufferPtr outData) const final;

    void blitFrom(const RTBlitParams& params) final;

    /// Bake in all settings and attachments to prepare it for rendering
    bool create() final;

    BindingState getAttachmentState(GLenum binding) const;
    void toggleAttachment(const RTAttachment& attachment, AttachmentState state, bool layeredRendering);

protected:
    void queueCheckStatus();
    bool checkStatus();

    void setBlendState(const RTDrawDescriptor& drawPolicy, const RTAttachmentPool::PoolEntry& activeAttachments);
    void prepareBuffers(const RTDrawDescriptor& drawPolicy, const RTAttachmentPool::PoolEntry& activeAttachments);

    void initAttachment(RTAttachmentType type, U8 index);

    bool hasDepth() const;

    bool hasColour() const;

    void setAttachmentState(GLenum binding, BindingState state);

    void clear(const RTClearDescriptor& descriptor) final;
    void setDefaultState(const RTDrawDescriptor& drawPolicy) final;

    void toggleAttachments();

   protected:

    RTAttachment* getAttachmentInternal(RTAttachmentType type, U8 index);

    void clear(const RTClearDescriptor& drawPolicy, const RTAttachmentPool::PoolEntry& activeAttachments) const;
    void begin(const RTDrawDescriptor& drawPolicy);
    void end(bool needsUnbind);
    void queueMipMapRecomputation();
    void queueMipMapRecomputation(const RTAttachment& attachment, U16 startLayer, U16 layerCount);

   protected:
    RTDrawDescriptor _previousPolicy;
    std::array<GLenum, MAX_RT_COLOUR_ATTACHMENTS> _activeColourBuffers;
    GLenum _activeReadBuffer;

    hashMap<GLenum, BindingState> _attachmentState;
    hashMap<GLenum, eastl::set<U16, eastl::greater<U16>>> _attachmentResolvedLayers;

    Rect<I32> _prevViewport;
    Str128 _debugMessage;
    glFramebuffer* _parent;
    GLuint _framebufferHandle;

    bool _isLayeredDepth;
    bool _statusCheckQueued;
    bool _activeDepthBuffer;
    static bool _zWriteEnabled;
};

namespace Attorney {
    class GLAPIRenderTarget {
        private:
        static void begin(glFramebuffer& buffer, const RTDrawDescriptor& drawPolicy) {
            buffer.begin(drawPolicy);
        }
        static void end(glFramebuffer& buffer, bool needsUnbind) {
            buffer.end(needsUnbind);
        }

        friend class Divide::GL_API;
    };
};  // namespace Attorney
};  // namespace Divide

#endif