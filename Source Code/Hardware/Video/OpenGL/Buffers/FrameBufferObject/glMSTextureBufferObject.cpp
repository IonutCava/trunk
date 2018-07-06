#include "Headers/glMSTextureBufferObject.h"

#include "core.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glMSTextureBufferObject::glMSTextureBufferObject() : glFrameBufferObject(FBO_2D_COLOR_MS),
													 _msaaBufferResolver(0),
													 _colorBufferHandle(0)
{
	_msaaSamples = ParamHandler::getInstance().getParam<U8>("rendering.FSAAsamples",2);
	_textureType = GL_TEXTURE_2D;
}

bool glMSTextureBufferObject::Create(GLushort width, GLushort height, GLubyte imageLayers){

	if(!_attachementDirty[TextureDescriptor::Color0]){
		return true;
	}
	
	D_PRINT_FN(Locale::get("GL_FBO_GEN_COLOR_MS"),width,height);
	Destroy();
	TextureDescriptor texDescriptor = _attachement[TextureDescriptor::Color0];
	///And get the image formats and data type
	if(_textureType != glTextureTypeTable[texDescriptor._type]){
		ERROR_FN(Locale::get("ERROR_FBO_ATTACHEMENT_DIFFERENT"), (I32)TextureDescriptor::Color0);
	}
	if(!_attachementDirty[TextureDescriptor::Color0]) return true;

	if(_msaaBufferResolver <= 0){
		// create a new fbo with multisampled color and depth attachements
		GLCheck(glGenFramebuffers(1, &_msaaBufferResolver));
		GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _msaaBufferResolver));	
	}
	
	GLenum format = glImageFormatTable[texDescriptor._format];
	GLenum internalFormat = glImageFormatTable[texDescriptor._internalFormat];
	GLenum dataType = glDataFormat[texDescriptor._dataType];

	_width = width;
	_height = height;
	_useDepthBuffer = true;

	GLCheck(glGenTextures(1, &_textureId[0]));
	GLCheck(glBindTexture(_textureType, _textureId[0]));

	///General texture parameters for either color or depth 
	if(texDescriptor._generateMipMaps){
		///(depth doesn't need mipmaps, but no need for another "if" to complicate things)
		GLCheck(glTexParameteri(_textureType, GL_TEXTURE_BASE_LEVEL, texDescriptor._mipMinLevel));
		GLCheck(glTexParameteri(_textureType, GL_TEXTURE_MAX_LEVEL,  texDescriptor._mipMaxLevel));
	}
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[texDescriptor._magFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[texDescriptor._minFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S,     glWrapTable[texDescriptor._wrapU]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T,     glWrapTable[texDescriptor._wrapV]));

	GLCheck(glTexImage2D(_textureType, 
						 0,
						 internalFormat,
						_width,
						_height,
						0,
						format,
						dataType,
						NULL));

	//create a resolver buffer for rendering
	GLCheck(glGenFramebuffers(1, &_msaaBufferResolver));
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _msaaBufferResolver));
	GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,
	                               GL_COLOR_ATTACHMENT0,
								   _textureType,
								   _textureId[TextureDescriptor::Color0],
								   0));

	GLCheck(glBindTexture(_textureType, 0));

// create a new fbo with multisampled color and depth attachements
	GLCheck(glGenFramebuffers(1, &_frameBufferHandle));
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));
	GLint samples = 0;

	GLCheck(glGenRenderbuffers(1, &_colorBufferHandle));
    GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _colorBufferHandle));
    GLCheck(glRenderbufferStorageMultisample(GL_RENDERBUFFER, _msaaSamples, internalFormat, _width, _height));

    GLCheck(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorBufferHandle));
    // Create the multisample depth render buffer image and attach it to the
    // second FBO.
    GLCheck(glGenRenderbuffers(1, &_depthBufferHandle));
    GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle));
    GLCheck(glRenderbufferStorageMultisample(GL_RENDERBUFFER, _msaaSamples, GL_DEPTH_COMPONENT, _width, _height));
    GLCheck(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBufferHandle));
	checkStatus();
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	return true;
}

void glMSTextureBufferObject::Destroy() {
	glFrameBufferObject::Destroy();

	if(_msaaBufferResolver > 0){
		GLCheck(glDeleteFramebuffers(1, &_msaaBufferResolver));
		_msaaBufferResolver = 0;
	}
	if (_colorBufferHandle > 0) {
        GLCheck(glDeleteRenderbuffersEXT(1, &_colorBufferHandle));
        _colorBufferHandle = 0;
    }
}

void glMSTextureBufferObject::End(GLubyte nFace) const {
	assert(nFace<6);
	GLCheck(glPopAttrib());//Viewport Bit
	GLCheck(glBindFramebuffer(GL_READ_FRAMEBUFFER, _frameBufferHandle));
	GLCheck(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _msaaBufferResolver));
	GLCheck(glBlitFramebuffer(0, 0, _width, _height,
				              0, 0, _width, _height,
						      GL_COLOR_BUFFER_BIT, GL_NEAREST));
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	
}





