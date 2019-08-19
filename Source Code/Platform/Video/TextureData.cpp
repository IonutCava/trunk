#include "stdafx.h"

#include "Headers/TextureData.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {
    TextureDataContainer::TextureDataContainer() {
        _textures.reserve(to_size(ShaderProgram::TextureUsage::COUNT));
    }

    TextureDataContainer::~TextureDataContainer()
    {
    }

    bool TextureDataContainer::set(const TextureDataContainer& other) {
        // EASTL should be fast enough to handle this
        const DataEntries& otherTextures = other.textures();
        if (_textures != otherTextures) {
            _textures = otherTextures;
            return true;
        }

        return false;
    }

    TextureDataContainer::UpdateState TextureDataContainer::setTexture(const TextureData& data, U8 binding, bool force) {
        assert(data.type() != TextureType::COUNT);
        return setTextureInternal(data, binding, force);
    }

    TextureDataContainer::UpdateState TextureDataContainer::setTextures(const TextureDataContainer& textureEntries, bool force) {
        return setTextures(textureEntries._textures, force);
    }

    TextureDataContainer::UpdateState TextureDataContainer::setTextures(const DataEntries& textureEntries, bool force) {
        UpdateState ret = UpdateState::COUNT;
        for (auto entry : textureEntries) {
            UpdateState state = setTextureInternal(entry.second, entry.first, force);
            if (ret == UpdateState::COUNT || state != UpdateState::NOTHING) {
                ret = state;
            }
        }

        return ret;
    }

    bool TextureDataContainer::removeTexture(U8 binding) {
        auto ret = _textures.erase(binding);
        assert(ret < 2);
        return ret != 0;
    }

    bool TextureDataContainer::removeTexture(const TextureData& data) {
        for (auto it : _textures) {
            if (it.second == data) {
                _textures.erase(it.first);
                return true;
            }
        }

        return false;
    }

}; //namespace Divide