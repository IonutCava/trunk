#ifndef _OBJECT_3D_H_
#define _OBJECT_3D_H_

#include "resource.h"
#include "Utility/Headers/BaseClasses.h"
#include "Utility/Headers/Transform.h"
#include "Utility/Headers/BoundingBox.h"
#include "Hardware/Video/GFXDevice.h"

class Object3D : public Resource
{
public:
	Object3D() : _color(0.0f,0.0f,0.0f),
				 _selected(false),
				 _update(false),
				 _name("default")
	{
		Quaternion rotation; rotation.FromEuler(0,0,-1);
		_transform = new Transform(rotation, vec3(0,0,0),vec3(1,1,1));
	};

	Object3D(const std::string& name) : _name(name),
										_color(0.0f,0.0f,0.0f),
										_selected(false),
										_update(false)
	{
		Quaternion rotation; rotation.FromEuler(0,0,-1);
		_transform = new Transform(rotation, vec3(0,0,0),vec3(1,1,1));
	}

	Object3D(const vec3& position,const vec3& scale,const vec3& orientation,const vec3& color) : _color(color),
																								 _selected(false),
																								 _name("default")
	{
		Quaternion rotation; rotation.FromEuler(orientation);
		_transform = new Transform(rotation, position,scale);
	}

	~Object3D() {delete _transform;}

	inline	vec3&						getColor()			{return _color;}
	inline	string&						getName()			{return _name;}
	inline	string&						getItemName()		{return _itemName;}
	inline	Transform*                  getTransform()		{return _transform;}
	inline	BoundingBox&				getBoundingBox()	{return _bb;}
	inline	bool                        isSelected()		{return _selected;}

	void			setSelected(bool state) {_selected = state;	if(_selected) cout << "Selected: " << getName() << endl;}
	
	virtual void    computeBoundingBox(){_originalBB = _bb;}
	virtual void	updateBBox();

protected:
	Transform*					 _transform;
	vec3						 _color;
	bool						 _selected,_update;
	string					     _name, _itemName;
	BoundingBox			         _bb, _originalBB; //_originalBB is a copy of the initialy calculate BB for transformation
												   //it should be copied in every computeBoungingBox call;
};

#endif