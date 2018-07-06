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

		ivec3	getColor(U32 x, U32 y) const;

		ImageData() {w = h = d = 0; data = NULL;}
		~ImageData() {Destroy();}
		void Destroy();
	};

	void     OpenImage(const std::string& filename, ImageData& img);
	GLubyte* OpenImage(const std::string& filename, U32& w, U32& h, U32& d,bool terrain = false);

	GLubyte* OpenImagePPM(const std::string& filename, U32& w, U32& h, U32& d);
	GLubyte* OpenImageDevIL(const std::string& filename, U32& w, U32& h, U32& d);
}

#endif

