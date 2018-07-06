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

#ifndef _GL_GENERIC_BUFFER_H_
#define _GL_GENERIC_BUFFER_H_

#include "Platform/Video/OpenGL/Headers/glResources.h"

namespace Divide {

struct BufferParams {
    BufferParams()
        : _usage(GL_NONE),
          _ringSizeFactor(1),
          _elementCount(0),
          _elementSizeInBytes(0),
          _frequency(BufferUpdateFrequency::ONCE),
          _data(nullptr),
          _name("")
    {
    }

    GLenum _usage;
    GLuint _ringSizeFactor;
    GLuint _elementCount;
    size_t _elementSizeInBytes;
    BufferUpdateFrequency _frequency;
    bufferPtr   _data;
    const char* _name;
};

class glBufferImpl;
class glGenericBuffer {
  public:
      glGenericBuffer(const BufferParams& params);
      ~glGenericBuffer();

      inline glBufferImpl* impl() const { return _buffer; }
      inline GLuint elementCount() const { return _elementCount; }
      inline size_t elementSize() const { return _elementSize; }

      GLuint bufferHandle() const;
      
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
                    GLuint ringReadOffset);
                    
       GLintptr getBindOffset(GLuint ringReadOffset);

       void     setBindOffset(GLuint elementCountOffset);

  protected:
      GLuint _elementCount;
      size_t _elementSize;
      GLuint _elementCountBindOffset;
      // A flag to check if the buffer uses the RingBuffer system
      glBufferImpl* _buffer;

      GLuint _ringSizeFactor;
};

}; //namespace Divide

#endif //_GENERIC_BUFFER_H_
