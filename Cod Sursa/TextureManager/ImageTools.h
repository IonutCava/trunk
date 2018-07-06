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
		std::string name;
		bool _flip;
		ivec3	getColor(U16 x, U16 y) const;

		ImageData() {w = h = d = 0; data = NULL;_flip = false;}
		~ImageData() {Destroy();}
		void Destroy();
	};
	void Flip(ImageData& image);
	void     OpenImage(const std::string& filename, ImageData& img);
	U8* OpenImage(const std::string& filename, U16& w, U16& h, U8& d, U32& t,bool flip=false);

	U8* OpenImagePPM(const std::string& filename, U16& w, U16& h, U8& d, U32& t,bool flip=false);
	U8* OpenImageDevIL(const std::string& filename, U16& w, U16& h, U8& d, U32& t,bool flip=false);
}

#endif

