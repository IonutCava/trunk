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
    bool BufferCompare(const ShaderBuffer* const a, const ShaderBuffer* const b) noexcept;

    class Texture;
    struct TextureView : public Hashable {
        Texture* _texture = nullptr;
        vec2<U16> _mipLevels = {};
        vec2<U16> _layerRange = {};

        inline bool operator==(const TextureView& other) const noexcept {
            return _mipLevels == other._mipLevels &&
                   _layerRange == other._layerRange &&
                   _texture == other._texture;
        }

        inline bool operator!=(const TextureView& other) const noexcept {
            return _mipLevels != other._mipLevels ||
                   _layerRange != other._layerRange ||
                   _texture != other._texture;
        }

        size_t getHash() const noexcept override;
    };

    struct TextureViewEntry : public Hashable {
        TextureView _view = {};
        U8 _binding = 0;

        inline bool operator==(const TextureViewEntry& other) const noexcept {
            return _binding == other._binding &&
                   _view == other._view;
        }

        inline bool operator!=(const TextureViewEntry& other) const noexcept {
            return _binding != other._binding ||
                   _view != other._view;
        }

        size_t getHash() const noexcept override;
    };

    struct ShaderBufferBinding {
        vec2<U32>     _elementRange;
        ShaderBuffer* _buffer = nullptr;
        ShaderBufferLocation _binding = ShaderBufferLocation::COUNT;

        bool set(const ShaderBufferBinding& other);
        bool set(ShaderBufferLocation binding, ShaderBuffer* buffer, const vec2<U32>& elementRange);

        inline bool operator==(const ShaderBufferBinding& other) const noexcept {
            return _binding == other._binding &&
                   _elementRange == other._elementRange &&
                   BufferCompare(_buffer, other._buffer);
        }

        inline bool operator!=(const ShaderBufferBinding& other) const noexcept {
            return _binding != other._binding ||
                   _elementRange != other._elementRange ||
                   !BufferCompare(_buffer, other._buffer);
        }

        XALLOCATOR
    };

    struct Image {
        enum class Flag : U8 {
            READ = 0,
            WRITE,
            READ_WRITE
        };

        Texture* _texture = nullptr;
        Flag _flag = Flag::READ;
        U8 _layer = 0;
        U8 _level = 0;
        U8 _binding = 0;

        inline bool operator==(const Image& other) const noexcept {
            return _flag == other._flag &&
                   _layer == other._layer &&
                   _level == other._level &&
                   _binding == other._binding &&
                   _texture == other._texture;
        }

        inline bool operator!=(const Image& other) const noexcept {
            return _flag != other._flag ||
                   _layer != other._layer ||
                   _level != other._level ||
                   _binding != other._binding ||
                   _texture != other._texture;
        }
    };

    using ShaderBufferList = eastl::fixed_vector<ShaderBufferBinding, 4, true>;
    using TextureViews = vectorEASTLFast<TextureViewEntry>;
    using Images = vectorEASTLFast<Image>;

    struct DescriptorSet {
        //This needs a lot more work!
        TextureDataContainer<> _textureData = {};
        ShaderBufferList _shaderBuffers = {};
        TextureViews _textureViews = {};
        Images _images = {};

        bool addShaderBuffer(const ShaderBufferBinding& entry);
        bool addShaderBuffers(const ShaderBufferList& entries);
        const ShaderBufferBinding* findBinding(ShaderBufferLocation slot) const noexcept;
        const TextureData* findTexture(U8 binding) const noexcept;
        const TextureView* findTextureView(U8 binding) const noexcept;
        const Image* findImage(U8 binding) const noexcept;

        inline bool operator==(const DescriptorSet &other) const noexcept {
            return _shaderBuffers == other._shaderBuffers &&
                   _textureViews == other._textureViews &&
                   _images == other._images &&
                   _textureData == other._textureData;
        }

        inline bool operator!=(const DescriptorSet &other) const noexcept {
            return _shaderBuffers != other._shaderBuffers ||
                   _textureViews != other._textureViews ||
                   _images != other._images ||
                   _textureData != other._textureData;
        }

        inline bool empty() const noexcept {
            return  _textureData.count() == 0 &&
                    _shaderBuffers.empty() &&
                    _textureViews.empty() &&
                    _images.empty();
        }

        XALLOCATOR
    };

    bool Merge(const DescriptorSet &lhs, DescriptorSet &rhs, bool& partial);


}; //namespace Divide

#endif //_DESCRIPTOR_SETS_H_
