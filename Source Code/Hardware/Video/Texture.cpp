#include "texture.h"

bool Texture::_generateMipmaps = true;
unordered_map<U8, U32> Texture::textureBoundMap;

Texture::Texture(bool flipped) : Resource(),
								_flipped(flipped),
								_repeatS(false),
								_repeatT(false),
								_minFilter(LINEAR_MIPMAP_LINEAR),
								_magFilter(LINEAR_MIPMAP_LINEAR),
								_handle(0),
								_bound(false),
								_hasTransparency(false)
{
	if(textureBoundMap.empty()){
		for(U8 i = 0; i < 15; i++){
			//Set all 16 texture slots to 0
			textureBoundMap.insert(std::make_pair(i,0));
		}
	}
}


bool Texture::LoadFile(U32 target, const std::string& name)
{
	setResourceLocation(name);
	ImageTools::OpenImage(name,_img);
	if(!_img.data) {
		Console::getInstance().errorfn("Texture: Unable to load texture [ %s ]", name.c_str());
		return false;
	}
	_width = _img.w; _height = _img.h; _bitDepth = _img.d;
	LoadData(target, _img.data, _img.w, _img.h, _img.d);
	_img.Destroy();
	return true;
}

void Texture::resize(U16 width, U16 height){
	_img.resize(width,height);
}

bool Texture::checkBinding(U16 unit, U32 handle){
	if(textureBoundMap[unit] != handle){
		textureBoundMap[unit] = handle;
		return true;
	}
	//return false;
	return true;
}


void Texture::Bind() const {
}

void Texture::Bind(U16 unit)  {
}

void Texture::Unbind() const {
}

void Texture::Unbind(U16 unit) {
}