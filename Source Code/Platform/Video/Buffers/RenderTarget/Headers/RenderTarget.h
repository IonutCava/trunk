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
#ifndef _RENDER_TARGET_H_
#define _RENDER_TARGET_H_

#include "RTDrawDescriptor.h"
#include "RTAttachmentPool.h"
#include "Platform/Video/Headers/GraphicsResource.h"

namespace Divide {
    constexpr U32 RT_MAX_ATTACHMENTS = 32;

class GFXRTPool;

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

struct RenderTargetDescriptor {
    U8 _attachmentCount = 0;
    stringImpl _name = "";
    vec2<U16>  _resolution = vec2<U16>(1u);
    RTAttachmentDescriptor* _attachments = nullptr;

    U8 _externalAttachmentCount = 0;
    ExternalRTAttachmentDescriptor* _externalAttachments = nullptr;

    F32 _depthValue = 1.0f;
    vec2<F32> _depthRange = vec2<F32>(0.0f, 1.0f);
};

class NOINITVTABLE RenderTarget : public GUIDWrapper, public GraphicsResource {
   public:
    enum class RenderTargetUsage : U8 {
        RT_READ_WRITE = 0,
        RT_READ_ONLY = 1,
        RT_WRITE_ONLY = 2
    };

   protected:
    explicit RenderTarget(GFXDevice& context, const RenderTargetDescriptor& descriptor);

   public:
    virtual ~RenderTarget();

    static RTDrawDescriptor& defaultPolicy();
    static RTDrawDescriptor& defaultPolicyKeepColour();
    static RTDrawDescriptor& defaultPolicyKeepDepth();
    static RTDrawDescriptor& defaultPolicyDepthOnly();
    static RTDrawDescriptor& defaultPolicyNoClear();

    /// Resize all attachments
    virtual bool resize(U16 width, U16 height) = 0;

    virtual const RTAttachment_ptr& getAttachmentPtr(RTAttachmentType type, U8 index) const;
    virtual const RTAttachment& getAttachment(RTAttachmentType type, U8 index) const;

    /// Use by multilayered FB's
    virtual void drawToLayer(RTAttachmentType type, U8 index, U16 layer, bool includeDepth = true) = 0;
    // This call sets the target mip level to write to
    // If an attachment does not support the specified mip level, it will be DISABLED to avoid completeness errors
    virtual void setMipLevel(U16 writeLevel) = 0;

    virtual void readData(const vec4<U16>& rect, GFXImageFormat imageFormat, GFXDataFormat dataType, bufferPtr outData) = 0;
    virtual void blitFrom(RenderTarget* inputFB, bool blitColour = true, bool blitDepth = false) = 0;
    virtual void blitFrom(RenderTarget* inputFB, U8 index, bool blitColour = true, bool blitDepth = false) = 0;

    /// Used by cubemap FB's
    void drawToFace(RTAttachmentType type, U8 index, U16 nFace, bool includeDepth = true);

    void readData(GFXImageFormat imageFormat, GFXDataFormat dataType, bufferPtr outData);

    U16 getWidth()  const;
    U16 getHeight() const;

    F32& depthClearValue();

    const stringImpl& name() const;

   protected:
    RTAttachmentPool* _attachmentPool;
    RenderTargetDescriptor _descriptor;
};

};  // namespace Divide


#endif //_RENDER_TARGET_H_
