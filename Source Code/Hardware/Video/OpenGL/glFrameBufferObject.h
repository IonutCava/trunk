#ifndef _GL_FRAME_BUFFER_OBJECT_H
#define _GL_FRAME_BUFFER_OBJECT_H

#include "../FrameBufferObject.h"

class glFrameBufferObject : public FrameBufferObject
{
public:

	glFrameBufferObject();
	~glFrameBufferObject() {Destroy();}

	bool Create(FBO_TYPE type, U16 width, U16 height);
				
	void Destroy();

	void Begin(U8 nFace=0) const;	
	void End(U8 nFace=0) const;		

	void Bind(U8 unit=0, U8 texture = 0) const;		
	void Unbind(U8 unit=0) const;	

private:
	bool checkStatus();

};

#endif

