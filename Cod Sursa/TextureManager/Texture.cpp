#include "resource.h"
#include "ImageTools.h"
#include "texture.h"

bool Texture::s_bGenerateMipmaps = true;

bool Texture::LoadFile(GLenum target, const std::string& name)
{
	U32 w, h, d;
	GLubyte* ptr = ImageTools::OpenImage(name, w, h, d);
	if(!ptr) {
		std::cout << "[Error] Impossible de charger la texture " << name << std::endl;
		return false;
	}

	LoadData(target, ptr, w, h, d);

	delete[] ptr;
	return true;
}

void Texture::LoadData(GLenum target, GLubyte* ptr, U32 w, U32 h, U32 d)
{
	glTexImage2D(target, 0, d==3?GL_RGB:GL_RGBA, w, h, 0, d==3?GL_RGB:GL_RGBA, GL_UNSIGNED_BYTE, ptr);
}



void Texture::Gen()
{
	Destroy();
	glGenTextures(1, &m_nHandle);
}



bool Texture::load(const std::string& name)
{
	Gen();

	if(m_nHandle == 0){
		std::cout << "Identifiant de texture incorrect" << std::endl;
		return false;
	}

	return true;
}


void Texture::Destroy()
{
	glDeleteTextures(1, &m_nHandle);
}

void Texture::Bind() const {
	glBindTexture(getTextureType(), m_nHandle);
}

void Texture::Bind(GLuint slot) const {
	glActiveTexture(GL_TEXTURE0+slot);
	glEnable(getTextureType());
	glBindTexture(getTextureType(), m_nHandle);
}

void Texture::Unbind() const {
	glBindTexture(getTextureType(), 0);
}

void Texture::Unbind(GLuint slot) const {
	glActiveTexture(GL_TEXTURE0+slot);
	glBindTexture(getTextureType(), 0);
}


