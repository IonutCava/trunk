#include "Core/Headers/Console.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

template<>
Resource_ptr ImplResourceLoader<Texture>::operator()() {
    assert(_descriptor.getEnumValue() >= to_const_uint(TextureType::TEXTURE_1D) &&
           _descriptor.getEnumValue() < to_const_uint(TextureType::COUNT));

    bool threadedLoad = _descriptor.getThreaded();

    TextureType type = static_cast<TextureType>(_descriptor.getEnumValue());
    if ((type == TextureType::TEXTURE_2D_MS || type == TextureType::TEXTURE_2D_ARRAY_MS) &&
        ParamHandler::instance().getParam<I32>(_ID("rendering.MSAAsampless"), 0) == 0) {
        if (type == TextureType::TEXTURE_2D_MS) {
            type = TextureType::TEXTURE_2D;
        }
        if (type == TextureType::TEXTURE_2D_ARRAY_MS) {
            type = TextureType::TEXTURE_2D_ARRAY;
        }
    }

    Texture_ptr ptr(_context.gfx().newTexture(_descriptor.getName(),
                                              _descriptor.getResourceLocation(),
                                              type,
                                              threadedLoad),
                    DeleteResource(_cache));

    if (_descriptor.getID() > 0) {
        ptr->setNumLayers(to_ubyte(_descriptor.getID()));
    }
    // Add the specified sampler, if any
    if (_descriptor.hasPropertyDescriptor()) {
        // cast back to a SamplerDescriptor from a PropertyDescriptor
        ptr->setCurrentSampler(*_descriptor.getPropertyDescriptor<SamplerDescriptor>());
    }

    if (!load(ptr)) {
        Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LOADER_FILE")),
                         _descriptor.getResourceLocation().c_str(),
                         _descriptor.getName().c_str());
        ptr.reset();
    }

    return ptr;
}

};
