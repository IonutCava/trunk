/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GL_FRAME_BUFFER_OBJECT_H
#define _GL_FRAME_BUFFER_OBJECT_H

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/Buffers/FrameBufferObject/Headers/FrameBufferObject.h"

class glFrameBufferObject : public FrameBufferObject {
public:

    glFrameBufferObject(FBOType type);
    virtual ~glFrameBufferObject();

    virtual bool Create(GLushort width, GLushort height, GLubyte imageLayers = 0);
    virtual void Destroy();
    virtual void DrawToLayer(TextureDescriptor::AttachmentType slot, GLubyte layer, bool includeDepth = true) const; ///<Use by multilayerd FBO's
    virtual void AddDepthBuffer();

    virtual void Begin(GLubyte nFace=0) const;
    virtual void End(GLubyte nFace=0) const;
    virtual void Bind(GLubyte unit=0, TextureDescriptor::AttachmentType slot = TextureDescriptor::Color0) const;
    virtual void Unbind(GLubyte unit=0) const;

    void BlitFrom(FrameBufferObject* inputFBO) const;

    void UpdateMipMaps(TextureDescriptor::AttachmentType slot) const ;

protected:
    bool checkStatus() const;
    void InitAttachement(TextureDescriptor::AttachmentType type, const TextureDescriptor& texDescriptor);
    bool CreateDeferred();

protected:
    GLuint _textureId[5];  ///<4 color attachements and 1 depth
    GLuint _imageLayers;
    GLuint _clearBufferMask;
    bool   _mipMapEnabled[5]; ///< depth may have mipmaps if needed, too
    bool   _hasDepth;
    bool   _hasColor;
    mutable bool _mipMapsDirty;
};

#endif
