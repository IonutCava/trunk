#ifndef TEXTURE_H
#define TEXTURE_H

#include "resource.h"
#include "Utility/Headers/BaseClasses.h"
#include "ImageTools.h"

class Texture : public Resource
{

public:
	virtual GLenum getTextureType() const = 0;

	void Gen();
	virtual bool load(const std::string& name);
	void Destroy();

	void Bind(GLuint slot) const;
	void Unbind(GLuint slot) const;

	U32 getHandle() const {return m_nHandle;} 
	U32 getWidth() const {return _width;}
	U32 getHeight() const {return _height;}
	U32 getBitDepth() const {return _bitDepth;}

	const string& getName() const {return m_nName;}
	static void EnableGenerateMipmaps(bool b) {s_bGenerateMipmaps=b;}

	Texture() : m_nHandle(0) {}
	~Texture() {Destroy();}

protected:
	void Bind() const;
	void Unbind() const;
	bool LoadFile(GLenum target, const std::string& name);
	void LoadData(GLenum target, GLubyte* ptr, U32& w, U32& h, U32 d);
	
protected:
	U32	m_nHandle,_width,_height,_bitDepth;				
	std::string m_nName;
	ImageTools::ImageData img;
	static bool s_bGenerateMipmaps;	
};

#endif

