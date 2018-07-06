#include "glResources.h"

#include "resource.h"
#include "glTexture.h"
#include "Hardware/Video/GFXDevice.h"

bool glTexture::load(const string& name)
{
	Gen();
	if(m_nHandle == 0)	return false;

	Bind();

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_PACK_ALIGNMENT,1);

	if(!s_bGenerateMipmaps) {
		glTexParameteri(_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		glTexParameterf(_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameterf(_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(_type, GL_GENERATE_MIPMAP, GL_TRUE);
	}
	if(_flipped) img._flip = true;
	if(_type == GL_TEXTURE_2D)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		if(_flipped) img._flip = true;
		if(!LoadFile(_type,name))
			return false;
	}
	else if(_type == GL_TEXTURE_CUBE_MAP)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		int i=0;
		stringstream ss( name );
		string it;
		while(std::getline(ss, it, ' '))
		{
			if(!LoadFile(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, it))
				return false;
			i++;
		}
	}

	Unbind();
	return true;
}

void glTexture::LoadData(U32 target, U8* ptr, U32& w, U32& h, U32 d)
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


void glTexture::Gen()
{
	Destroy();
	glGenTextures(1, &m_nHandle);
}


void glTexture::Destroy()
{
	glDeleteTextures(1, &m_nHandle);
}

void glTexture::Bind() const {

	glBindTexture(_type, m_nHandle);
}

void glTexture::Bind(U32 slot) const {
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LIGHTING_BIT);

	RenderState& s = GFXDevice::getInstance().getActiveRenderState();
	if(s.isEnabled() && !s.texturesEnabled()) return;
	if(s.isEnabled() && !s.blendingEnabled()) glDisable(GL_BLEND);
	if(s.isEnabled() && !s.cullingEnabled())  glDisable(GL_CULL_FACE);
	if(s.isEnabled() && !s.lightingEnabled()) glDisable(GL_LIGHTING);

	glActiveTexture(GL_TEXTURE0+slot);
	glEnable(_type);
	glBindTexture(_type, m_nHandle);
}

void glTexture::Unbind() const {
	glBindTexture(_type, 0);
}

void glTexture::Unbind(U32 slot) const {

	glActiveTexture(GL_TEXTURE0+slot);
	glBindTexture(_type, 0);
	glPopAttrib();//RenderState
}


