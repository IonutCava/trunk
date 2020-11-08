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
#ifndef GL_UNIFORM_BUFFER_OBJECT_H_
#define GL_UNIFORM_BUFFER_OBJECT_H_

#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"

namespace Divide {

class AtomicCounter;
class glBufferImpl;
class glGenericBuffer;
struct BufferLockEntry;

/// Base class for shader uniform blocks
class glUniformBuffer final : public ShaderBuffer {
    public:
        static void onGLInit();

    public:
        glUniformBuffer(GFXDevice& context, const ShaderBufferDescriptor& descriptor);
        ~glUniformBuffer();

        void clearData(U32 offsetElementCount, U32 rangeElementCount) override;
        void readData(U32 offsetElementCount, U32 rangeElementCount, bufferPtr result) const override;
        void writeData(U32 offsetElementCount, U32 rangeElementCount, bufferPtr data) override;
        void writeBytes(ptrdiff_t offsetInBytes, ptrdiff_t rangeInBytes, bufferPtr data) override;
        bool bindRange(U8 bindIndex, U32 offsetElementCount, U32 rangeElementCount) override;
        bool bind(U8 bindIndex) override;

        glBufferImpl* bufferImpl() const;

    protected:
        glBufferImpl* _buffer;
        size_t _alignedBufferSize;
};

};  // namespace Divide

#endif