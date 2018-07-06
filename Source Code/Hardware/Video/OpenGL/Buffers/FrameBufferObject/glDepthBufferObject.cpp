#include "Headers/glDepthBufferObject.h"

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Headers/glFrameBufferObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glDepthBufferObject::glDepthBufferObject() : glFrameBufferObject(FBO_2D_DEPTH){

	_textureType = GL_TEXTURE_2D;
}