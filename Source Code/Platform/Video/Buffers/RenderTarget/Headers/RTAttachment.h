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
#include "Managers/Headers/FrameListenerManager.h"

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

        void flush();

        bool used() const;
        
        bool changed() const;
        void clearChanged();

        bool toggledState() const;
        void toggledState(const bool state);

        bool enabled() const;
        void enabled(const bool state);

        bool dirty() const;
        void flagDirty();
        void clean();

        void fromDescriptor(const TextureDescriptor& descriptor);

        void clearColour(const vec4<F32>& clearColour);
        const vec4<F32>& clearColour() const;

        void mipMapLevel(U16 min, U16 max);
        const vec2<U16>& mipMapLevel() const;

        const Texture_ptr& asTexture() const;
        void setTexture(const Texture_ptr& tex);

        U32 binding() const;
        void binding(U32 binding);

    protected:
        bool _attDirty;
        bool _enabled;
        bool _toggledState;
        bool _changed;
        U32  _binding;
        Texture_ptr _texture;
        vec4<F32> _clearColour;
        vec2<U16> _mipMapLevel;
        TextureDescriptor _descriptor;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(RTAttachment);

class RenderTarget;
class RTAttachmentPool : public FrameListener {
    public:
        typedef std::array<vectorImpl<RTAttachment_ptr>, to_const_uint(RTAttachment::Type::COUNT)> AttachmentPool;

    public:
        RTAttachmentPool();
        ~RTAttachmentPool();

        void init(RenderTarget* parent, U8 colourAttCount);

        void add(RTAttachment::Type type,
                 U8 index,
                 const TextureDescriptor& descriptor,
                 bool keepPreviousFrame);

        RTAttachment_ptr& get(RTAttachment::Type type, U8 index);
        const RTAttachment_ptr& get(RTAttachment::Type type, U8 index) const;
        const RTAttachment_ptr& getPrevFrame(RTAttachment::Type type, U8 index) const;
        
        U8 attachmentCount(RTAttachment::Type type) const;

        void destroy();

        void onClear();

    protected:
        bool frameEnded(const FrameEvent& evt) override;

    private:
        RTAttachment_ptr& getInternal(AttachmentPool& pool, RTAttachment::Type type, U8 index);
        const RTAttachment_ptr& getInternal(const AttachmentPool& pool, RTAttachment::Type type, U8 index) const;

    private:
        AttachmentPool _attachment;
        AttachmentPool _attachmentHistory;
        vectorImpl<std::pair<RTAttachment::Type, U8>> _attachmentHistoryIndex;
        std::array < U8, to_const_uint(RTAttachment::Type::COUNT)> _attachmentCount;

        bool _isFrameListener;

        RenderTarget* _parent;
};

}; //namespace Divide

#endif //_RENDER_TARGET_ATTACHMENT_H_
