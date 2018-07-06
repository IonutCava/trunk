#include "Headers/glFrameBufferObject.h"
#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Hardware/Video/Headers/GFXDevice.h"

bool glFrameBufferObject::checkStatus() {
	//Not defined in GLEW
	#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS 0x8CDA
	#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9

    // check FBO status
    U32 status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status)
    {
    case GL_FRAMEBUFFER_COMPLETE:
        return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		ERROR_FN(Locale::get("ERROR_FBO_ATTACHMENT_INCOMPLETE"));
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        ERROR_FN(Locale::get("ERROR_FBO_NO_IMAGE"));
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        ERROR_FN(Locale::get("ERROR_FBO_DIMENSIONS"));
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
        ERROR_FN(Locale::get("ERROR_FBO_FORMAT"));
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        ERROR_FN(Locale::get("ERROR_FBO_INCOMPLETE_DRAW_BUFFER"));
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        ERROR_FN(Locale::get("ERROR_FBO_INCOMPLETE_READ_BUFFER"));
        return false;

    case GL_FRAMEBUFFER_UNSUPPORTED:
        ERROR_FN(Locale::get("ERROR_FBO_UNSUPPORTED"));
        return false;

    default:
        ERROR_FN(Locale::get("ERROR_UNKNOWN"));
        return false;
    }
}

