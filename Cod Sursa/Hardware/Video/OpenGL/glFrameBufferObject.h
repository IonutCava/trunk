#ifndef _GL_FRAME_BUFFER_OBJECT_H
#define _GL_FRAME_BUFFER_OBJECT_H

#include "../FrameBufferObject.h"

class glFrameBufferObject : public FrameBufferObject
{
public:

	glFrameBufferObject();
	~glFrameBufferObject() {Destroy();}

	bool Create(FBO_TYPE type, U32 width, U32 height);
	void Destroy();

	void Begin(U32 nFace=0) const;	
	void End(U32 nFace=0) const;		

	void Bind(int unit=0) const;		
	void Unbind(int unit=0) const;	

private:
	bool checkStatus();

};

#endif

