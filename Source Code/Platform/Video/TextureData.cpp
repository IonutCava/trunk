#include "Headers/TextureData.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Core/Math/Headers/MathHelper.h"

namespace Divide {

size_t TextureData::getHash() const {
    size_t hash = 0;
    Util::Hash_combine(hash, to_uint(_textureType));
    Util::Hash_combine(hash, _samplerHash);
    Util::Hash_combine(hash, _textureHandle);
    return hash;
}

TextureDataContainer::TextureDataContainer()
{
}

TextureDataContainer::~TextureDataContainer()
{
}

bool TextureDataContainer::addTexture(const TextureData& data) {
    if (Config::Build::IS_DEBUG_BUILD) {
        vectorImpl<TextureData>::const_iterator it;
        it = std::find_if(std::cbegin(_textures),
                          std::cend(_textures),
                          [&data](const TextureData& textureData) {
                                return (textureData.getHandleLow() == data.getHandleLow());
                           });

        if (it == std::cend(_textures)) {
            _textures.push_back(data);
            return true;
        }

        Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_DATA_CONTAINER_CONFLICT")));
        return false;
    } else {
        _textures.push_back(data);
    }

    return true;
}

bool TextureDataContainer::removeTexture(const TextureData& data) {
    size_t inputHash = data.getHash();

    vectorImpl<TextureData>::iterator it;
    it = std::find_if(std::begin(_textures), std::end(_textures),
                      [&inputHash](const TextureData& textureData) {
                          return (textureData.getHash() == inputHash);
                      });

    if (it != std::end(_textures)) {
        _textures.erase(it);
        return true;
    }

    return false;
}

vectorImpl<TextureData>& TextureDataContainer::textures() {
    return _textures;
}

const vectorImpl<TextureData>& TextureDataContainer::textures() const {
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