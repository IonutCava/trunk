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

#ifndef _GL_RENDER_TARGET_ATTACHMENT_H_
#define _GL_RENDER_TARGET_ATTACHMENT_H_

#include "Platform/Video/Buffers/RenderTarget/Headers/RTAttachment.h"
#include "Platform/Video/OpenGL/Textures/Headers/glTexture.h"

namespace Divide {
    class glRTAttachment : public RTAttachment {
    public:
        glRTAttachment();
        ~glRTAttachment();

        void flush() override;

        bool used() const override;
        
        Texture_ptr asTexture() const override;

        void clearRefreshFlag();
        void setTexture(const Texture_ptr& tex);

        void setInfo(GLenum slot, U32 handle);
        std::pair<GLenum, U32> getInfo();

        bool toggledState() const;
        void toggledState(const bool state);

        void mipMapLevel(U16 min, U16 max);
        const vec2<U16>& mipMapLevel() const;

    protected:
        bool _toggledState;
        vec2<U16> _mipMapLevel;
        glTexture_ptr _texture;
        std::pair<GLenum, U32> _info;
    };

}; //namespace Divide

#endif //_GL_RENDER_TARGET_ATTACHMENT_H_
