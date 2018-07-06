/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef IMAGETOOLS_H
#define IMAGETOOLS_H

#include "core.h"

namespace ImageTools {
	class ImageData {
	public:
		U8*	data;
		U16 w, h;
		U8  d;
		U32 type;
		U32 ilTexture;
		std::string name;
		bool _flip;
		vec3<I32>	getColor(U16 x, U16 y) const;

		ImageData() {w = h = d = 0; data = NULL;_flip = false;}
		~ImageData() {Destroy();}
		void Destroy();
		void resize(U16 width, U16 height);
	};

	void Flip(ImageData& image);
	void OpenImage(const std::string& filename, ImageData& img, bool& alpha);
	U8* OpenImage(const std::string& filename, U16& w, U16& h, U8& d, U32& t,U32& ilTexture, bool& alpha,const bool flip=false);
	U8* OpenImagePPM(const std::string& filename, U16& w, U16& h, U8& d, U32& t,U32& ilTexture,const bool flip=false);
	U8* OpenImageDevIL(const std::string& filename, U16& w, U16& h, U8& d, U32& t,U32& ilTexture,bool& alpha,const  bool flip=false);
	I8 saveToTGA(char *filename, U16 width, U16 height, U8 pixelDepth, U8 *imageData);
	I8 SaveSeries(char *filename, U16 width,U16 height,U8 pixelDepth,U8 *imageData);
	static SharedLock _loadingMutex;
}

#endif
