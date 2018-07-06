#ifndef _SPHERE_3D_H_
#define _SPHERE_3D_H_
#include "Geometry/Object3D.h"

class Sphere3D : public Object3D
{
public:
	//Size is radius
	Sphere3D(F32 size,U32 resolution) : _size(size), _resolution(resolution),_texture(NULL),_shader(NULL) {};
	~Sphere3D(){};
	bool load(const std::string &name) {return true;}
	bool unload() {return true;}

	F32&			  getSize()       {return _size;}
	U32&			  getResolution() {return _resolution;}
	inline Texture2D* getTexture()    {return _texture;}
	inline Shader*    getShader()     {return _shader;}

protected:
	F32 _size;
	U32 _resolution;
	Texture2D* _texture;
	Shader*   _shader;
	
};


#endif