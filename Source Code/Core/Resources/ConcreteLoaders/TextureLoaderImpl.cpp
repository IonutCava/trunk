#include "Core/Resources/Headers/ResourceLoader.h"
#include "Hardware/Video/Texture.h"
#include "Hardware/Video/GFXDevice.h"

Texture* ImplResourceLoader<Texture>::operator()(){

	Texture* ptr = NULL;
	std::stringstream ss( _descriptor.getResourceLocation() );
	std::string it;
	I8 i = 0;
	while(std::getline(ss, it, ' ')) i++;

	if(i == 6){
		ptr = GFX_DEVICE.newTextureCubemap(_descriptor.getFlag());
	}else if (i == 1){
		ptr = GFX_DEVICE.newTexture2D(_descriptor.getFlag());
	}else{
		ERROR_FN("TextureLoader: wrong number of files for cubemap texture: [ %s ]", _descriptor.getName().c_str());
		return NULL;
	}

	if(!load(ptr,_descriptor.getResourceLocation())){
		ERROR_FN("TextureLoader: could not load texture file [ (%s)%s ]",_descriptor.getResourceLocation().c_str(), _descriptor.getName().c_str());
		return NULL;
	}

	return ptr;
}

DEFAULT_HW_LOADER_IMPL(Texture)