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
#ifndef _TEXTURE_DATA_INL_
#define _TEXTURE_DATA_INL_

namespace Divide {
    inline bool operator==(const TextureEntry & lhs, const TextureEntry & rhs) noexcept {
        return lhs._binding == rhs._binding &&
               lhs._sampler == rhs._sampler &&
               lhs._data == rhs._data;
    }

    inline bool operator!=(const TextureEntry & lhs, const TextureEntry & rhs) noexcept {
        return lhs._binding != rhs._binding ||
               lhs._sampler != rhs._sampler ||
               lhs._data != rhs._data;
    }

    template<size_t Size>
    bool TextureDataContainer<Size>::set(const TextureDataContainer<Size>& other) {
        // EASTL should be fast enough to handle this
        const DataEntries& otherTextures = other.textures();
        if (_textures != otherTextures) {
            _textures = otherTextures;
            _count = other._count;
            return true;
        }

        return false;
    }

    template<size_t Size>
    TextureUpdateState TextureDataContainer<Size>::setTexture(const TextureData& data, const size_t samplerHash, const U8 binding) {
        assert(data._textureType != TextureType::COUNT);
        return setTextureInternal(data, samplerHash, binding);
    }

    template<size_t Size>
    TextureUpdateState TextureDataContainer<Size>::setTexture(const TextureData& data, const size_t samplerHash, const TextureUsage binding) {
        return setTexture(data, samplerHash, to_U8(binding));
    }

    template<size_t Size>
    TextureUpdateState TextureDataContainer<Size>::setTextures(const TextureDataContainer& textureEntries) {
        return setTextures(textureEntries._textures);
    }

    template<size_t Size>
    TextureUpdateState TextureDataContainer<Size>::setTextures(const DataEntries& textureEntries) {
        TextureUpdateState ret = TextureUpdateState::COUNT;
        for (const TextureEntry& entry : textureEntries) {
            const TextureUpdateState state = setTextureInternal(entry._data, entry._sampler, entry._binding);
            if (ret == TextureUpdateState::COUNT || state != TextureUpdateState::NOTHING) {
                ret = state;
            }
        }

        return ret;
    }

    template<size_t Size>
    bool TextureDataContainer<Size>::removeTexture(const U8 binding) {
        for (TextureEntry& it : _textures) {
            U8& crtBinding = it._binding;
            assert(_count > 0u);
            if (crtBinding == binding) {
                crtBinding = INVALID_BINDING;
                --_count;
                return true;
            }
        }

        return false;
    }

    template<size_t Size>
    bool TextureDataContainer<Size>::removeTexture(const TextureData& data) {
        assert(_count > 0u);
        for (TextureEntry& it : _textures) {
            U8& crtBinding = it._binding;
            if (it._data == data) {
                crtBinding = INVALID_BINDING;
                --_count;
                return true;
            }
        }

        return false;
    }

    template<size_t Size>
    void TextureDataContainer<Size>::clear() {
        for (const TextureEntry& it : _textures) {
            removeTexture(it._binding);
        }
    }

    template<size_t Size>
    void TextureDataContainer<Size>::sortByBinding() {
        eastl::sort(_textures.begin(), _textures.begin() + count(), [](const TextureEntry& lhs, const TextureEntry& rhs) {
            return lhs._binding < rhs._binding;
        });
    }

    template<size_t Size>
    TextureUpdateState TextureDataContainer<Size>::setTextureInternal(const TextureData& data, const size_t samplerHash, const U8 binding) {
        OPTICK_EVENT();
        if (binding != INVALID_BINDING) {
            for (TextureEntry& it : _textures) {
                if (it._binding == binding) {
                    TextureData& crtData = it._data;
                    size_t& crtSamp = it._sampler;
                    if (crtData != data || crtSamp != samplerHash) {
                        crtData = data;
                        crtSamp = samplerHash;
                        return TextureUpdateState::REPLACED;
                    }
                     
                    return TextureUpdateState::NOTHING;
                }
            }
            for (TextureEntry& it : _textures) {
                U8& crtBinding = it._binding;
                if (crtBinding == INVALID_BINDING) {
                    crtBinding = binding;
                    it._data = data;
                    it._sampler = samplerHash;
                    ++_count;
                    return TextureUpdateState::ADDED;
                }
            }
        }
        
        return TextureUpdateState::NOTHING;
    }

    template<size_t Size>
    bool operator==(const TextureDataContainer<Size> & lhs, const TextureDataContainer<Size> & rhs) noexcept {
        const size_t lhsCount = lhs.count();
        const size_t rhsCount = rhs.count();
    
        if (lhsCount != rhsCount) {
            return false;
        }

        const auto & lhsTextures = lhs.textures();
        const auto & rhsTextures = rhs.textures();
    
        bool foundEntry = false;
        for (size_t i = 0; i < lhsCount; ++i) {
            const auto & lhsEntry = lhsTextures[i];
        
            for (const TextureEntry& rhsEntry : rhsTextures) {
                if (rhsEntry._binding == lhsEntry._binding) {
                    if (rhsEntry._sampler != lhsEntry._sampler || rhsEntry._data != lhsEntry._data) {
                        return false;
                    }
                    foundEntry = true;
                }
            }
        }

        return foundEntry;
    }

    template<size_t Size>
    bool operator!=(const TextureDataContainer<Size> & lhs, const TextureDataContainer<Size> & rhs) noexcept {
        const size_t lhsCount = lhs.count();
        const size_t rhsCount = rhs.count();

        if (lhsCount != rhsCount) {
            return true;
        }

        const auto & lhsTextures = lhs.textures();
        const auto & rhsTextures = rhs.textures();

        bool foundEntry = false;
        for (size_t i = 0; i < lhsCount; ++i) {
            const auto & lhsEntry = lhsTextures[i];

            for (const auto& rhsEntry : rhsTextures) {
                if (rhsEntry._binding == lhsEntry._binding) {
                    if (rhsEntry._sampler != lhsEntry._sampler || rhsEntry._data != lhsEntry._data) {
                        return true;
                    }
                    foundEntry = true;
                }
            }
        }

        return !foundEntry;
    }
}; //namespace Divide

#endif //_TEXTURE_DATA_INL_
