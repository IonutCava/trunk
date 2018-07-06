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

#include "Utility/Headers/GUIDWrapper.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

class Texture;
class NOINITVTABLE Framebuffer : private NonCopyable, public GUIDWrapper {
   public:
    struct FramebufferTarget {
        enum class BufferMask : U32 {
            COLOR = 0,
            DEPTH = 1,
            BOTH = 2,
            COUNT
        };

        BufferMask _drawMask;
        U32 _numColorChannels;
        bool _clearBuffersOnBind;
        bool _changeViewport;

        FramebufferTarget()
            : _drawMask(BufferMask::BOTH),
              _numColorChannels(1),
              _clearBuffersOnBind(true),
              _changeViewport(true)
        {
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

    virtual Texture* GetAttachment(TextureDescriptor::AttachmentType slot = TextureDescriptor::AttachmentType::Color0,
                                   bool flushStateOnRequest = true);

    virtual bool AddAttachment(const TextureDescriptor& descriptor,
                               TextureDescriptor::AttachmentType slot);

    /// If the FB is not initialized, it gets created, otherwise
    /// the attachements get resized.
    /// If any internal state was changed between calls (_shouldRebuild == true),
    /// the entire FB is recreated with the new state.
    virtual bool Create(U16 width, U16 height) = 0;

    virtual void Destroy() = 0;

    /// Use by multilayered FB's
    virtual void DrawToLayer(TextureDescriptor::AttachmentType slot, U8 layer,
                             bool includeDepth = true) = 0;
    /// Used by cubemap FB's
    inline void DrawToFace(TextureDescriptor::AttachmentType slot, U8 nFace,
                           bool includeDepth = true) {
        DrawToLayer(slot, nFace, includeDepth);
    }

    virtual void SetMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel,
                             TextureDescriptor::AttachmentType slot) = 0;

    virtual void ResetMipLevel(TextureDescriptor::AttachmentType slot) = 0;

    virtual void Begin(const FramebufferTarget& drawPolicy) = 0;

    virtual void End() = 0;

    virtual void Bind(U8 unit = 0,
                      TextureDescriptor::AttachmentType
                          slot = TextureDescriptor::AttachmentType::Color0,
                      bool flushStateOnRequest = true) = 0;

    virtual void ReadData(const vec4<U16>& rect, GFXImageFormat imageFormat,
                          GFXDataFormat dataType, void* outData) = 0;

    inline void ReadData(GFXImageFormat imageFormat, GFXDataFormat dataType,
                         void* outData) {
        ReadData(vec4<U16>(0, 0, _width, _height), imageFormat, dataType,
                 outData);
    }

    virtual void BlitFrom(Framebuffer* inputFB,
                          TextureDescriptor::AttachmentType
                              slot = TextureDescriptor::AttachmentType::Color0,
                          bool blitColor = true, bool blitDepth = false) = 0;
    // If true, array texture and/or cubemaps are bound to a single attachment
    // and shader based layered rendering should be used
    virtual void toggleLayeredRendering(const bool state) {
        _shouldRebuild = (_layeredRendering != state);
        _layeredRendering = state;
    }
    // Enable/Disable color writes
    virtual void toggleColorWrites(const bool state) {
        _shouldRebuild = (_disableColorWrites == state);
        _disableColorWrites = !state;
    }
    // Enable/Disable the presence of a depth renderbuffer
    virtual void toggleDepthBuffer(const bool state) {
        _shouldRebuild = (_useDepthBuffer != state);
        _useDepthBuffer = state;
    }
    // Set the color the FB will clear to when drawing to it
    inline void setClearColor(const vec4<F32>& clearColor) {
        _clearColor.set(clearColor);
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

    inline void clearBuffers(bool state) {
        _clearBuffersState = state;
    }

    inline bool clearBuffers() const {
        return _clearBuffersState;
    }

    Framebuffer(bool multiSample);
    virtual ~Framebuffer();

   protected:
    virtual bool checkStatus() const = 0;

   protected:
    bool _shouldRebuild;
    bool _layeredRendering;
    bool _clearBuffersState;
    bool _useDepthBuffer;
    bool _disableColorWrites;
    bool _multisampled;
    U16 _width, _height;
    U32 _framebufferHandle;
    vec4<F32> _clearColor;
    std::array<TextureDescriptor,
               to_const_uint(TextureDescriptor::AttachmentType::COUNT)>
        _attachment;
    std::array<Texture*,
               to_const_uint(TextureDescriptor::AttachmentType::COUNT)>
        _attachmentTexture;
    std::array<bool, to_const_uint(TextureDescriptor::AttachmentType::COUNT)>
        _attachmentDirty;
};

};  // namespace Divide
#endif
