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

#ifndef _PUSH_BUFFER_POOL_H_
#define _PUSH_BUFFER_POOL_H_

#include "PushConstant.h"
#include "config.h"

#include <MemoryPool/StackAlloc.h>
#include <MemoryPool/C-11/MemoryPool.h>

namespace Divide {
namespace GFX {

    class PushConstantPool {
      public:
        PushConstantPool();
        ~PushConstantPool();

        template <typename... Args>
        PushConstant* allocateConstant(Args&&... );

        void deallocateConstant(PushConstant*& constans);

      private:
        mutable SharedLock _mutex;
        size_t _count;
        MemoryPool<PushConstant, 4096> _pool;
    };

    static PushConstantPool s_pushConstantPool;

    template <typename... Args>
    PushConstant* allocatePushConstant(Args&&... args);

    void deallocatePushConstant(PushConstant*& buffer);

}; //namespace GFX
}; //namespace Divide

#endif //_PUSH_BUFFER_POOL_H_

#include "PushConstantPool.inl"