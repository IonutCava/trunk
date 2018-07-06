#include "resource.h"
#include "texture.h"

bool Texture::s_bGenerateMipmaps = true;

bool Texture::LoadFile(GLenum target, const std::string& name)
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

void Texture::LoadData(GLenum target, GLubyte* ptr, U32& w, U32& h, U32 d)
{
	///If the current texture is a 2D one, than converting it to n^2 by n^2 dimensions will result in faster
	///rendering for the cost of a slightly higher loading overhead
	///The conversion code is based on the glmimg code from the glm library;

    if (target == GL_TEXTURE_2D) {

		int xSize2 = w, ySize2 = h;
		double xPow2 = log((double)xSize2) / log(2.0);
		double yPow2 = log((double)ySize2) / log(2.0);

		int ixPow2 = (int)xPow2;
		int iyPow2 = (int)yPow2;

		if (xPow2 != (double)ixPow2)   ixPow2++;
		if (yPow2 != (double)iyPow2)   iyPow2++;

		xSize2 = 1 << ixPow2;
		ySize2 = 1 << iyPow2;
    
		if((w != xSize2) || (h != ySize2)) {
			unsigned char* rdata = new unsigned char[xSize2*ySize2*d];
			gluScaleImage(d==3?GL_RGB:GL_RGBA, w, h,GL_UNSIGNED_BYTE, ptr,
									   xSize2, ySize2, GL_UNSIGNED_BYTE, rdata);
			if(ptr) {
				delete [] ptr;
				ptr = NULL;
			}
			ptr = rdata;
			w = xSize2; h = ySize2;
		}
	}

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

	if(m_nHandle == 0)
		return false;
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


