/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

