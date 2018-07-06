#ifndef TEXTURECUBEMAP_H
#define TEXTURECUBEMAP_H

#include <GL/glew.h>
#include "Texture.h"

// -------------------------------
// Textures Cubemap (6 images 2D)
// -------------------------------

class TextureCubemap : public Texture
{
public:
	virtual GLenum getTextureType() const {return GL_TEXTURE_CUBE_MAP;}
	bool load(const std::string& name);
	bool unload(){return true;}
	TextureCubemap() : Texture() {}

};

#endif

