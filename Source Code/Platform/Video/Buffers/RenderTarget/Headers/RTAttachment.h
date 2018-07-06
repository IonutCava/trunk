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
#ifndef _RENDER_TARGET_ATTACHMENT_H_
#define _RENDER_TARGET_ATTACHMENT_H_

#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

FWD_DECLARE_MANAGED_CLASS(Texture);
FWD_DECLARE_MANAGED_CLASS(RTAttachment);

/// This enum is used when creating render targets to define the channel that the texture will attach to
enum class RTAttachmentType : U8 {
    Colour = 0,
    Depth = 1,
    Stencil = 2,
    COUNT
};

// Only for colour attachments
struct ExternalRTAttachmentDescriptor {
    RTAttachment_ptr _attachment;
    U8 _index = 0;
};

struct RTAttachmentDescriptor {
    TextureDescriptor _texDescriptor;
    RTAttachmentType _type = RTAttachmentType::COUNT;
    U8 _index = 0;
    FColour _clearColour = DefaultColours::WHITE;
};

class RTAttachment {
    public:
        explicit RTAttachment(const RTAttachmentDescriptor& descriptor);
        explicit RTAttachment(const RTAttachmentDescriptor& descriptor, const RTAttachment_ptr& externalAtt);
        virtual ~RTAttachment();

        bool used() const;
        
        bool isExternal() const;

        bool changed() const;
        void clearChanged();

        void clearColour(const FColour& clearColour);
        const FColour& clearColour() const;

        void mipWriteLevel(U16 level);
        U16  mipWriteLevel() const;

        void writeLayer(U16 layer);
        U16  writeLayer() const;

        const Texture_ptr& texture() const;
        void texture(const Texture_ptr& tex);

        U32 binding() const;
        void binding(U32 binding);

        const RTAttachmentDescriptor& descriptor() const;

    protected:
        bool _changed;
        U32  _binding;
        U16  _mipWriteLevel;
        U16  _writeLayer;
        Texture_ptr _texture;
        RTAttachment_ptr _externalAttachment;
        RTAttachmentDescriptor _descriptor;
};

FWD_DECLARE_MANAGED_CLASS(RTAttachment);

}; //namespace Divide

#endif //_RENDER_TARGET_ATTACHMENT_H_
