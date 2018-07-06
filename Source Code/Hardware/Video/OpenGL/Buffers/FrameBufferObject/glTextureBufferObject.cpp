#include "Headers/glTextureBufferObject.h"

#include "core.h"
#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glTextureBufferObject::glTextureBufferObject(bool cubeMap, bool depthOnly) : glFrameBufferObject(cubeMap ? (depthOnly ? FBO_CUBE_DEPTH : FBO_CUBE_COLOR) : FBO_2D_COLOR){
	cubeMap ? _textureType = GL_TEXTURE_CUBE_MAP : _textureType = GL_TEXTURE_2D;
	toggleColorWrites(!depthOnly); ///<Use glDepthBufferObject instead.
}