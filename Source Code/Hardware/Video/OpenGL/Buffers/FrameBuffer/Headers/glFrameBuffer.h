/*
   Copyright (c) 2014 DIVIDE-Studio
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
#include "Hardware/Video/Buffers/FrameBuffer/Headers/FrameBuffer.h"

class glFrameBuffer : public FrameBuffer {
public:
    /// if resolveBuffer is not null, we add all of our attachments to it and initialize it with this buffer
    glFrameBuffer(glFrameBuffer* resolveBuffer = nullptr);
    ~glFrameBuffer();

    bool Create(GLushort width, GLushort height);
    void Destroy();
    void DrawToLayer(TextureDescriptor::AttachmentType slot, GLubyte layer, bool includeDepth = true) const;
    void AddDepthBuffer();

    void Begin(const FrameBufferTarget& drawPolicy);
    void End();

    void Bind(GLubyte unit=0, TextureDescriptor::AttachmentType slot = TextureDescriptor::Color0);
    void Unbind(GLubyte unit=0) const;

    void BlitFrom(FrameBuffer* inputFB, TextureDescriptor::AttachmentType slot = TextureDescriptor::Color0, bool blitColor = true, bool blitDepth = false);

    void UpdateMipMaps(TextureDescriptor::AttachmentType slot) const;

protected:
    void resolve();
    bool checkStatus() const;
    void InitAttachment(TextureDescriptor::AttachmentType type, const TextureDescriptor& texDescriptor);

protected:
    GLuint _textureId[5];  ///<4 color attachments and 1 depth
    GLuint _textureType[5];
    GLuint _clearBufferMask;
    GLuint _totalLayerCount;
    bool   _mipMapEnabled[5]; ///< depth may have mipmaps if needed, too
    bool   _hasDepth;
    bool   _hasColor;
    bool   _resolved;
    bool   _isLayeredDepth;
    static bool         _viewportChanged;
    static vec2<U16>    _prevViewportDim;
    vectorImpl<GLenum > _colorBuffers;
    bool _colorMaskChanged;
    bool _depthMaskChanged;
    static bool _mipMapsDirty;
    static GLint _maxColorAttachments;
    glFrameBuffer* _resolveBuffer;
};

#endif
