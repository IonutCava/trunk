#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Textures/Headers/Texture.h"
#include "Hardware/Video/Textures/Headers/TextureDescriptor.h"

Texture* ImplResourceLoader<Texture>::operator()(){
    Texture* ptr = nullptr;

    if(_descriptor.getEnumValue() == TEXTURE_CUBE_MAP){
        ptr = GFX_DEVICE.newTextureCubemap(_descriptor.getFlag());
    }else{
        ptr = GFX_DEVICE.newTexture2D(_descriptor.getFlag());
    }

    ptr->enableThreadedLoading(_descriptor.getThreaded());
    ptr->setResourceLocation(_descriptor.getResourceLocation());
    //Add the specified sampler, if any o
    if(_descriptor.hasPropertyDescriptor()){
        //cast back to a SamplerDescriptor from a PropertyDescriptor
        const SamplerDescriptor* sampler = dynamic_cast<const SamplerDescriptor*>(_descriptor.getPropertyDescriptor<SamplerDescriptor>());
        ptr->setCurrentSampler(*sampler);
    }

    if(!load(ptr,_descriptor.getResourceLocation())){
        ERROR_FN(Locale::get("ERROR_TEXTURE_LOADER_FILE"),_descriptor.getResourceLocation().c_str(), _descriptor.getName().c_str());
        SAFE_DELETE(ptr)
    }

    return ptr;
}

DEFAULT_HW_LOADER_IMPL(Texture)