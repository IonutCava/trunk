/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _RENDER_TARGET_ATTACHMENT_POOL_H_
#define _RENDER_TARGET_ATTACHMENT_POOL_H_

#include "RTAttachment.h"

namespace Divide {

class RenderTarget;
class RTAttachmentPool {
public:
    typedef std::array<vectorImpl<RTAttachment_ptr>, to_base(RTAttachment::Type::COUNT)> AttachmentPool;

public:
    explicit RTAttachmentPool(RenderTarget& parent, U8 colourAttCount);
    ~RTAttachmentPool();

    void copy(const RTAttachmentPool& other);

    void add(RTAttachment::Type type,
        U8 index,
        const TextureDescriptor& descriptor);

    RTAttachment_ptr& get(RTAttachment::Type type, U8 index);
    const RTAttachment_ptr& get(RTAttachment::Type type, U8 index) const;

    void get(RTAttachment::Type type, vectorImpl<RTAttachment_ptr>& attachments) const;

    U8 attachmentCount(RTAttachment::Type type) const;

    void onClear();

private:
    RTAttachment_ptr& getInternal(AttachmentPool& pool, RTAttachment::Type type, U8 index);
    const RTAttachment_ptr& getInternal(const AttachmentPool& pool, RTAttachment::Type type, U8 index) const;

private:
    AttachmentPool _attachment;
    std::array < U8, to_base(RTAttachment::Type::COUNT)> _attachmentCount;

    bool _isFrameListener;

    RenderTarget& _parent;
};
}; //namespace Divide

#endif //_RENDER_TARGET_ATTACHMENT_POOL_H_