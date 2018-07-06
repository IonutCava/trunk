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

#ifndef IMAGETOOLS_H
#define IMAGETOOLS_H

#include "resource.h"

namespace ImageTools
{
	class ImageData {
	public:
		U8*	data;
		U16 w, h;
		U8  d;
		U32 type;
		U32 ilTexture;
		std::string name;
		bool _flip;
		ivec3	getColor(U16 x, U16 y) const;

		ImageData() {w = h = d = 0; data = NULL;_flip = false;}
		~ImageData() {Destroy();}
		void Destroy();
		void resize(U16 width, U16 height);
	};
	void Flip(ImageData& image);
	void     OpenImage(const std::string& filename, ImageData& img);
	U8* OpenImage(const std::string& filename, U16& w, U16& h, U8& d, U32& t,U32& ilTexture,bool flip=false);
	U8* OpenImagePPM(const std::string& filename, U16& w, U16& h, U8& d, U32& t,U32& ilTexture,bool flip=false);
	U8* OpenImageDevIL(const std::string& filename, U16& w, U16& h, U8& d, U32& t,U32& ilTexture,bool flip=false);
}

#endif

