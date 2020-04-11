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

    template<size_t Size>
    TextureDataContainer<Size>::TextureDataContainer() {
        _textures.fill({ INVALID_BINDING, {} });
    }

    template<size_t Size>
    bool TextureDataContainer<Size>::set(const TextureDataContainer<Size>& other) {
        // EASTL should be fast enough to handle this
        const DataEntries& otherTextures = other.textures();
        if (_textures != otherTextures) {
            _textures = otherTextures;
            _count = other._count;
            _hash = other._hash
            return true;
        }

        return false;
    }

    template<size_t Size>
    TextureUpdateState TextureDataContainer<Size>::setTexture(const TextureData& data, U8 binding) {
        assert(data.type() != TextureType::COUNT);
        return setTextureInternal(data, binding);
    }

    template<size_t Size>
    TextureUpdateState TextureDataContainer<Size>::setTextures(const TextureDataContainer& textureEntries) {
        return setTextures(textureEntries._textures);
    }

    template<size_t Size>
    TextureUpdateState TextureDataContainer<Size>::setTextures(const DataEntries& textureEntries) {
        UpdateState ret = UpdateState::COUNT;
        for (auto entry : textureEntries) {
            const UpdateState state = setTextureInternal(entry.second, entry.first);
            if (ret == UpdateState::COUNT || state != UpdateState::NOTHING) {
                ret = state;
            }
        }

        return ret;
    }

    template<size_t Size>
    bool TextureDataContainer<Size>::removeTexture(U8 binding) {
#if !defined(CPP_17_SUPPORT)
        for (auto& it : _textures) {
            auto& crtBinding = it.first;
            auto& crtData = it.second;
#else
        for (auto& [crtBinding, crtData] : _textures) {
#endif
            assert(_count > 0);
            if (crtBinding == binding) {
                crtBinding = INVALID_BINDING;
                --_count;
                _hashDirty = true;
                return true;
            }
        }

        return false;
    }

    template<size_t Size>
    bool TextureDataContainer<Size>::removeTexture(const TextureData& data) {
#if !defined(CPP_17_SUPPORT)
        for (auto& it : _textures) {
            auto& crtBinding = it.first;
            auto& crtData = it.second;
#else
        for (auto& [crtBinding, crtData] : _textures) {
#endif
            assert(_count > 0);
            if (crtData == data) {
                crtBinding = INVALID_BINDING;
                --_count;
                _hashDirty = true;
                return true;
            }
        }

        return false;
    }

    template<size_t Size>
    void TextureDataContainer<Size>::clear() {
#if !defined(CPP_17_SUPPORT)
        for (const auto& it : _textures) {
            const auto& crtBinding = it.first;
            const auto& crtData = it.second;
#else
        for (const auto& [crtBinding, crtData] : _textures) {
#endif
            removeTexture(crtBinding);
        }
    }

    template<size_t Size>
    TextureUpdateState TextureDataContainer<Size>::setTextureInternal(const TextureData& data, U8 binding) {
        OPTICK_EVENT();
        if (binding != INVALID_BINDING) {
#if !defined(CPP_17_SUPPORT)
            for (auto& it : _textures) {
                auto& crtBinding = it.first;
                auto& crtData = it.second;
#else
            for (auto& [crtBinding, crtData] : _textures) {
#endif
                if (crtBinding == binding) {
                    if (crtData == data) {
                        crtData = data;
                        _hashDirty = true;
                        return TextureUpdateState::REPLACED;
                    } else {
                        return TextureUpdateState::NOTHING;
                    }
                }
            }
#if !defined(CPP_17_SUPPORT)
            for (auto& it : _textures) {
                auto& crtBinding = it.first;
                auto& crtData = it.second;
#else
            for (auto& [crtBinding, crtData] : _textures) {
#endif
                if (crtBinding == INVALID_BINDING) {
                    crtBinding = binding;
                    crtData = data;
                    ++_count;
                    _hashDirty = true;
                    return TextureUpdateState::ADDED;
                }
            }
        }
        
        return TextureUpdateState::NOTHING;
    }

    template<size_t Size>
    size_t TextureDataContainer<Size>::getHash() const noexcept {
        if (_hashDirty) {
            _hash = 109;
#if !defined(CPP_17_SUPPORT)
            for (const auto& it : _textures) {
                const auto& binding = it.first;
                const auto& data = it.second;
#else
            for (const auto& [binding, data] : _textures) {
#endif
                Util::Hash_combine(_hash, binding);
                if (binding != INVALID_BINDING) {
                    Util::Hash_combine(_hash, data.textureHandle());
                    Util::Hash_combine(_hash, data.samplerHandle());
                    Util::Hash_combine(_hash, data.type());
                }
            }
            _hashDirty = false;
        }
        return _hash;
    }
}; //namespace Divide

#endif //_TEXTURE_DATA_INL_
