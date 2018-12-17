#include "stdafx.h"

#include "Headers/DescriptorSets.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

    const ShaderBufferBinding* DescriptorSet::findBinding(ShaderBufferLocation slot) const {
        for (const ShaderBufferBinding& it : _shaderBuffers) {
            if (it._binding == slot)
                return &it;
        }
        return nullptr;
    }

    const TextureData* DescriptorSet::findTexture(U8 binding) const {
        for (auto& it : _textureData.textures()) {
            if (it.second == binding)
                return &it.first;
        }
        return nullptr;
    }

    ShaderBufferBinding::ShaderBufferBinding()
        : ShaderBufferBinding(ShaderBufferLocation::COUNT, nullptr)
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

    bool ShaderBufferBinding::set(const ShaderBufferBinding& other) {
        return set(other._binding, other._buffer, other._range, other._atomicCounter);
    }

    bool ShaderBufferBinding::set(ShaderBufferLocation binding,
                                  ShaderBuffer* buffer,
                                  const vec2<U16>& range)
    {
        return set(binding, buffer, range, std::make_pair(false, vec2<U32>(0u)));
    }

    bool ShaderBufferBinding::set(ShaderBufferLocation binding,
                                  ShaderBuffer* buffer,
                                  const vec2<U16>& range,
                                  const std::pair<bool, vec2<U32>>& atomicCounter) {
        ACKNOWLEDGE_UNUSED(atomicCounter);
        bool ret = false;
        if (_binding != binding) {
            _binding = binding;
            ret = true;
        }
        if (_buffer != buffer) {
            _buffer = buffer;
            ret = true;
        }
        if (_range != range) {
            _range.set(range);
            ret = true;
        }

        return ret;
    }

    bool Merge(DescriptorSet &lhs, DescriptorSet &rhs, bool& partial) {
        auto& otherTextureData = rhs._textureData.textures();

        vector<vec_size> textureEraseList;
        for (size_t i = 0; i < otherTextureData.size(); ++i) {
            const eastl::pair<TextureData, U8>& otherTexture = otherTextureData[i];

            const TextureData* texData = lhs.findTexture(otherTexture.second);
            bool erase = false;
            if (texData == nullptr) {
                lhs._textureData.addTexture(otherTexture);
                erase = true;
            } else {
                if (*texData == otherTexture.first) {
                    erase = true;
                }
            }
            if (erase) {
                textureEraseList.push_back(i);
                partial = true;
            }
        }
        otherTextureData = erase_indices(otherTextureData, textureEraseList);

        vector<vec_size> bufferEraseList;
        for (size_t i = 0; i < rhs._shaderBuffers.size(); ++i) {
            const ShaderBufferBinding& otherBinding = rhs._shaderBuffers[i];

            const ShaderBufferBinding* binding = lhs.findBinding(otherBinding._binding);
            bool erase = false;
            if (binding == nullptr) {
                //ToDo: This is problematic because we don't know what the current state is
                // If the current descriptor set doesn't set a binding, that doesn't mean that that binding is empty
                /*lhs._shaderBuffers.push_back(otherBinding);
                erase = true;*/
            } else {
                if (*binding == otherBinding) {
                    erase = true;
                }
            }

            if (erase) {
                bufferEraseList.push_back(i);
                partial = true;
            }
        }
        rhs._shaderBuffers = erase_indices(rhs._shaderBuffers, bufferEraseList);

        return rhs._shaderBuffers.empty() && rhs._textureData.textures().empty();
    }

    bool ShaderBufferBinding::operator==(const ShaderBufferBinding& other) const {
        return _binding == other._binding &&
               _range == other._range &&
               _buffer->getGUID() == other._buffer->getGUID();
    }

    bool ShaderBufferBinding::operator!=(const ShaderBufferBinding& other) const {
        return _binding != other._binding ||
               _range != other._range ||
               _buffer->getGUID() != other._buffer->getGUID();
    }
}; //namespace Divide