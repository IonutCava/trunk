#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include "Texture.h"

class FrameBufferObject;

// -------------------------------
// Texture 2D
// -------------------------------

class Texture2D : public Texture
{
public:
	virtual GLenum getTextureType() const {return GL_TEXTURE_2D;}
	bool load(const std::string& name);
	bool load(GLubyte* ptr, U32 w, U32 h, U32 d);
	bool unload(); 

	Texture2D() : Texture() {}

};

#endif

