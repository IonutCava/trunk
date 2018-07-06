#ifndef _BOX_3D_H_
#define _BOX_3D_H_
#include "Geometry/Object3D.h"

class Box3D : public Object3D
{
public:
	Box3D(F32 size) : _size(size),_texture(NULL),_shader(NULL) {};
	~Box3D(){};
	bool load(const std::string &name) {return true;}
	bool unload() {return true;}

	F32&              getSize()    {return _size;}
	inline Texture2D* getTexture() {return _texture;}
	inline Shader*    getShader()  {return _shader;}
private:
	F32 _size;
	Texture2D* _texture;
	Shader*   _shader;
};


#endif