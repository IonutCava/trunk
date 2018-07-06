#ifndef _GL_PIXEL_BUFFER_OBJECT_H
#define _GL_PIXEL_BUFFER_OBJECT_H

#include "../PixelBufferObject.h"

class glPixelBufferObject : public PixelBufferObject
{
public:

	glPixelBufferObject();
	~glPixelBufferObject() {Destroy();}

	bool Create(U16 width, U16 height);
				
	void Destroy();

	void* Begin(U8 nFace=0) const;	
	void End(U8 nFace=0) const;		

	void Bind(U8 unit=0) const;		
	void Unbind(U8 unit=0) const;	

	void updatePixels(const F32 * const pixels);

private:
	bool checkStatus();

};

#endif

