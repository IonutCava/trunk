#include "stdafx.h"

#include "Headers/DescriptorSets.h"

#include "Core/Headers/Console.h"
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
            if (it.first == binding)
                return &it.second;
        }
        return nullptr;
    }

    const TextureView* DescriptorSet::findTextureView(U8 binding) const {
        for (auto& it : _textureViews) {
            if (it._binding == binding) {
                return &it._view;
            }
        }

        return nullptr;
    }

    const Image* DescriptorSet::findImage(U8 binding) const {
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
                                                     [&entry](const ShaderBufferBinding& binding)
                                                      -> bool { return binding._binding == entry._binding; });

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

        auto& otherTextureData = rhs._textureData.textures();

        for (auto it = eastl::begin(otherTextureData); it != eastl::end(otherTextureData);) {
            const TextureData* texData = lhs.findTexture(it->first);
            bool erase = false;
            if (texData == nullptr) {
                lhs._textureData.setTexture(it->second, it->first);
                erase = true;
            } else {
                if (*texData == it->second) {
                    erase = true;
                }
            }
            if (erase) {
                it = otherTextureData.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }

        auto& otherViewList = rhs._textureViews;
        for (auto it = eastl::begin(otherViewList); it != eastl::end(otherViewList);) {
            const TextureViewEntry& otherView = *it;

            const TextureView* texViewData = lhs.findTextureView(otherView._binding);
            bool erase = false;
            if (texViewData == nullptr) {
                lhs._textureViews.push_back(otherView);
                erase = true;
            } else {
                if (*texViewData == otherView._view) {
                    erase = true;
                }
            }

            if (erase) {
                it = otherViewList.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }

        auto& otherImageList = rhs._images;
        for (auto it = eastl::begin(otherImageList); it != eastl::end(otherImageList);) {
            const Image& otherImage = *it;

            const Image* image = lhs.findImage(otherImage._binding);
            bool erase = false;
            if (image == nullptr) {
                lhs._images.push_back(otherImage);
                erase = true;
            } else {
                if (*image == otherImage) {
                    erase = true;
                }
            }

            if (erase) {
                it = otherImageList.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }

        for (auto it = eastl::begin(rhs._shaderBuffers); it != eastl::end(rhs._shaderBuffers);) {
            const ShaderBufferBinding& otherBinding = *it;

            const ShaderBufferBinding* binding = lhs.findBinding(otherBinding._binding);
            bool erase = false;
            if (binding == nullptr) {
                STUBBED("ToDo: This is problematic because we don't know what the current state is. If the current descriptor set doesn't set a binding, that doesn't mean that that binding is empty");
                //lhs._shaderBuffers.push_back(otherBinding);
                //erase = true;
            } else {
                if (*binding == otherBinding) {
                    erase = true;
                }
            }

            if (erase) {
                it = rhs._shaderBuffers.erase(it);
                partial = true;
            } else {
                ++it;
            }
        }

        return rhs._shaderBuffers.empty() && rhs._textureData.textures().empty() && rhs._textureViews.empty() && rhs._images.empty();
    }

    bool ShaderBufferBinding::operator==(const ShaderBufferBinding& other) const {
        return _binding == other._binding &&
               _elementRange == other._elementRange &&
               _buffer->getGUID() == other._buffer->getGUID();
    }

    bool ShaderBufferBinding::operator!=(const ShaderBufferBinding& other) const {
        return _binding != other._binding ||
               _elementRange != other._elementRange ||
               _buffer->getGUID() != other._buffer->getGUID();
    }
}; //namespace Divide