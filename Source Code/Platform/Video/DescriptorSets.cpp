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

    bool IsEmpty(const DescriptorSet& set) noexcept {
        return set._textureData.empty() &&
               set._buffers.empty() &&
               set._textureViews.empty() &&
               set._images.empty();
    }

    bool Merge(const DescriptorSet &lhs, DescriptorSet &rhs, bool& partial) {
        if (rhs._textureData.count() > 0) {
            const auto findTextureDataEntry = [](const TextureDataContainer& source, const U8 binding) -> const TextureEntry* {
                for (const TextureEntry& it : source._entries) {
                    if (it._binding == binding) {
                        return &it;
                    }
                }

                return nullptr;
            };

            const auto& rhsTextures = rhs._textureData._entries;
            for (auto* it = begin(rhsTextures); it != end(rhsTextures);) {

                if (it->_binding == INVALID_TEXTURE_BINDING) {
                    continue;
                }

                const TextureEntry* texData = findTextureDataEntry(lhs._textureData, it->_binding);
                if (texData != nullptr && *texData == *it) {
                    rhs._textureData.remove(it->_binding);
                    partial = true;
                } else {
                    ++it;
                }
            }
        }

        const auto findTextureViewEntry = [](const TextureViews& source, const U8 binding) -> const TextureViewEntry* {
            for (const auto& it : source._entries) {
                if (it._binding == binding) {
                    return &it;
                }
            }

            return nullptr;
        };

        TextureViews& otherViewList = rhs._textureViews;
        for (auto* it = begin(otherViewList._entries); it != end(otherViewList._entries);) {
            const TextureViewEntry* texViewData = findTextureViewEntry(lhs._textureViews, it->_binding);
            if (texViewData != nullptr && texViewData->_view == it->_view) {
                it = otherViewList._entries.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }

        const auto findImage = [](const Images& source, const U8 binding) -> const Image* {
            for (const auto& it : source._entries) {
                if (it._binding == binding) {
                    return &it;
                }
            }

            return nullptr;
        };

        Images& otherImageList = rhs._images;
        for (auto* it = begin(otherImageList._entries); it != end(otherImageList._entries);) {
            const Image* image = findImage(lhs._images, it->_binding);
            if (image != nullptr && *image == *it) {
                it = otherImageList._entries.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }

        ShaderBuffers& otherShaderBuffers = rhs._buffers;
        for (U8 i = 0; i < otherShaderBuffers.count(); ++i) {
            ShaderBufferBinding& it = otherShaderBuffers._entries[i];
            const ShaderBufferBinding* binding = lhs._buffers.find(it._binding);
            if (binding != nullptr && *binding == it) {
                partial = otherShaderBuffers.remove(it._binding) || partial;
            }
        }

        return IsEmpty(rhs);
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

    bool TextureDataContainer::remove(const U8 binding) {
        for (auto it = begin(_entries); it != end(_entries); ++it) {
            if (it->_binding == binding) {
                _entries.erase(it);
                return true;
            }
        }

        return false;
    }

    void TextureDataContainer::sortByBinding() {
        eastl::sort(_entries.begin(), _entries.begin() + count(), [](const TextureEntry& lhs, const TextureEntry& rhs) {
            return lhs._binding < rhs._binding;
        });
    }

    TextureUpdateState TextureDataContainer::add(const TextureEntry& entry) {
        OPTICK_EVENT();
        if (entry._binding != INVALID_TEXTURE_BINDING) {
            for (TextureEntry& it : _entries) {
                if (it._binding == entry._binding) {
                    if (it._hasAddress && it._gpuAddress != entry._gpuAddress) {
                        it._gpuAddress = entry._gpuAddress;
                        it._hasAddress = true;
                        _hasBindlessTextures = true;
                        return TextureUpdateState::REPLACED;
                    }

                    if (it._gpuData != entry._gpuData) {
                        it._gpuData = entry._gpuData;
                        it._hasAddress = false;
                        return TextureUpdateState::REPLACED;
                    }

                    return TextureUpdateState::NOTHING;
                }
            }

            _entries.push_back(entry);
             return TextureUpdateState::ADDED;
        }

        return TextureUpdateState::NOTHING;
    }

    bool operator==(const TextureDataContainer & lhs, const TextureDataContainer & rhs) noexcept {
        const size_t lhsCount = lhs.count();
        const size_t rhsCount = rhs.count();

        if (lhsCount != rhsCount) {
            return false;
        }

        const auto & lhsTextures = lhs._entries;
        const auto & rhsTextures = rhs._entries;

        bool foundEntry = false;
        for (size_t i = 0; i < lhsCount; ++i) {
            const auto & lhsEntry = lhsTextures[i];

            for (const TextureEntry& rhsEntry : rhsTextures) {
                if (rhsEntry._binding == lhsEntry._binding) {
                    if (lhsEntry != rhsEntry) {
                        return false;
                    }
                    foundEntry = true;
                }
            }
        }

        return foundEntry;
    }

    bool operator!=(const TextureDataContainer & lhs, const TextureDataContainer & rhs) noexcept {
        const size_t lhsCount = lhs.count();
        const size_t rhsCount = rhs.count();

        if (lhsCount != rhsCount) {
            return true;
        }

        const auto & lhsTextures = lhs._entries;
        const auto & rhsTextures = rhs._entries;

        bool foundEntry = false;
        for (size_t i = 0; i < lhsCount; ++i) {
            const auto & lhsEntry = lhsTextures[i];

            for (const auto& rhsEntry : rhsTextures) {
                if (rhsEntry._binding == lhsEntry._binding) {
                    if (lhsEntry != rhsEntry) {
                        return true;
                    }
                    foundEntry = true;
                }
            }
        }

        return !foundEntry;
    }

    bool operator==(const DescriptorSet &lhs, const DescriptorSet &rhs) noexcept {
        return lhs._buffers == rhs._buffers &&
               lhs._textureViews == rhs._textureViews &&
               lhs._images == rhs._images &&
               lhs._textureData == rhs._textureData;
    }

    bool operator!=(const DescriptorSet &lhs, const DescriptorSet &rhs) noexcept {
        return lhs._buffers != rhs._buffers ||
               lhs._textureViews != rhs._textureViews ||
               lhs._images != rhs._images ||
               lhs._textureData != rhs._textureData;
    }

    bool operator==(const TextureView& lhs, const TextureView &rhs) noexcept {
        return lhs._samplerHash == rhs._samplerHash &&
               lhs._targetType == rhs._targetType &&
               lhs._mipLevels == rhs._mipLevels &&
               lhs._layerRange == rhs._layerRange &&
               lhs._textureData == rhs._textureData;
    }

    bool operator!=(const TextureView& lhs, const TextureView &rhs) noexcept {
        return lhs._samplerHash != rhs._samplerHash ||
               lhs._targetType != rhs._targetType ||
               lhs._mipLevels != rhs._mipLevels ||
               lhs._layerRange != rhs._layerRange ||
               lhs._textureData != rhs._textureData;
    }

    bool operator==(const TextureViewEntry& lhs, const TextureViewEntry &rhs) noexcept {
        return lhs._binding == rhs._binding &&
               lhs._view == rhs._view &&
               lhs._descriptor == rhs._descriptor;
    }

    bool operator!=(const TextureViewEntry& lhs, const TextureViewEntry &rhs) noexcept {
        return lhs._binding != rhs._binding ||
               lhs._view != rhs._view ||
               lhs._descriptor != rhs._descriptor;
    }

    bool operator==(const ShaderBufferBinding& lhs, const ShaderBufferBinding &rhs) noexcept {
        return lhs._binding == rhs._binding &&
               lhs._elementRange == rhs._elementRange &&
               BufferCompare(lhs._buffer, rhs._buffer);
    }

    bool operator!=(const ShaderBufferBinding& lhs, const ShaderBufferBinding &rhs) noexcept {
        return lhs._binding != rhs._binding ||
               lhs._elementRange != rhs._elementRange ||
               !BufferCompare(lhs._buffer, rhs._buffer);
    }

    bool operator==(const Image& lhs, const Image &rhs) noexcept {
        return lhs._flag == rhs._flag &&
               lhs._layer == rhs._layer &&
               lhs._level == rhs._level &&
               lhs._binding == rhs._binding &&
               lhs._texture == rhs._texture;
    }

    bool operator!=(const Image& lhs, const Image &rhs) noexcept {
        return lhs._flag != rhs._flag ||
               lhs._layer != rhs._layer ||
               lhs._level != rhs._level ||
               lhs._binding != rhs._binding ||
               lhs._texture != rhs._texture;
    }
}; //namespace Divide