#ifndef _PIXEL_BUFFER_OBJECT_H
#define _PIXEL_BUFFER_OBJECT_H

#include <iostream>
#include "Hardware/Platform/PlatformDefines.h"

class PixelBufferObject 
{
public:

	virtual bool Create(U16 width, U16 height) = 0;
	virtual void Destroy() = 0;

	virtual void* Begin(U8 nFace=0) const = 0;	
	virtual void End(U8 nFace=0) const = 0;		

	virtual void Bind(U8 unit=0) const = 0;		
	virtual void Unbind(U8 unit=0) const = 0;	

	virtual void updatePixels(const F32 * const pixels) = 0;
	inline U32 getTextureHandle() const	{return _textureId;} 
	inline U16 getWidth() const			{return _width;}
	inline U16 getHeight() const		{return _height;}

	virtual ~PixelBufferObject(){};

protected:
	U32         _textureId;
	U16		    _width, _height;
	U32		    _pixelBufferHandle;
	U32		    _textureType;

};


#endif

