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

#ifndef _RENDER_TARGET_H_
#define _RENDER_TARGET_H_

#include "RTDrawDescriptor.h"
#include "Platform/Video/Headers/GraphicsResource.h"

namespace Divide {

struct RenderTargetID {
    RenderTargetID() : RenderTargetID(RenderTargetUsage::COUNT)
    {
    }

    RenderTargetID(RenderTargetUsage usage) : RenderTargetID(usage, 0)
    {
    }

    RenderTargetID(RenderTargetUsage usage, U32 index)
        : _index(index),
        _usage(usage)
    {
    }

    U32 _index = 0;
    RenderTargetUsage _usage = RenderTargetUsage::COUNT;
};

class RenderTarget;
struct RenderTargetHandle {
    RenderTargetHandle()
        : RenderTargetHandle(RenderTargetID(), nullptr)
    {
    }

    RenderTargetHandle(RenderTargetID targetID, RenderTarget* rt)
        : _rt(rt),
        _targetID(targetID)
    {
    }

    RenderTarget* _rt;
    RenderTargetID _targetID;
};

class NOINITVTABLE RenderTarget : protected GraphicsResource, public GUIDWrapper {
   public:
    enum class RenderTargetUsage : U32 {
        RT_READ_WRITE = 0,
        RT_READ_ONLY = 1,
        RT_WRITE_ONLY = 2
    };

   protected:
    RenderTarget(GFXDevice& context, const stringImpl& name);

   public:
    virtual ~RenderTarget();

    static RTDrawDescriptor& defaultPolicy();
    static RTDrawDescriptor& defaultPolicyKeepDepth();
    static RTDrawDescriptor& defaultPolicyDepthOnly();

    /// If the FB is not initialized, it gets created, otherwise
    /// the attachements get resized.
    virtual bool create(U16 width, U16 height) = 0;
    virtual const RTAttachment& getAttachment(RTAttachment::Type type, U8 index, bool flushStateOnRequest = true);
    virtual const RTAttachment& getAttachment(RTAttachment::Type type, U8 index) const;
    virtual void destroy() = 0;
    /// Use by multilayered FB's
    virtual void drawToLayer(RTAttachment::Type type, U8 index, U32 layer, bool includeDepth = true) = 0;
    // This call also limits the min and max mip levels of the attachment
    virtual void setMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel, RTAttachment::Type type, U8 index) = 0;
    // This call sets the target mip level to write to
    virtual void setMipLevel(U16 writeLevel, RTAttachment::Type type, U8 index) = 0;
    virtual void resetMipLevel(RTAttachment::Type type, U8 index) = 0;
    virtual void begin(const RTDrawDescriptor& drawPolicy) = 0;
    virtual void end() = 0;
    virtual void bind(U8 unit, RTAttachment::Type type, U8 index, bool flushStateOnRequest = true) = 0;
    virtual void readData(const vec4<U16>& rect, GFXImageFormat imageFormat, GFXDataFormat dataType, bufferPtr outData) = 0;
    virtual void clear(const RTDrawDescriptor& drawPolicy) const = 0;
    virtual void blitFrom(RenderTarget* inputFB, bool blitColour = true, bool blitDepth = false) = 0;
    virtual void blitFrom(RenderTarget* inputFB, U8 index, bool blitColour = true, bool blitDepth = false) = 0;

    TextureDescriptor& getDescriptor(RTAttachment::Type type, U8 index);
    void addAttachment(const TextureDescriptor& descriptor, RTAttachment::Type type, U8 index);
    bool create(U16 widthAndHeight);
    /// Used by cubemap FB's
    void drawToFace(RTAttachment::Type type, U8 index, U32 nFace, bool includeDepth = true);
    void setMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel);
    void setMipLevel(U16 writeLevel);

    void resetMipLevel();
    void readData(GFXImageFormat imageFormat, GFXDataFormat dataType, bufferPtr outData);
    // Set the colour the FB will clear to when drawing to it
    void setClearColour(RTAttachment::Type type, U8 index, const vec4<F32>& clearColour);
    void setClearDepth(F32 depthValue);

    U16 getWidth()  const;
    U16 getHeight() const;

   protected:
    virtual bool checkStatus() const = 0;

   protected:
    static U8 g_maxColourAttachments;

    U16 _width, _height;
    F32 _depthValue;
    stringImpl _name;
    RTAttachmentPool _attachments;
};

};  // namespace Divide

#endif //_RENDER_TARGET_H_
