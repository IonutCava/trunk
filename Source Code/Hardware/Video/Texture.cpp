#include "texture.h"

bool Texture::_generateMipmaps = true;

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

