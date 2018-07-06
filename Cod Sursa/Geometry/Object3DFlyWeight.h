#ifndef _OBJECT_3D_FLYWEIGHT_H_
#define _OBJECT_3D_FLYWEIGHT_H_
#include "resource.h"
#include "Object3D.h"

class Object3DFlyWeight
{

public:
	Object3DFlyWeight(Object3D* referenceObject, Transform* realTransform):
			_reference(referenceObject),
			_realTransform(realTransform)
	{
	}

	Object3D* getObject() {return _reference;}
	Transform* getTransform() {return _realTransform;}

	void setTransform(Transform* transform){delete _realTransform; _realTransform = transform;}
	void setObject(Object3D* object){delete _reference; _reference = object;}

private:
	Object3D*  _reference;
	Transform* _realTransform;
	
	
};

#endif