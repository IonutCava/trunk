#ifndef _OBJECT_3D_H_
#define _OBJECT_3D_H_

#include "resource.h"
#include "Utility/Headers/BaseClasses.h"
#include "Utility/Headers/Transform.h"
#include "Utility/Headers/BoundingBox.h"
#include "Hardware/Video/GFXDevice.h"

enum PrimitiveType
{
	SPHERE_3D,
	BOX_3D,
	QUAD_3D,
	TEXT_3D,
	MESH,
	SUBMESH,
};

class BoundingBox;
class Object3D : public Resource
{
public:
	Object3D() : _color(0.0f,0.0f,0.0f),
				 _selected(false),
				 _update(false),
				 _name("default"),
				_render(true),
				_transform(NULL)

	{
	}

	Object3D(const std::string& name) : _name(name),
									_color(0.0f,0.0f,0.0f),
									_selected(false),
									_update(false),
									_render(true),
									_transform(NULL)
	{
	}

	Object3D(const vec3& position,const vec3& scale,const vec3& orientation,const vec3& color) : _color(color),
																								 _selected(false),
																								 _name("default"),
																								 _render(true)
	{
		Quaternion rotation; rotation.FromEuler(orientation);
		_transform = New Transform(rotation, position,scale);
	}

	Object3D(const Object3D& old);
	~Object3D() {delete _transform;}

	inline	vec3&						getColor()		      {return _color;}
	inline	std::string&				getName()		 	  {return _name;}
	inline	std::string&				getItemName()		  {return _itemName;}
	inline	BoundingBox&				getBoundingBox()	  {return _bb;}
	inline  PrimitiveType               getType()       const {return _geometryType;}
	inline  bool                        getVisibility() const {return _render;}
	inline	bool                        isSelected()	      {return _selected;}

	Transform*      const getTransform();

	void			setSelected(bool state)					        {_selected = state;}
	void            setVisibility(bool state)				        {_render = state;}

	virtual inline  std::vector<Shader* >&   getShaders()			{return _shaders; }
	virtual                          void    addShader(Shader* s)   {_shaders.push_back(s);}
	virtual							 void    computeBoundingBox()   {_originalBB = _bb;}
	virtual							 void    onDraw();
	virtual                          void    drawBBox();

protected:
	Transform*					 _transform;
	vec3						 _color;
	bool						 _selected,_update, _render, _drawBB;
	std::string				     _name, _itemName;
	BoundingBox			         _bb, _originalBB; //_originalBB is a copy of the initialy calculate BB for transformation
												   //it should be copied in every computeBoungingBox call;
	PrimitiveType				_geometryType;
	std::vector<Shader*>		_shaders;
};

#endif