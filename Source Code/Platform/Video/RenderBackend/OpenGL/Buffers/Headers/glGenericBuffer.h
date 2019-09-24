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
#ifndef _GL_GENERIC_BUFFER_H_
#define _GL_GENERIC_BUFFER_H_

#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"

namespace Divide {

struct BufferParams {
    GLenum _usage = GL_NONE;
    bool   _zeroMem = false;
    bool   _unsynced = true;
    GLuint _ringSizeFactor = 1;
    GLuint _elementCount = 0;
    size_t _elementSizeInBytes = 0;
    BufferUpdateFrequency _frequency = BufferUpdateFrequency::ONCE;
    BufferUpdateUsage _updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
    BufferStorageType _storageType = BufferStorageType::AUTO;
    const char* _name = "";
    bufferPtr _data = nullptr;
};

class glBufferImpl;
class glGenericBuffer {
  public:
      glGenericBuffer(GFXDevice& context, const BufferParams& params);
      ~glGenericBuffer();

      inline GLuint elementCount() const { return _elementCount; }

      GLuint bufferHandle() const;
      
      void zeroMem(GLuint elementOffset, GLuint ringWriteOffset);

      void clearData(GLuint elementOffset, GLuint ringWriteOffset);

      void writeData(GLuint elementCount,
                     GLuint elementOffset,
                     GLuint ringWriteOffset,
                     const bufferPtr data);

      void readData(GLuint elementCount,
                    GLuint elementOffset,
                    GLuint ringReadOffset,
                    bufferPtr dataOut);

      void lockData(GLuint elementCount,
                    GLuint elementOffset,
                    GLuint ringReadOffset,
                    bool flush);

       GLintptr getBindOffset(GLuint ringReadOffset);

       void     setBindOffset(GLuint elementCountOffset);

       glBufferImpl* bufferImpl() const;
  protected:
      GLuint _elementCount;
      GLuint _elementCountBindOffset;
      // A flag to check if the buffer uses the RingBuffer system
      glBufferImpl* _buffer;

      GLuint _ringSizeFactor;
};

}; //namespace Divide

#endif //_GENERIC_BUFFER_H_
