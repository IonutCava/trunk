/*“Copyright 2009-2011 DIVIDE-Studio”*/
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
	Texture(bool flipped = false) : Resource(), _flipped(flipped), _handle(0){}

protected:
	U32	_handle;
	U16 _width,_height;
	U8  _bitDepth;			
	bool _flipped;
	ImageTools::ImageData _img;
	static bool s_bGenerateMipmaps;	
};


#endif

