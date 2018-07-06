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

#ifndef _FRAME_BUFFER_H_
#define _FRAME_BUFFER_H_

#include "Platform/Video/Headers/GraphicsResource.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

class Texture;
class NOINITVTABLE Framebuffer : protected GraphicsResource, public GUIDWrapper {
   public:
    struct FramebufferTarget {
        typedef std::array<bool, to_const_uint(TextureDescriptor::AttachmentType::COUNT)> FBOBufferMask;

        FBOBufferMask _drawMask;
        bool _clearColorBuffersOnBind;
        bool _clearDepthBufferOnBind;
        bool _changeViewport;

        FramebufferTarget()
            : _clearColorBuffersOnBind(true),
              _clearDepthBufferOnBind(true),
              _changeViewport(true)
        {
            _drawMask.fill(true);
        }
    };

    enum class FramebufferUsage : U32 {
        FB_READ_WRITE = 0,
        FB_READ_ONLY = 1,
        FB_WRITE_ONLY = 2
    };

    inline static FramebufferTarget& defaultPolicy() {
        static FramebufferTarget _defaultPolicy;
        return _defaultPolicy;
    }

    inline TextureDescriptor getDescriptor(TextureDescriptor::AttachmentType slot = TextureDescriptor::AttachmentType::Color0) {
        return _attachment[to_uint(slot)];
    }

    virtual Texture* getAttachment(TextureDescriptor::AttachmentType slot = TextureDescriptor::AttachmentType::Color0,
                                   bool flushStateOnRequest = true);

    void addAttachment(const TextureDescriptor& descriptor, TextureDescriptor::AttachmentType type);

    void addAttachment(Texture& texture, TextureDescriptor::AttachmentType type);

    /// If the FB is not initialized, it gets created, otherwise
    /// the attachements get resized.
    /// If any internal state was changed between calls (_shouldRebuild == true),
    /// the entire FB is recreated with the new state.
    virtual bool create(U16 width, U16 height) = 0;

    virtual void destroy() = 0;

    /// Use by multilayered FB's
    virtual void drawToLayer(TextureDescriptor::AttachmentType slot, U32 layer,
                             bool includeDepth = true) = 0;
    /// Used by cubemap FB's
    inline void drawToFace(TextureDescriptor::AttachmentType slot, U32 nFace,
                           bool includeDepth = true) {
        drawToLayer(slot, nFace, includeDepth);
    }

    virtual void setMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel,
                             TextureDescriptor::AttachmentType slot) = 0;

    virtual void resetMipLevel(TextureDescriptor::AttachmentType slot) = 0;

    void setMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel);

    void resetMipLevel();

    virtual void begin(const FramebufferTarget& drawPolicy) = 0;

    virtual void end() = 0;

    virtual void bind(U8 unit = 0,
                      TextureDescriptor::AttachmentType
                          slot = TextureDescriptor::AttachmentType::Color0,
                      bool flushStateOnRequest = true) = 0;

    virtual void readData(const vec4<U16>& rect, GFXImageFormat imageFormat,
                          GFXDataFormat dataType, void* outData) = 0;

    inline void readData(GFXImageFormat imageFormat, GFXDataFormat dataType,
                         void* outData) {
        readData(vec4<U16>(0, 0, _width, _height), imageFormat, dataType,
                 outData);
    }

    virtual void clear(const FramebufferTarget& drawPolicy) const = 0;

    virtual void blitFrom(Framebuffer* inputFB,
                          TextureDescriptor::AttachmentType
                              slot = TextureDescriptor::AttachmentType::Color0,
                          bool blitColor = true, bool blitDepth = false) = 0;

    // Enable/Disable the presence of a depth renderbuffer
    virtual void useAutoDepthBuffer(const bool state) {
        if (_useDepthBuffer != state) {
            _shouldRebuild = true;
            _useDepthBuffer = state;
        }
    }
    
    // Set the color the FB will clear to when drawing to it
    inline void setClearColor(const vec4<F32>& clearColor,
                              TextureDescriptor::AttachmentType
                                 slot = TextureDescriptor::AttachmentType::COUNT) {
        if (slot == TextureDescriptor::AttachmentType::COUNT) {
            _clearColors.fill(clearColor);
        } else {
            _clearColors[to_uint(slot)].set(clearColor);
        }
    }

    inline void setClearDepth(F32 depthValue) {
        _depthValue = depthValue;
    }

    inline bool isMultisampled() const {
        return _multisampled;
    }

    inline U16 getWidth()  const { return _width; }
    inline U16 getHeight() const { return _height; }
    inline U32 getHandle() const { return _framebufferHandle; }

    inline vec2<U16> getResolution() const {
        return vec2<U16>(_width, _height);
    }

    Framebuffer(GFXDevice& context, bool multiSample);
    virtual ~Framebuffer();

   protected:
    virtual bool checkStatus() const = 0;

   protected:
    
    bool _shouldRebuild;
    bool _useDepthBuffer;
    bool _multisampled;
    U16 _width, _height;
    U32 _framebufferHandle;
    F32 _depthValue;
    std::array<vec4<F32>, to_const_uint(TextureDescriptor::AttachmentType::COUNT)> _clearColors;
    std::array<TextureDescriptor, to_const_uint(TextureDescriptor::AttachmentType::COUNT)> _attachment;
    std::array<Texture*, to_const_uint(TextureDescriptor::AttachmentType::COUNT)> _attachmentTexture;
    std::array<bool, to_const_uint(TextureDescriptor::AttachmentType::COUNT)> _attachmentChanged;
};

};  // namespace Divide
#endif
