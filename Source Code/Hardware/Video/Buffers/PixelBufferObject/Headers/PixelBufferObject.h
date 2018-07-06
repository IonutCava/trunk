/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PIXEL_BUFFER_OBJECT_H
#define _PIXEL_BUFFER_OBJECT_H

#include "Hardware/Platform/Headers/PlatformDefines.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"

class PixelBufferObject {

public:

	virtual bool Create(U16 width, U16 height,U16 depth = 0, 
						GFXImageFormat internalFormatEnum = RGBA8, 
						GFXImageFormat formatEnum = RGBA,
						GFXDataFormat   dataTypeEnum = FLOAT_32) = 0;

	virtual void Destroy() = 0;

	virtual void* Begin(U8 nFace=0) const = 0;	
	virtual void End(U8 nFace=0) const = 0;		

	virtual void Bind(U8 unit=0) const = 0;		
	virtual void Unbind(U8 unit=0) const = 0;	

	virtual void updatePixels(const F32 * const pixels) = 0;
	inline U32 getTextureHandle() const	{return _textureId;} 
	inline U16 getWidth() const			{return _width;}
	inline U16 getHeight() const		{return _height;}
	inline U16 getDepth() const		    {return _depth;}
	inline U8  getType()   const	    {return _pbotype;}
	virtual ~PixelBufferObject(){};
	PixelBufferObject(PBOType type) : _pbotype(type),
									  _textureId(0),
									  _width(0),
									  _height(0),
									  _depth(0),
									  _pixelBufferHandle(0),
									  _textureType(0){}
protected:
	U8          _pbotype;
	U32         _textureId;
	U16		    _width, _height, _depth;
	U32		    _pixelBufferHandle;
	U32		    _textureType;

};


#endif

