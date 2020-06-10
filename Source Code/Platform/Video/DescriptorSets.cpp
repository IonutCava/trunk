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

    const ShaderBufferBinding* DescriptorSet::findBinding(ShaderBufferLocation slot) const noexcept {
        for (const ShaderBufferBinding& it : _shaderBuffers) {
            if (it._binding == slot) {
                return &it;
            }
        }

        return nullptr;
    }

    const TextureEntry* DescriptorSet::findTexture(U8 binding) const noexcept {
        auto& textures = _textureData.textures();
        for (const auto& it : textures) {
            if (std::get<0>(it) == binding) {
                return &it;
            }
        }

        return nullptr;
    }

    const TextureView* DescriptorSet::findTextureView(U8 binding) const noexcept {
        for (auto& it : _textureViews) {
            if (it._binding == binding) {
                return &it._view;
            }
        }

        return nullptr;
    }

    const Image* DescriptorSet::findImage(U8 binding) const noexcept {
        for (auto& it : _images) {
            if (it._binding == binding) {
                return &it;
            }
        }

        return nullptr;
    }

    bool DescriptorSet::addShaderBuffers(const ShaderBufferList& entries) {
        bool ret = false;
        for (auto entry : entries) {
            ret = addShaderBuffer(entry) || ret;
        }

        return ret;
    }

    bool DescriptorSet::addShaderBuffer(const ShaderBufferBinding& entry) {
        assert(entry._buffer != nullptr && entry._binding != ShaderBufferLocation::COUNT);

        ShaderBufferList::iterator it = eastl::find_if(eastl::begin(_shaderBuffers),
                                                     eastl::end(_shaderBuffers),
                                                     [&entry](const ShaderBufferBinding& binding) noexcept -> bool {
                                                         return binding._binding == entry._binding;
                                                     });

        if (it == eastl::end(_shaderBuffers)) {
            _shaderBuffers.push_back(entry);
            return true;
        }
         
        return it->set(entry);
    }

    bool ShaderBufferBinding::set(const ShaderBufferBinding& other) {
        return set(other._binding, other._buffer, other._elementRange);
    }

    bool ShaderBufferBinding::set(ShaderBufferLocation binding,
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
                if (std::get<0>(it) != TextureDataContainer<>::INVALID_BINDING) {
                    const TextureEntry* texData = lhs.findTexture(std::get<0>(it));
                    if (texData != nullptr && *texData == it) {
                        partial = rhs._textureData.removeTexture(std::get<0>(it)) || partial;
                    }
                }
            }
        }

        TextureViews& otherViewList = rhs._textureViews;
        if (!otherViewList.empty()) {
            for (auto it = eastl::begin(otherViewList); it != eastl::end(otherViewList);) {
                const TextureView* texViewData = lhs.findTextureView(it->_binding);
                if (texViewData != nullptr && *texViewData == it->_view) {
                    it = otherViewList.erase(it);
                    partial = true;
                } else {
                    ++it;
                }
            }
        }

        Images& otherImageList = rhs._images;
        if (!otherImageList.empty()) {
            for (auto it = eastl::begin(otherImageList); it != eastl::end(otherImageList);) {
                const Image* image = lhs.findImage(it->_binding);
                if (image != nullptr && *image == *it) {
                    it = otherImageList.erase(it);
                    partial = true;
                } else {
                    ++it;
                }
            }
        }

        ShaderBufferList& otherShaderBuffers = rhs._shaderBuffers;
        if (!otherShaderBuffers.empty()) {
            for (auto it = eastl::begin(otherShaderBuffers); it != eastl::end(otherShaderBuffers);) {
                const ShaderBufferBinding* binding = lhs.findBinding(it->_binding);
                if (binding != nullptr && *binding == *it) {
                    it = otherShaderBuffers.erase(it);
                    partial = true;
                } else {
                    ++it;
                }
            }
        }
        return rhs.empty();
    }

    size_t TextureView::getHash() const noexcept {
        _hash = 109;
        if (_texture != nullptr) {
            Util::Hash_combine(_hash, _texture->data()._textureHandle);
            Util::Hash_combine(_hash, to_base(_texture->data()._textureType));
        }

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