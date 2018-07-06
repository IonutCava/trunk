#include "Headers/GenericShape.h"

GenericShape::GenericShape() : Object3D(GENERIC) {
};

GenericShape::~GenericShape() {
};

void GenericShape::addVertexPosition(vec3<F32> vertPosition){
	_geometry->getPosition().push_back(vertPosition);
	getIndices().push_back(_geometry->getPosition().size() - 1);
}