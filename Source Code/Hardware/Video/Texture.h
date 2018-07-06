#ifndef TEXTURE_H
#define TEXTURE_H

#include "resource.h"
#include "Utility/Headers/BaseClasses.h"
#include "TextureManager/ImageTools.h"

class Texture : public Resource
{
/*Abstract interface*/
public:
	virtual void Bind(U16 slot) const = 0;
	virtual void Unbind(U16 slot) const = 0;
	virtual void Destroy() = 0;
	virtual void LoadData(U32 target, U8* ptr, U16& w, U16& h, U8 d) = 0;
	virtual void SetMatrix(U16 slot, const mat4& transformMatrix) = 0;
	virtual void RestoreMatrix(U16 slot) = 0;
	virtual ~Texture() {_img.Destroy();}

protected:
	virtual void Bind() const = 0;
	virtual void Unbind() const = 0;
/*Abstract interface*/

public:

	U32 getHandle() const {return _handle;} 
	U16 getWidth() const {return _width;}
	U16 getHeight() const {return _height;}
	U8  getBitDepth() const {return _bitDepth;}
	bool isFlipped()  {return _flipped;}
	static void EnableGenerateMipmaps(bool b) {s_bGenerateMipmaps=b;}
	bool LoadFile(U32 target, const std::string& name);
	
protected:
	Texture(bool flipped = false) :_flipped(flipped), _handle(0){}

protected:
	U32	_handle;
	U16 _width,_height;
	U8  _bitDepth;			
	bool _flipped;
	ImageTools::ImageData _img;
	static bool s_bGenerateMipmaps;	
};


#endif

