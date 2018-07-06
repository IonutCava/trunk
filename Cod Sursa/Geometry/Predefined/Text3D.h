#ifndef _TEXT_3D_H_
#define _TEXT_3D_H_
#include "Geometry/Object3D.h"

class Text3D : public Object3D
{
public:
	Text3D(string text) : _text(text) {_font = GLUT_STROKE_ROMAN;}
	~Text3D(){};
	bool load(const std::string &name) {_text = name; return true;}
	bool unload() {_text.clear(); return true;}

	inline string& getText()  {return _text;}
	inline void*   getFont()  {return _font;}
	inline F32&    getWidth() {return _width;}
private:
	string _text;
	void* _font;
	F32   _width;
};


#endif