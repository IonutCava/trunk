#ifndef _TEXT_3D_H_
#define _TEXT_3D_H_
#include "Geometry/Object3D.h"

class Text3D : public Object3D
{
public:
	Text3D(){};
	~Text3D(){};
	bool load(const std::string &name) {_text = name; return true;}
	bool unload() {_text.clear(); return true;}
	void draw();
	string& getText() {return _text;}
private:
	string _text;
};


#endif