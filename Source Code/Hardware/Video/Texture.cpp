#include "texture.h"
#include "Utility/Headers/ImageTools.h"
#include "Core/Headers/ParamHandler.h"

bool Texture::_generateMipmaps = true;
unordered_map<U8, U32> Texture::textureBoundMap;

Texture::Texture(bool flipped) : HardwareResource(),
								_flipped(flipped),
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
	/// Defaults
	_wrapU = TextureWrap_Repeat;
	_wrapV = TextureWrap_Repeat;
	_wrapW = TextureWrap_Repeat;
}


/// Use DevIL to load a file intro a Texture Object
bool Texture::LoadFile(U32 target, const std::string& name){
	setResourceLocation(name);
	/// Create a new imageData object
	ImageTools::ImageData img;
	/// Flip image if needed
	if(_flipped){
		img._flip = true;
	}
	/// Save file contents in  the "img" object
	ImageTools::OpenImage(name,img,_hasTransparency);
	/// validate data
	if(!img.data) {
		ERROR_FN("Texture: Unable to load texture [ %s ]", name.c_str());
		///Missing texture fallback
		ParamHandler& par = ParamHandler::getInstance();
		ImageTools::OpenImage(par.getParam<std::string>("assetsLocation")+"/"+
			                  par.getParam<std::string>("defaultTextureLocation") +"/"+
							  "missing_texture.jpg", img,_hasTransparency);
		//return false;
	}

	/// Get width
	_width = img.w;
	/// Get height
	_height = img.h;
	/// Get bitdepth
	_bitDepth = img.d;
	/// Create a new API-dependent texture object
	LoadData(target, img.data, _width, _height, _bitDepth);
	/// Unload file data - ImageData destruction handles this
	//img.Destroy();
	return true;
}

void Texture::resize(U16 width, U16 height){
	//_img.resize(width,height);
	/// ToDo: use gluScaleImage if this is needed!
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