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

#ifndef _RENDER_TARGET_ATTACHMENT_H_
#define _RENDER_TARGET_ATTACHMENT_H_

#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

FWD_DECLARE_MANAGED_CLASS(Texture);

class RTAttachment {
    public:
        /// This enum is used when creating render targets to define the channel that
        /// the texture will attach to
        enum class Type : U32 {
            Colour = 0,
            Depth = 1,
            Stencil = 2,
            COUNT

        };
    public:
        RTAttachment();
        virtual ~RTAttachment();

        TextureDescriptor& descriptor();
        const TextureDescriptor& descriptor() const;
        virtual void flush();
        virtual bool used() const;
        virtual bool enabled() const;
        virtual bool changed() const;

        virtual void enabled(const bool state);

        void fromDescriptor(const TextureDescriptor& descriptor);

        void clearColour(const vec4<F32>& clearColour);

        const vec4<F32>& clearColour() const;
        // RT should be convertible to a texture on demand!
        virtual Texture_ptr asTexture() const = 0;

        bool dirty() const;
        void flagDirty();
        void clean();

    protected:
        bool _attDirty;
        bool _enabled;
        vec4<F32> _clearColour;
        TextureDescriptor _descriptor;
        bool _needsRefresh;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(RTAttachment);


class RTAttachmentPool {
    public:
        RTAttachmentPool();
        ~RTAttachmentPool();

        const RTAttachment_ptr& get(RTAttachment::Type type, U8 index) const;
        U8 attachmentCount(RTAttachment::Type type) const;

        template<typename T>
        typename std::enable_if<std::is_base_of<RTAttachment, T>::value, void>::type
        init(U8 colourAttachmentCount);

    private:
        std::array<vectorImpl<RTAttachment_ptr>, to_const_uint(RTAttachment::Type::COUNT)> _attachment;
};

template<typename T>
typename std::enable_if<std::is_base_of<RTAttachment, T>::value, void>::type
RTAttachmentPool::init(U8 colourAttachmentCount) {
    for (U8 i = 0; i < colourAttachmentCount; ++i) {
        _attachment[to_const_uint(RTAttachment::Type::Colour)].emplace_back(std::make_shared<T>());
    }

    _attachment[to_const_uint(RTAttachment::Type::Depth)].emplace_back(std::make_shared<T>());
    _attachment[to_const_uint(RTAttachment::Type::Stencil)].emplace_back(std::make_shared<T>());
}

}; //namespace Divide

#endif //_RENDER_TARGET_ATTACHMENT_H_
