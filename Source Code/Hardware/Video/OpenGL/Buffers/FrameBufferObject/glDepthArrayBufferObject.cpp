#include "Headers/glDepthArrayBufferObject.h"

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Headers/glFrameBufferObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glDepthArrayBufferObject::glDepthArrayBufferObject() : glFrameBufferObject(FBO_2D_ARRAY_DEPTH)
{
	if(!glewIsSupported("GL_EXT_texture_array") && !glewIsSupported("GL_MESA_texture_array"))
		ERROR_FN(Locale::get("ERROR_GL_NO_TEXTURE_ARRAY"));
	_textureType = GL_TEXTURE_2D_ARRAY_EXT;
}

void glDepthArrayBufferObject::DrawToLayer(GLubyte face, GLubyte layer) const {
	GLCheck(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthId, 0, layer));
	GLCheck(glClear( _clearBufferMask ));
}

void glDepthArrayBufferObject::Bind(GLubyte unit, GLubyte texture) const {
	FrameBufferObject::Bind(unit);
    GL_API::setActiveTextureUnit(unit);
	GLCheck(glBindTexture(_textureType, _depthId));
	if(texture > 0){ ///Hack for previewing shadow maps
		GLCheck(glTexParameteri( _textureType, GL_TEXTURE_COMPARE_MODE, GL_NONE));
	}else{
		GLCheck(glTexParameteri( _textureType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE));
	}
}