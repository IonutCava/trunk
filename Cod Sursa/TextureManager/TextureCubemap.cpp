#include "TextureCubemap.h"
#include "resource.h"



bool TextureCubemap::load(const std::string& name)
{
	if(!Texture::load(name))
		return false;

	Bind();

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_PACK_ALIGNMENT,1);

	if(!Texture::s_bGenerateMipmaps) {
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


	if(!name.empty()) {
		int i=0;
		std::stringstream ss( name );
		std::string it;
		while(std::getline(ss, it, ' '))
		{
			if(i>=6) {
				std::cout << "ERROR : trop de fichiers specifies pour le chargement de la cubemap : " << name << std::endl;
				return false;
			}

			if(!LoadFile(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, it))
				return false;
			i++;
		}
	}



	Unbind();

	return true;
}
