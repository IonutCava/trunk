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

#ifndef _GL_PIXEL_BUFFER_OBJECT_H
#define _GL_PIXEL_BUFFER_OBJECT_H

#include "Platform/Video/Buffers/PixelBuffer/Headers/PixelBuffer.h"

namespace Divide {
class GL_API;

namespace Attorney {
    class GLAPIPixelBuffer;
};

class glPixelBuffer : public PixelBuffer {
    friend class Attorney::GLAPIPixelBuffer;

   public:
    explicit glPixelBuffer(GFXDevice& context, PBType type);
    ~glPixelBuffer();

    bool create(GLushort width, GLushort height, GLushort depth = 0,
                GFXImageFormat internalFormatEnum = GFXImageFormat::RGBA8,
                GFXImageFormat formatEnum = GFXImageFormat::RGBA,
                GFXDataFormat dataTypeEnum = GFXDataFormat::FLOAT_32) override;

    void bind(GLubyte unit = 0) const  override;

    void updatePixels(const GLfloat* const pixels, GLuint pixelCount)  override;

   protected:
    friend class GL_API;
    bufferPtr begin() const;
    void end() const;

   private:
    size_t sizeOf(GLenum dataType) const;

   private:
    GLuint _bufferSize;
    GLuint _dataSizeBytes;
    GLenum _dataType;
    GLenum _format;
    GLenum _internalFormat;
};

namespace Attorney {
    class GLAPIPixelBuffer {
        private:
        static bufferPtr begin(glPixelBuffer& buffer) {
            return buffer.begin();
        }
        static void end(glPixelBuffer& buffer) {
            buffer.end();
        }
        friend class Divide::GL_API;
    };
};  // namespace Attorney
};  // namespace Divide

#endif
