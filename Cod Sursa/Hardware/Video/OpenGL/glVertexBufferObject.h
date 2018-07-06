#ifndef _GL_VERTEX_BUFFER_OBJECT_H
#define _GL_VERTEX_BUFFER_OBJECT_H

#include "../VertexBufferObject.h"

class glVertexBufferObject : public VertexBufferObject
{

public:

	bool		Create(U32 usage);
	void		Destroy();

	
	void		Enable();
	void		Disable();

	
	inline vector<vec3>&	getPosition()	{return _dataPosition;}
	inline vector<vec3>&	getNormal()		{return _dataNormal;}
	inline vector<vec2>&	getTexcoord()	{return _dataTexcoord;}
	inline vector<vec3>&	getTangent()	{return _dataTangent;}

	glVertexBufferObject();
	~glVertexBufferObject() {Destroy();}

private:
	void Enable_VA();	
	void Enable_VBO();	
	void Disable_VA();	
	void Disable_VBO();

};

#endif

