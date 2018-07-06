#ifndef IMAGETOOLS_H
#define IMAGETOOLS_H

#include "resource.h"
#include "Utility/Headers/MathClasses.h"


namespace ImageTools
{
	class ImageData {
	public:
		GLubyte*	data;
		U32 w, h, d;
		GLenum type;
		string name;
		bool _flip;
		ivec3	getColor(U32 x, U32 y) const;

		ImageData() {w = h = d = 0; data = NULL;_flip = false;}
		~ImageData() {Destroy();}
		void Destroy();
	};
	void Flip(ImageData& image);
	void     OpenImage(const std::string& filename, ImageData& img);
	unsigned char* OpenImage(const std::string& filename, U32& w, U32& h, U32& d, U32& t,bool flip=false);

	unsigned char* OpenImagePPM(const std::string& filename, U32& w, U32& h, U32& d, U32& t,bool flip=false);
	unsigned char* OpenImageDevIL(const std::string& filename, U32& w, U32& h, U32& d, U32& t,bool flip=false);
}

#endif

