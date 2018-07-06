#include "Headers/glDepthBufferObject.h"

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Headers/glFrameBufferObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glDepthBufferObject::glDepthBufferObject() : glFrameBufferObject(){

	_fboType = FBO_2D_DEPTH;
	_textureType = GL_TEXTURE_2D;
	
	if(!glewIsSupported("glBindFramebuffer")){
		glBindFramebuffer = GLEW_GET_FUN(__glewBindFramebuffer);
		glDeleteFramebuffers = GLEW_GET_FUN(__glewDeleteFramebuffers);
		glFramebufferTexture2D = GLEW_GET_FUN(__glewFramebufferTexture2D);
		glGenFramebuffers   = GLEW_GET_FUN(__glewGenFramebuffers);
		glCheckFramebufferStatus = GLEW_GET_FUN(__glewCheckFramebufferStatus);
	}

	if(!glewIsSupported("glBindRenderbuffer")) {
		glDeleteRenderbuffers = GLEW_GET_FUN(__glewDeleteRenderbuffers);
		glGenRenderbuffers = GLEW_GET_FUN(__glewGenRenderbuffers);
		glBindRenderbuffer = GLEW_GET_FUN(__glewBindRenderbuffer);
		glRenderbufferStorage = GLEW_GET_FUN(__glewRenderbufferStorage);
		glFramebufferRenderbuffer = GLEW_GET_FUN(__glewFramebufferRenderbuffer);
	}

}

bool glDepthBufferObject::Create(U16 width, U16 height, IMAGE_FORMATS internalFormatEnum, 
								                        IMAGE_FORMATS formatEnum){
	
	GLenum internalFormat = glFormatTable[internalFormatEnum];
	GLenum format = glFormatTable[formatEnum];

	D_PRINT_FN(Locale::get("GL_FBO_GEN_DEPTH"),width,height);
	Destroy();
	_width = width;
	_height = height;
	_useDepthBuffer = true;

	// Texture
	GLCheck(glGenTextures( 1, &_textureId));
	GLCheck(glBindTexture(GL_TEXTURE_2D, _textureId));
	GLCheck(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _width, _height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0));
	GLCheck(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCheck(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCheck(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCheck(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE));
	GLCheck(glBindTexture(GL_TEXTURE_2D, 0));

	/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);*/
	
	GLCheck(glGenFramebuffers(1, &_frameBufferHandle));
	GLCheck(glGenRenderbuffers(1, &_depthBufferHandle));

	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));	
	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle));

	// Depth buffer
	GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height));
	GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBufferHandle));
	GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _textureId, 0));

	//  disable drawing to any buffers, we only want the depth
	GLCheck(glDrawBuffer(GL_NONE));
	GLCheck(glReadBuffer(GL_NONE));
	
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	return true;
}

void glDepthBufferObject::Destroy() {
	if(_textureType != 0) Unbind();
	if(_textureId > 0){
		/*GLCheck(*/glDeleteTextures(1, &_textureId)/*)*/;
	}
	if(_frameBufferHandle > 0){
		/*GLCheck(*/glDeleteFramebuffers(1, &_frameBufferHandle)/*)*/;
	}

	if(_depthBufferHandle > 0){
		GLCheck(glDeleteRenderbuffers(1, &_depthBufferHandle));
	}

	_width = 0;
	_height = 0;
}


void glDepthBufferObject::Begin(U8 nFace) const {
	assert(nFace<6);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, _width, _height);
 
	glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle);

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
}

void glDepthBufferObject::End(U8 nFace) const {
	assert(nFace<6);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glPopAttrib();//Viewport Bit
}

void glDepthBufferObject::Bind(U8 unit, U8 texture) {
	//if(_bound) return;
	glActiveTexture(GL_TEXTURE0 + (U32)unit);
	GLCheck(glEnable(_textureType));
	GLCheck(glBindTexture(_textureType, _textureId));
	//GLCheck(glEnable(_textureType));
	_bound = true;
}

void glDepthBufferObject::Unbind(U8 unit) {
	//if(!_bound) return; //If it's already bound on any slot, including this one
	glActiveTexture(GL_TEXTURE0 + (U32)unit);
	glBindTexture(_textureType, 0 );
	glDisable(_textureType);
	_bound = false;
}
