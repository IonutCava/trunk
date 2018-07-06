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
#ifndef _DESCRIPTOR_SETS_H_
#define _DESCRIPTOR_SETS_H_

#include "ClipPlanes.h"
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
            ACKNOWLEDGE_UNUSED(atomicCounter);
            _binding = binding;
            _buffer = buffer;
            _range.set(range);
        }

        inline bool operator==(const ShaderBufferBinding& other) const {
            return _binding == other._binding &&
                   _buffer == other._buffer &&
                   _range == other._range;
        }

        inline bool operator!=(const ShaderBufferBinding& other) const {
            return _binding != other._binding ||
                   _buffer != other._buffer ||
                   _range != other._range;
        }
    };

    typedef vectorImpl<ShaderBufferBinding> ShaderBufferList;


    struct DescriptorSet {
        //This needs a lot more work!
        ShaderBufferList _shaderBuffers;
        TextureDataContainer _textureData;


        inline bool operator==(const DescriptorSet &other) const {
            return _shaderBuffers == other._shaderBuffers &&
                   _textureData == other._textureData;
        }

        inline bool operator!=(const DescriptorSet &other) const {
            return _shaderBuffers != other._shaderBuffers ||
                   _textureData != other._textureData;
        }

        inline bool merge(const DescriptorSet &other) {
            // Check stage
            for (const ShaderBufferBinding& ourBinding : _shaderBuffers) {
                for (const ShaderBufferBinding& otherBinding : other._shaderBuffers) {
                    // Make sure the bindings are different
                    if (ourBinding._binding == otherBinding._binding) {
                        return false;
                    }
                }
            }
            auto ourTextureData = _textureData.textures();
            auto otherTextureData = other._textureData.textures();

            for (const std::pair<TextureData, U8>& ourTexture : ourTextureData) {
                for (const std::pair<TextureData, U8>& otherTexture : otherTextureData) {
                    // Make sure the bindings are different
                    if (ourTexture.second == otherTexture.second && ourTexture.first != otherTexture.first) {
                        return false;
                    }
                }
            }

            // Merge stage
            _shaderBuffers.insert(std::cend(_shaderBuffers),
                                  std::cbegin(other._shaderBuffers),
                                  std::cend(other._shaderBuffers));

            // The incoming texture data is either identical or new at this point, so only insert unique items
            insert_unique(_textureData.textures(), other._textureData.textures());
            return true;
        }
    };
}; //namespace Divide

#endif //_DESCRIPTOR_SETS_H_
