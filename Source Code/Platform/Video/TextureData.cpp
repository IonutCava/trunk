#include "stdafx.h"

#include "Headers/TextureData.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

size_t TextureData::getHash() const {
    Util::Hash_combine(_hash, to_U32(_textureType));
    Util::Hash_combine(_hash, _samplerHash);
    Util::Hash_combine(_hash, _textureHandle);
    return _hash;
}

TextureDataContainer::TextureDataContainer()
{
}

TextureDataContainer::~TextureDataContainer()
{
}

bool TextureDataContainer::set(const TextureDataContainer& other) {
    auto compareTextures = [](const vectorImplFast<std::pair<TextureData, U8>>& a,
                              const vectorImplFast<std::pair<TextureData, U8>>& b) -> bool {
        if (a.size() != b.size()) {
            return false;
        }

        for (vectorAlg::vecSize i = 0; i < a.size(); ++i) {
            if (a[i] != b[i]) {
                return false;
            }
        }

        return true;
    };

    const vectorImplFast<std::pair<TextureData, U8>>& otherTextures = other.textures();
    if (!compareTextures(otherTextures, _textures)) {
        _textures.resize(0);
        _textures.reserve(otherTextures.size());
        _textures.insert(std::begin(_textures),
                         std::begin(otherTextures),
                         std::end(otherTextures));
        return true;
    }

    return false;
}

bool TextureDataContainer::addTexture(const std::pair<TextureData, U8 /*binding*/>& textureEntry) {
    return addTexture(textureEntry.first, textureEntry.second);
}

bool TextureDataContainer::addTexture(const TextureData& data, U8 binding) {
    if (Config::Build::IS_DEBUG_BUILD) {
        if (std::find_if(std::cbegin(_textures),
                         std::cend(_textures),
                         [&binding](const std::pair<TextureData, U8>& textureData) {
                               return (textureData.second == binding);
                          }) != std::cend(_textures))
        {
            Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_DATA_CONTAINER_CONFLICT")));
            return false;
        }
    }

    _textures.emplace_back(data, binding);
    return true;
}

bool TextureDataContainer::removeTexture(U8 binding) {
    vectorImplFast<std::pair<TextureData, U8>>::iterator it;
    it = std::find_if(std::begin(_textures), std::end(_textures),
                      [&binding](const std::pair<TextureData, U8>& entry) {
        return (entry.second == binding);
    });

    if (it != std::end(_textures)) {
        _textures.erase(it);
        return true;
    }

    return false;
}

bool TextureDataContainer::removeTexture(const TextureData& data) {
    size_t inputHash = data.getHash();

    vectorImplFast<std::pair<TextureData, U8>>::iterator it;
    it = std::find_if(std::begin(_textures), std::end(_textures),
                      [&inputHash](const std::pair<TextureData, U8>& entry) {
                          return (entry.first.getHash() == inputHash);
                      });

    if (it != std::end(_textures)) {
        _textures.erase(it);
        return true;
    }

    return false;
}

vectorImplFast<std::pair<TextureData, U8>>& TextureDataContainer::textures() {
    return _textures;
}

const vectorImplFast<std::pair<TextureData, U8>>& TextureDataContainer::textures() const {
    return _textures;
}

void TextureDataContainer::clear(bool clearMemory) {
    if (clearMemory) {
        _textures.clear();
    } else {
        _textures.resize(0);
    }
}

}; //namespace Divide