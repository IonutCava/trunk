#include "Headers/GenericShape.h"

GenericShape::GenericShape() : Object3D(GENERIC) {
};

GenericShape::~GenericShape() {
};

void GenericShape::addVertexPosition(vec3<F32> vertPosition){
	_geometry->addPosition(vertPosition);
	_geometry->getHWIndices().push_back(_geometry->getPosition().size() - 1);
	_geometry->setIndiceLimits(vec2<U16>(0,_geometry->getPosition().size() - 1));
}