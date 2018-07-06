#ifndef TEXTURE_H
#define TEXTURE_H

#include "resource.h"
#include "Utility/Headers/BaseClasses.h"

class Texture : public Resource
{

public:
	virtual GLenum getTextureType() const = 0;

	void Gen();
	virtual bool load(const std::string& name);
	void Destroy();

	void Bind(GLuint slot) const;
	void Unbind(GLuint slot) const;

	GLuint getHandle() const {return m_nHandle;} 

	static void EnableGenerateMipmaps(bool b) {s_bGenerateMipmaps=b;}

	Texture() {m_nHandle=0;}
	~Texture() {Destroy();}

protected:
	void Bind() const;
	void Unbind() const;
	bool LoadFile(GLenum target, const std::string& name);
	void LoadData(GLenum target, GLubyte* ptr, U32 w, U32 h, U32 d);
	
protected:
	GLuint	m_nHandle;				
	static bool s_bGenerateMipmaps;	
};


#endif

