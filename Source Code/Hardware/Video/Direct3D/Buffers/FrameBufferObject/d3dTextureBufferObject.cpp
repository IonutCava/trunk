#include "Headers/d3dTextureBufferObject.h"

d3dTextureBufferObject::d3dTextureBufferObject(bool cubeMap) : d3dFrameBufferObject() {
	cubeMap ? _fboType = FBO_CUBE_COLOR : _fboType = FBO_2D_COLOR;
}