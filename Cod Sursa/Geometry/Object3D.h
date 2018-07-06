#ifndef _OBJECT_3D_H
#define _OBJECT_3D_H
#include "resource.h"
#include "Utility/Headers/BaseClasses.h"

class Object3D : public Resource
{
public:
	~Object3D(){}
	Object3D() : _position(0,0,0),
		         _scale(1,1,1),
				 _orientation(0,0,-1),
				 _color(0.0f,0.0f,0.0f),
				 _selected(false),
				 _update(false),
				 _name("default"){};

	Object3D(const std::string& name) : _name(name),
										_position(0,0,0),
										_scale(1,1,1),
										_orientation(0,0,-1),
										_color(0.0f,0.0f,0.0f),
										_selected(false),
										_update(false){}

	Object3D(vec3& position, vec3& scale, vec3& orientation, vec3& color) : _position(position),
															   _scale(scale),
															   _orientation(orientation),
															   _color(color),
															   _selected(false),
															   _name("default") {}

	vec3&						getPosition(){return _position;}
	vec3&						getOrientation(){return _orientation;}
	vec3&						getScale(){return _scale;}
	vec3&						getColor(){return _color;}
	string&						getName(){return _name;}
	void                        setSelected(bool state) {_selected = state;	if(_selected) cout << "Selected: " << getName() << endl;}
	bool                        isSelected() {return _selected;}

protected:
	vec3 _position, _orientation, _scale,_color;
	bool _selected,_update;
	string _name;
};

#endif