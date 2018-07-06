#include "Headers/d3dTextureBufferObject.h"

d3dTextureBufferObject::d3dTextureBufferObject(bool cubeMap) : d3dFrameBufferObject(cubeMap ? FBO_CUBE_COLOR : FBO_2D_COLOR) {
}