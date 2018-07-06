#ifndef _TEXT_3D_H_
#define _TEXT_3D_H_
#include "Geometry/Object3D.h"

class Text3D : public Object3D
{
public:
	Text3D(){};
	~Text3D(){};
private:
	string _text;
};


#endif