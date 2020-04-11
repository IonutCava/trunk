#include "stdafx.h"

#include "Headers/DescriptorSets.h"

#include "Core/Headers/Console.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

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

    const TextureData* DescriptorSet::findTexture(U8 binding) const noexcept {
        auto& textures = _textureData.textures();
        for (const auto& it : textures) {
            if (it.first == binding) {
                return &it.second;
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

        ShaderBufferList::iterator it = std::find_if(std::begin(_shaderBuffers),
                                                     std::end(_shaderBuffers),
                                                     [&entry](const ShaderBufferBinding& binding) noexcept -> bool {
                                                         return binding._binding == entry._binding;
                                                     });

        if (it == std::end(_shaderBuffers)) {
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

    bool Merge(DescriptorSet &lhs, DescriptorSet &rhs, bool& partial) {
        const size_t texCount = rhs._textureData.textures().size();
        for (size_t i = 0; i < texCount; ++i) {
            const auto& it = rhs._textureData.textures()[i];
            if (it.first != TextureDataContainer<>::INVALID_BINDING) {
                const TextureData* texData = lhs.findTexture(it.first);
                if (texData != nullptr && *texData == it.second) {
                    partial = rhs._textureData.removeTexture(it.first) || partial;
                }
            }
        }

        TextureViews& otherViewList = rhs._textureViews;
        for (auto it = eastl::begin(otherViewList); it != eastl::end(otherViewList);) {
            const TextureViewEntry& otherView = *it;

            const TextureView* texViewData = lhs.findTextureView(otherView._binding);
            if (texViewData != nullptr && *texViewData == otherView._view) {
                it = otherViewList.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }

        Images& otherImageList = rhs._images;
        for (auto it = eastl::begin(otherImageList); it != eastl::end(otherImageList);) {
            const Image& otherImage = *it;

            const Image* image = lhs.findImage(otherImage._binding);
            if (image != nullptr && *image == otherImage) {
                it = otherImageList.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }

        for (auto it = eastl::begin(rhs._shaderBuffers); it != eastl::end(rhs._shaderBuffers);) {
            const ShaderBufferBinding& otherBinding = *it;

            const ShaderBufferBinding* binding = lhs.findBinding(otherBinding._binding);
            if (binding != nullptr && *binding == otherBinding) {
                it = rhs._shaderBuffers.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }

        return rhs._shaderBuffers.empty() && rhs._textureData.empty() && rhs._textureViews.empty() && rhs._images.empty();
    }

    size_t TextureView::getHash() const noexcept {
        _hash = 109;
        if (_texture != nullptr) {
            Util::Hash_combine(_hash, _texture->data().textureHandle());
            Util::Hash_combine(_hash, to_base(_texture->data().type()));
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