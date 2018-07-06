#include "Core/Headers/Console.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

Texture* ImplResourceLoader<Texture>::operator()(){
    Texture* ptr = nullptr;

    if(_descriptor.getEnumValue() == TEXTURE_CUBE_MAP){
        ptr = GFX_DEVICE.newTextureCubemap(_descriptor.getFlag());
    }else if(_descriptor.getEnumValue() == TEXTURE_2D_ARRAY ||
             _descriptor.getEnumValue() == TEXTURE_2D_ARRAY_MS) {
        ptr = GFX_DEVICE.newTextureArray(_descriptor.getFlag());
        ptr->setNumLayers(_descriptor.getId());
    }else{
        ptr = GFX_DEVICE.newTexture2D(_descriptor.getFlag());
    }

    ptr->enableThreadedLoading(_descriptor.getThreaded());
    ptr->setResourceLocation(_descriptor.getResourceLocation());
    //Add the specified sampler, if any
    if(_descriptor.hasPropertyDescriptor()){
        //cast back to a SamplerDescriptor from a PropertyDescriptor
		ptr->setCurrentSampler( *_descriptor.getPropertyDescriptor<SamplerDescriptor>() );
    }

    if ( !load( ptr, _descriptor.getName() ) ) {
        ERROR_FN(Locale::get( "ERROR_TEXTURE_LOADER_FILE" ), 
                 _descriptor.getResourceLocation().c_str(),
                 _descriptor.getName().c_str() );
        MemoryManager::DELETE( ptr );
    }

    return ptr;
}

DEFAULT_HW_LOADER_IMPL(Texture)

};