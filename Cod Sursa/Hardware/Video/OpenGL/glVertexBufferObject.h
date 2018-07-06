#ifndef _GL_VERTEX_BUFFER_OBJECT_H
#define _GL_VERTEX_BUFFER_OBJECT_H

#include "../VertexBufferObject.h"

class glVertexBufferObject : public VertexBufferObject
{

public:
	bool        Create();
	bool		Create(U32 usage);

	void		Destroy();

	
	void		Enable();
	void		Disable();

	
	glVertexBufferObject();
	~glVertexBufferObject() {Destroy();}

private:
	void Enable_VA();	
	void Enable_VBO();	
	void Disable_VA();	
	void Disable_VBO();

private:
	bool _created; //VBO's can be auto-created as GL_STATIC_DRAW if Enable() is called before Create();
				   //This helps with multi-threaded asset loading without creating separate GL contexts for each thread

};

#endif

