#include "texture.h"

bool Texture::s_bGenerateMipmaps = true;

bool Texture::LoadFile(U32 target, const string& name)
{
	m_nName = name;
	ImageTools::OpenImage(name,img);
	if(!img.data) {
		Con::getInstance().errorfn("Texture: Unable to load texture [ %s ]", name.c_str());
		return false;
	}
	LoadData(target, img.data, img.w, img.h, img.d);

	return true;
}

