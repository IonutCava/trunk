#ifndef _QUAD_3D_H_
#define _QUAD_3D_H_
#include "Geometry/Object3D.h"

class Shader;
class Quad3D : public Object3D
{
public:
	Quad3D(const vec3& topLeft,const  vec3& topRight,
		   const  vec3& bottomLeft,const  vec3& bottomRight) : _tl(topLeft),
		    												   _tr(topRight),
															   _bl(bottomLeft),
															   _br(bottomRight)
															   {_geometryType = QUAD_3D;}
	Quad3D() : _tl(vec3(1,1,0)),
			   _tr(vec3(-1,0,0)),
			   _bl(vec3(0,-1,0)),
			   _br(vec3(-1,-1,0)) {}

	bool load(const std::string &name) {_name = name; return true;}

	enum CornerLocation{
		TOP_LEFT,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT
	} ;

	vec3& getCorner(CornerLocation corner)
	{
		switch(corner){
			case TOP_LEFT: return _tl;
			case TOP_RIGHT: return _tr;
			case BOTTOM_LEFT: return _bl;
			case BOTTOM_RIGHT: return _br;
			default: break;
		}
		return _tl; //default returns top left corner. Why? Don't care ... seems like a good idea. - Ionut
	}
	
	void computeBoundingBox() {_bb.setMin(_bl); _bb.setMax(_tr); _bb.isComputed() = true; _originalBB = _bb;}
public: 



private:
	vec3 _tl,_tr,_bl,_br;
};


#endif