#ifndef _TEXT_3D_H_
#define _TEXT_3D_H_
#include "Geometry/Object3D.h"

class Text3D : public Object3D
{
public:
	Text3D(string text) : _text(text),_texture(NULL),_shader(NULL),_font(GLUT_STROKE_ROMAN){}
	~Text3D(){};
	bool load(const std::string &name) {_text = name; return true;}
	bool unload() {_text.clear(); return true;}

	inline string&    getText()    {return _text;}
	inline void*      getFont()    {return _font;}
	inline F32&       getWidth()   {return _width;}
	inline Texture2D* getTexture() {return _texture;}
	inline Shader*    getShader()  {return _shader;}
private:
	string _text;
	void* _font;
	F32   _width;
	Texture2D* _texture;
	Shader*   _shader;
};


#endif