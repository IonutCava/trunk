#ifndef TEXTURE_H
#define TEXTURE_H

#include "resource.h"
#include "Utility/Headers/BaseClasses.h"
#include "TextureManager/ImageTools.h"

class Texture : public Resource
{
/*Abstract interface*/
public:
	virtual void Bind(U32 slot) const = 0;
	virtual void Unbind(U32 slot) const = 0;
	virtual void Destroy() = 0;;
	virtual void LoadData(U32 target, U8* ptr, U32& w, U32& h, U32 d) = 0;

protected:
	virtual void Bind() const = 0;
	virtual void Unbind() const = 0;
/*Abstract interface*/

public:

	U32 getHandle() const {return m_nHandle;} 
	U32 getWidth() const {return _width;}
	U32 getHeight() const {return _height;}
	U32 getBitDepth() const {return _bitDepth;}
	const string& getName() const {return m_nName;}

	static void EnableGenerateMipmaps(bool b) {s_bGenerateMipmaps=b;}
	bool LoadFile(U32 target, const std::string& name);
	
protected:
	Texture() : m_nHandle(0){}

protected:
	U32	m_nHandle,_width,_height,_bitDepth;				
	std::string m_nName;
	ImageTools::ImageData img;
	static bool s_bGenerateMipmaps;	
};


#endif

