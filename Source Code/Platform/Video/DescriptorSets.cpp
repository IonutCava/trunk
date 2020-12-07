#include "stdafx.h"

#include "Headers/DescriptorSets.h"

#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {
    bool BufferCompare(const ShaderBuffer* const a, const ShaderBuffer* const b) noexcept {
        if (a != nullptr && b != nullptr) {
            return a->getGUID() == b->getGUID();
        }

        return a == nullptr && b == nullptr;
    }

    const ShaderBufferBinding* DescriptorSet::findBinding(const ShaderBufferLocation slot) const noexcept {
        for (const ShaderBufferBinding& it : _shaderBuffers) {
            if (it._binding == slot) {
                return &it;
            }
        }

        return nullptr;
    }

    const TextureEntry* DescriptorSet::findTexture(const U8 binding) const noexcept {
        return _textureData.findEntry(binding);
    }

    const TextureViewEntry* DescriptorSet::findTextureViewEntry(const U8 binding) const noexcept {
        for (const auto& it : _textureViews) {
            if (it._binding == binding) {
                return &it;
            }
        }

        return nullptr;
    }

    void DescriptorSet::addTextureViewEntry(const TextureViewEntry& view) noexcept {
        TextureViewEntry* existingEntry = nullptr;
        for (auto& it : _textureViews) {
            if (it._binding == view._binding) {
                existingEntry = &it;
                break;
            }
        }

        if (existingEntry != nullptr) {
            *existingEntry = view;
        } else {
            _textureViews.push_back(view);
        }
    }

    const Image* DescriptorSet::findImage(const U8 binding) const noexcept {
        for (const auto& it : _images) {
            if (it._binding == binding) {
                return &it;
            }
        }

        return nullptr;
    }

    bool DescriptorSet::addShaderBuffers(const ShaderBufferList& entries) {
        bool ret = false;
        for (const auto& entry : entries) {
            ret = addShaderBuffer(entry) || ret;
        }

        return ret;
    }

    bool DescriptorSet::addShaderBuffer(const ShaderBufferBinding& entry) {
        assert(entry._buffer != nullptr && entry._binding != ShaderBufferLocation::COUNT);

        ShaderBufferList::iterator it = eastl::find_if(begin(_shaderBuffers),
                                                       end(_shaderBuffers),
                                                       [&entry](const ShaderBufferBinding& binding) noexcept -> bool {
                                                           return binding._binding == entry._binding;
                                                       });

        if (it == end(_shaderBuffers)) {
            _shaderBuffers.push_back(entry);
            return true;
        }
         
        return it->set(entry);
    }

    bool ShaderBufferBinding::set(const ShaderBufferBinding& other) {
        return set(other._binding, other._buffer, other._elementRange);
    }

    bool ShaderBufferBinding::set(const ShaderBufferLocation binding,
                                  ShaderBuffer* buffer,
                                  const vec2<U32>& elementRange) {
        bool ret = false;
        if (_binding != binding) {
            _binding = binding;
            ret = true;
        }
        if (_buffer != buffer) {
            _buffer = buffer;
            ret = true;
        }
        if (_elementRange != elementRange) {
            _elementRange.set(elementRange);
            ret = true;
        }

        return ret;
    }

    bool Merge(const DescriptorSet &lhs, DescriptorSet &rhs, bool& partial) {
        if (rhs._textureData.count() > 0) {
            const auto& rhsTextures = rhs._textureData.textures();
            const size_t texCount = rhsTextures.size();
            for (size_t i = 0; i < texCount; ++i) {
                const TextureEntry& it = rhsTextures[i];

                if (it._binding == TextureDataContainer<>::INVALID_BINDING) {
                    continue;
                }

                const TextureEntry* texData = lhs.findTexture(it._binding);
                if (texData != nullptr && *texData == it) {
                    partial = rhs._textureData.removeTexture(it._binding) || partial;
                }
            }
        }

        TextureViews& otherViewList = rhs._textureViews;
        for (auto* it = begin(otherViewList); it != end(otherViewList);) {
            const TextureViewEntry* texViewData = lhs.findTextureViewEntry(it->_binding);
            if (texViewData != nullptr && texViewData->_view == it->_view) {
                it = otherViewList.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }

        Images& otherImageList = rhs._images;
        for (auto* it = begin(otherImageList); it != end(otherImageList);) {
            const Image* image = lhs.findImage(it->_binding);
            if (image != nullptr && *image == *it) {
                it = otherImageList.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }

        ShaderBufferList& otherShaderBuffers = rhs._shaderBuffers;
        for (auto* it = begin(otherShaderBuffers); it != end(otherShaderBuffers);) {
            const ShaderBufferBinding* binding = lhs.findBinding(it->_binding);
            if (binding != nullptr && *binding == *it) {
                it = otherShaderBuffers.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }
        
        if (!rhs._textureResidencyQueue.empty()) {
            auto& otherResidentTextures = rhs._textureResidencyQueue;
            for (auto it = begin(otherResidentTextures); it != end(otherResidentTextures);) {
                if (lhs._textureResidencyQueue.find(*it) != end(lhs._textureResidencyQueue)) {
                    it = otherResidentTextures.erase(it);
                    partial = true;
                } else {
                    ++it;
                }
            }
        }

        return rhs.empty();
    }

    size_t TextureView::getHash() const noexcept {
        _hash = GetHash(_textureData);

        Util::Hash_combine(_hash, _targetType);
        Util::Hash_combine(_hash, _mipLevels.x);
        Util::Hash_combine(_hash, _mipLevels.y);
        Util::Hash_combine(_hash, _layerRange.x);
        Util::Hash_combine(_hash, _layerRange.y);

        return _hash;
    }

    size_t TextureViewEntry::getHash() const noexcept {
        _hash = _view.getHash();
        Util::Hash_combine(_hash, _binding);

        return _hash;
    }
}; //namespace Divide