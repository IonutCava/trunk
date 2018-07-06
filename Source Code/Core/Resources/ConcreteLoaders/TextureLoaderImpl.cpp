#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Textures/Headers/Texture.h"
#include "Hardware/Video/Headers/GFXDevice.h"

Texture* ImplResourceLoader<Texture>::operator()(){
	Texture* ptr = NULL;

    if(_descriptor.getEnumValue() == TEXTURE_CUBE_MAP){
		ptr = GFX_DEVICE.newTextureCubemap(_descriptor.getFlag());
    }else{
		ptr = GFX_DEVICE.newTexture2D(_descriptor.getFlag());
	}
	ptr->enableThreadedLoading(_descriptor.getThreaded());
    if(_descriptor.getMask().b.b0 == 1){ //disable mip-maps
        ptr->enableGenerateMipmaps(false);
    }else{
        if(_descriptor.getId() != RAND_MAX){
            ptr->setAnisotrophyLevel(_descriptor.getId());
        }
    }

	if(!load(ptr,_descriptor.getResourceLocation())){
		ERROR_FN(Locale::get("ERROR_TEXTURE_LOADER_FILE"),_descriptor.getResourceLocation().c_str(), _descriptor.getName().c_str());
        SAFE_DELETE(ptr)
	}

	return ptr;
}

DEFAULT_HW_LOADER_IMPL(Texture)