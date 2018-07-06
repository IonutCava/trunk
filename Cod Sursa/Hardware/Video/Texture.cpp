#include "texture.h"

bool Texture::s_bGenerateMipmaps = true;

bool Texture::LoadFile(U32 target, const std::string& name)
{
	m_nName = name;
	ImageTools::OpenImage(name,img);
	if(!img.data) {
		cout << "Texture: Unable to load texture [ " << name << " ] " << endl;
		return false;
	}
	LoadData(target, img.data, img.w, img.h, img.d);

	return true;
}

