#include "stdafx.h"

#include "Headers/TextureData.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

size_t TextureData::getHash() const {
    Util::Hash_combine(_hash, to_U32(_textureType));
    Util::Hash_combine(_hash, _samplerHash);
    Util::Hash_combine(_hash, _textureHandle);
    return _hash;
}

TextureDataContainer::TextureDataContainer()
{
    _textures.reserve(to_base(ShaderProgram::TextureUsage::COUNT));
}

TextureDataContainer::~TextureDataContainer()
{
}

bool TextureDataContainer::set(const TextureDataContainer& other) {
    // EASTL should be fast enough to handle this
    const vectorEASTL<eastl::pair<TextureData, U8>>& otherTextures = other.textures();
    if (_textures != otherTextures) {
        _textures = otherTextures;
        return true;
    }

    return false;
}

bool TextureDataContainer::addTexture(const eastl::pair<TextureData, U8 /*binding*/>& textureEntry) {
    return addTexture(textureEntry.first, textureEntry.second);
}

bool TextureDataContainer::addTexture(const TextureData& data, U8 binding) {
    if (Config::Build::IS_DEBUG_BUILD) {
        if (eastl::find_if(eastl::cbegin(_textures),
                           eastl::cend(_textures),
                           [&binding](const eastl::pair<TextureData, U8>& textureData) {
                                 return (textureData.second == binding);
                            }) != eastl::cend(_textures))
        {
            Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_DATA_CONTAINER_CONFLICT")));
            return false;
        }
    }

    _textures.emplace_back(data, binding);
    return true;
}

bool TextureDataContainer::removeTexture(U8 binding) {
    vectorEASTL<eastl::pair<TextureData, U8>>::iterator it;
    it = eastl::find_if(eastl::begin(_textures), eastl::end(_textures),
                      [&binding](const eastl::pair<TextureData, U8>& entry)
    {
        return (entry.second == binding);
    });

    if (it != eastl::end(_textures)) {
        _textures.erase(it);
        return true;
    }

    return false;
}

bool TextureDataContainer::removeTexture(const TextureData& data) {
    size_t inputHash = data.getHash();

    vectorEASTL<eastl::pair<TextureData, U8>>::iterator it;
    it = eastl::find_if(eastl::begin(_textures), eastl::end(_textures),
                      [&inputHash](const eastl::pair<TextureData, U8>& entry) {
                          return (entry.first.getHash() == inputHash);
                      });

    if (it != eastl::end(_textures)) {
        _textures.erase(it);
        return true;
    }

    return false;
}

vectorEASTL<eastl::pair<TextureData, U8>>& TextureDataContainer::textures() {
    return _textures;
}

const vectorEASTL<eastl::pair<TextureData, U8>>& TextureDataContainer::textures() const {
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