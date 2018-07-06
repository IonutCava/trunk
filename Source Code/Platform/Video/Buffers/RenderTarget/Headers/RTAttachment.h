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

        bool used() const;
        
        bool changed() const;
        void clearChanged();

        void clearColour(const vec4<F32>& clearColour);
        const vec4<F32>& clearColour() const;

        void mipWriteLevel(U16 level);
        U16  mipWriteLevel() const;

        void writeLayer(U16 layer);
        U16  writeLayer() const;

        const Texture_ptr& texture() const;
        void texture(const Texture_ptr& tex);

        U32 binding() const;
        void binding(U32 binding);

    protected:
        bool _changed;
        U32  _binding;
        U16  _mipWriteLevel;
        U16  _writeLayer;
        vec4<F32> _clearColour;
        Texture_ptr _texture;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(RTAttachment);

}; //namespace Divide

#endif //_RENDER_TARGET_ATTACHMENT_H_
