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

#ifndef _DESCRIPTOR_SETS_H_
#define _DESCRIPTOR_SETS_H_

#include "TextureData.h"
#include "Core/Math/Headers/MathVectors.h"

namespace Divide {
    class ShaderBuffer;
    struct ShaderBufferBinding {
        ShaderBufferLocation _binding;
        ShaderBuffer* _buffer;
        vec2<U32>     _range;
        std::pair<bool, vec2<U32>> _atomicCounter;

        ShaderBufferBinding()
            : ShaderBufferBinding(ShaderBufferLocation::COUNT,
                                  nullptr,
                                  vec2<U32>(0, 0))
        {
        }

        ShaderBufferBinding(ShaderBufferLocation slot,
                            ShaderBuffer* buffer,
                            const vec2<U32>& range)
            : ShaderBufferBinding(slot, buffer, range, std::make_pair(false, vec2<U32>(0u)))
        {
        }

        ShaderBufferBinding(ShaderBufferLocation slot,
                            ShaderBuffer* buffer,
                            const vec2<U32>& range,
                            const std::pair<bool, vec2<U32>>& atomicCounter)
            : _binding(slot),
              _buffer(buffer),
              _range(range),
              _atomicCounter(atomicCounter)
        {
        }

        inline void set(const ShaderBufferBinding& other) {
            set(other._binding, other._buffer, other._range, other._atomicCounter);
        }

        inline void set(ShaderBufferLocation binding,
                        ShaderBuffer* buffer,
                        const vec2<U32>& range)
        {
            set(binding, buffer, range, std::make_pair(false, vec2<U32>(0u)));
        }

        inline void set(ShaderBufferLocation binding,
                        ShaderBuffer* buffer,
                        const vec2<U32>& range,
                        const std::pair<bool, vec2<U32>>& atomicCounter) {
            _binding = binding;
            _buffer = buffer;
            _range.set(range);
        }
    };

    typedef vectorImpl<ShaderBufferBinding> ShaderBufferList;


    struct DescriptorSet {
        //This needs a lot more work!
        ShaderBufferList _shaderBuffers;
        TextureDataContainer _textureData;
    };
}; //namespace Divide

#endif //_DESCRIPTOR_SETS_H_
