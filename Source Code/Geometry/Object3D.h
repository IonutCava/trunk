#ifndef _OBJECT_3D_H_
#define _OBJECT_3D_H_

#include "resource.h"
#include "Material/Material.h"
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
	OBJECT_3D_FLYWEIGHT
};

class BoundingBox;

class Object3D : public Resource
{
public:
	Object3D() : _selected(false),
				 _update(false),
				_render(true),
				_computedLightShaders(false),
				_transform(NULL)

	{
	}

	Object3D(const std::string& name) : Resource(name),
									_selected(false),
									_update(false),
									_render(true),
									_computedLightShaders(false),
									_transform(NULL)
	{
	}

	Object3D(const vec3& position,const vec3& scale,const vec3& orientation,const Material& mat) : _material(mat),
																								   _selected(false),
																								   _render(true),
																								   _computedLightShaders(false)
	{
		Quaternion rotation; rotation.FromEuler(orientation);
		_transform = New Transform(rotation, position,scale);
	}

	Object3D(const Object3D& old);
	virtual ~Object3D();

	inline	Material&					getMaterial()		  {return _material;}
	inline  PrimitiveType               getType()       const {return _geometryType;}
	virtual inline  bool                getVisibility() const {return _render;}
	inline	bool                        isSelected()	      {return _selected;}
	inline  const   BoundingBox&    	getOriginalBB()		  {return _originalBB;}
	virtual inline	BoundingBox&		getBoundingBox()	  {if(!_bb.isComputed()) computeBoundingBox(); _bb.Transform(_originalBB,getTransform()->getMatrix()); return _bb;}
			

	Transform*      const getTransform();

	void			setSelected(bool state)					        {_selected = state;}
	void            setVisibility(bool state)				        {_render = state;}
			inline std::vector<Object3D*>      getChildren()        {return _children;}                           
	
	void    addChild(Object3D* object);
	mat4&   getParentMatrix() {return _parentMatrix;}
	void    setParentMatrix(const mat4& parentMatrix) {_parentMatrix = parentMatrix;}

	virtual							 void    computeBoundingBox()   = 0;
	virtual							 void    onDraw();
	virtual                          void    drawBBox();
	virtual							 bool    unload();
	//! Use clear materials to draw only the basic primitive through the fixed pipeline and with no textures
	//! Useful for deferred rendering - Ionut
	virtual                          void    clearMaterials();

	
protected:
	Transform*					 _transform;
	Material				     _material;
	bool						 _selected,_update, _render, _drawBB,_computedLightShaders;
	BoundingBox			         _bb, _originalBB; //_originalBB is a copy of the initialy calculate BB for transformation
												   //it should be copied in every computeBoungingBox call;
	PrimitiveType				_geometryType;
	std::vector<Object3D* >     _children;
	mat4	                    _parentMatrix;
};

#endif