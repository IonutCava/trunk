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

#ifndef _D3D_PIXEL_BUFFER_OBJECT_H
#define _D3D_PIXEL_BUFFER_OBJECT_H

#include "Hardware/Video/Buffers/PixelBufferObject/Headers/PixelBufferObject.h"

class d3dPixelBufferObject : public PixelBufferObject {
public:

	d3dPixelBufferObject(PBOType type) : PixelBufferObject(type) {}
	~d3dPixelBufferObject() {Destroy();}

	bool Create(U16 width, U16 height,U16 depth = 0, 
				GFXImageFormat internalFormatEnum = RGBA8,
				GFXImageFormat formatEnum = RGBA,
				GFXDataFormat dataTypeEnum = FLOAT_32) {return true;}
				
	void Destroy() {};

	void* Begin(U8 nFace=0) const {return 0;};
	void End(U8 nFace=0) const {}

	void Bind(U8 unit=0) const {}	
	void Unbind(U8 unit=0) const {}	

	void updatePixels(const F32 * const pixels) {}

private:
	bool checkStatus() {return true;}

};

#endif

