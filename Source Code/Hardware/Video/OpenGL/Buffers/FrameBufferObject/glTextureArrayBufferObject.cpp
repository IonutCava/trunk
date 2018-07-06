#include "Headers/glTextureArrayBufferObject.h"

#include "core.h"
#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glTextureArrayBufferObject::glTextureArrayBufferObject(bool cubeMap, bool depthOnly) : glFrameBufferObject(cubeMap ? (depthOnly ? FBO_CUBE_DEPTH_ARRAY : FBO_CUBE_COLOR_ARRAY) : FBO_2D_ARRAY_COLOR){
	cubeMap ? _textureType = GL_TEXTURE_CUBE_MAP_ARRAY : _textureType = GL_TEXTURE_2D_ARRAY_EXT;
	toggleColorWrites(!depthOnly); ///<Use glDepthBufferObject instead.
}

void glTextureArrayBufferObject::DrawToLayer(GLubyte face, GLubyte layer) const {
	GLCheck(glFramebufferTextureLayerEXT(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT, _depthBufferHandle, 0, layer));
	GLCheck(glFramebufferTextureLayerEXT(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, _textureId[0], 0, layer));
}