#include "stdafx.h"

#include "Headers/DescriptorSets.h"

namespace Divide {
    ShaderBufferBinding::ShaderBufferBinding()
        : ShaderBufferBinding(ShaderBufferLocation::COUNT, nullptr, vec2<U32>(0, 0))
    {
    }

    ShaderBufferBinding::ShaderBufferBinding(ShaderBufferLocation slot,
                                             ShaderBuffer* buffer)
        : ShaderBufferBinding(slot, buffer, vec2<U32>(0, 0))
    {
    }

    ShaderBufferBinding::ShaderBufferBinding(ShaderBufferLocation slot,
                                             ShaderBuffer* buffer,
                                             const vec2<U32>& range)
        : ShaderBufferBinding(slot, buffer, range, std::make_pair(false, vec2<U32>(0u)))
    {
    }

    ShaderBufferBinding::ShaderBufferBinding(ShaderBufferLocation slot,
                                             ShaderBuffer* buffer,
                                             const vec2<U32>& range,
                                             const std::pair<bool, vec2<U32>>& atomicCounter)
      : _binding(slot),
        _buffer(buffer),
        _range(range),
        _atomicCounter(atomicCounter)
    {
    }

    void ShaderBufferBinding::set(const ShaderBufferBinding& other) {
        set(other._binding, other._buffer, other._range, other._atomicCounter);
    }

    void ShaderBufferBinding::set(ShaderBufferLocation binding,
                                  ShaderBuffer* buffer,
                                  const vec2<U16>& range)
    {
        set(binding, buffer, range, std::make_pair(false, vec2<U32>(0u)));
    }

    void ShaderBufferBinding::set(ShaderBufferLocation binding,
                                  ShaderBuffer* buffer,
                                  const vec2<U16>& range,
                                  const std::pair<bool, vec2<U32>>& atomicCounter) {
        ACKNOWLEDGE_UNUSED(atomicCounter);
        _binding = binding;
        _buffer = buffer;
        _range.set(range);
    }

    bool Merge(DescriptorSet &lhs, const DescriptorSet &rhs) {
        // Check stage
        for (const ShaderBufferBinding& ourBinding : lhs._shaderBuffers) {
            for (const ShaderBufferBinding& otherBinding : rhs._shaderBuffers) {
                // Make sure the bindings are different
                if (ourBinding._binding == otherBinding._binding) {
                    return false;
                }
            }
        }
        auto ourTextureData = lhs._textureData.textures();
        auto otherTextureData = rhs._textureData.textures();

        for (const eastl::pair<TextureData, U8>& ourTexture : ourTextureData) {
            for (const eastl::pair<TextureData, U8>& otherTexture : otherTextureData) {
                // Make sure the bindings are different
                if (ourTexture.second == otherTexture.second && ourTexture.first != otherTexture.first) {
                    return false;
                }
            }
        }

        // Merge stage
        lhs._shaderBuffers.insert(eastl::cend(lhs._shaderBuffers),
                                  eastl::cbegin(rhs._shaderBuffers),
                                  eastl::cend(rhs._shaderBuffers));

        // The incoming texture data is either identical or new at this point, so only insert unique items
        insert_unique(lhs._textureData.textures(), rhs._textureData.textures());
        return true;
    }
}; //namespace Divide