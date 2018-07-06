#ifndef _VERTEX_BUFFER_OBJECT_H
#define _VERTEX_BUFFER_OBJECT_H

#include <iostream>
#include "Utility/Headers/MathClasses.h"
#include "resource.h"

class VertexBufferObject
{
public:
	virtual bool Create() = 0;
	virtual bool Create(U32 usage) = 0;
	virtual void Destroy() = 0;
	
	virtual void Enable() = 0;
	virtual void Disable() = 0;

	
	inline std::vector<vec3>&	getPosition()	{return _dataPosition;}
	inline std::vector<vec3>&	getNormal()		{return _dataNormal;}
	inline std::vector<vec2>&	getTexcoord()	{return _dataTexcoord;}
	inline std::vector<vec3>&	getTangent()	{return _dataTangent;}

	virtual ~VertexBufferObject(){};

protected:
	virtual void Enable_VA() = 0;	
	virtual void Enable_VBO() = 0;	
	virtual void Disable_VA() = 0;	
	virtual void Disable_VBO() = 0;

protected:
	
	U32  		_VBOid;
	ptrdiff_t	_VBOoffsetPosition;
	ptrdiff_t	_VBOoffsetNormal;
	ptrdiff_t	_VBOoffsetTexcoord;
	ptrdiff_t	_VBOoffsetTangent;

	
	std::vector<vec3>	_dataPosition;
	std::vector<vec3>	_dataNormal;
	std::vector<vec2>	_dataTexcoord;
	std::vector<vec3>	_dataTangent;
};

#endif

