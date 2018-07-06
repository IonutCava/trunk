#include "Headers/glDeferredBufferObject.h"

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Headers/glFrameBufferObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glDeferredBufferObject::glDeferredBufferObject() : glFrameBufferObject(),
												   _normalBufferHandle(0),
												   _positionBufferHandle(0),
												   _diffuseBufferHandle(0){
	_fboType = FBO_2D_DEFERRED;
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

bool glDeferredBufferObject::Create(U16 width, U16 height, IMAGE_FORMATS internalFormatEnum, IMAGE_FORMATS formatEnum){
	
	GLenum internalFormat = glFormatTable[internalFormatEnum];
	GLenum format = glFormatTable[formatEnum];
	
	D_PRINT_FN(Locale::get("GL_FBO_GEN_DEFERRED"),width,height);
	Destroy();
	_width = width;
	_height = height;
	_useDepthBuffer = true;
	_textureType = GL_TEXTURE_2D;
	U32 textureId;


	GLCheck(glGenFramebuffers(1, &_frameBufferHandle));
	GLCheck(glGenRenderbuffers(1, &_depthBufferHandle));
	GLCheck(glGenFramebuffers(1, &_diffuseBufferHandle));
	GLCheck(glGenRenderbuffers(1, &_positionBufferHandle));
	GLCheck(glGenRenderbuffers(1, &_normalBufferHandle));
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));

	//R8G8B8A8, 32bit format for diffuse
	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _diffuseBufferHandle));
	GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width,  height));
	GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER, _diffuseBufferHandle));
	//R32G32B32A32, HDR 128bit format for position data
	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _positionBufferHandle));
	GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, width, height));
	GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_RENDERBUFFER, _positionBufferHandle));
	//R16G16B16A16, 64bit format for normals
	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _normalBufferHandle));
	GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA16F, width,height));
	GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT2,GL_RENDERBUFFER, _normalBufferHandle));

	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle));
	GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,width, height));
	GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER, _depthBufferHandle));
	
	GLCheck(glGenTextures(1, &textureId));
	GLCheck(glBindTexture(GL_TEXTURE_2D, textureId));
	GLCheck(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,GL_RGBA, GL_UNSIGNED_BYTE, NULL));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, textureId, 0));
	_textureIDs.push_back(textureId);

	GLCheck(glGenTextures(1, &textureId));
	GLCheck(glBindTexture(GL_TEXTURE_2D, textureId));
	GLCheck(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D, textureId, 0));
	_textureIDs.push_back(textureId);

	GLCheck(glGenTextures(1, &textureId));
	GLCheck(glBindTexture(GL_TEXTURE_2D, textureId));
	GLCheck(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,GL_RGBA, GL_FLOAT, NULL));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT2,GL_TEXTURE_2D, textureId, 0));
	_textureIDs.push_back(textureId); 


	return true;
}

void glDeferredBufferObject::Destroy(){
	GLCheck(glDeleteFramebuffers(1, &_normalBufferHandle));
	GLCheck(glDeleteFramebuffers(1, &_positionBufferHandle));
	GLCheck(glDeleteFramebuffers(1, &_diffuseBufferHandle));
	if(_textureType != 0) Unbind();
	if(!_textureIDs.empty() && _textureIDs[0] > 0){
		/*GLCheck(*/glDeleteTextures(_textureIDs.size(), &_textureIDs[0])/*)*/;
	}
	if(_frameBufferHandle > 0){
		/*GLCheck(*/glDeleteFramebuffers(1, &_frameBufferHandle)/*)*/;
	}

	if(_useDepthBuffer && _depthBufferHandle > 0){
		GLCheck(glDeleteRenderbuffers(1, &_depthBufferHandle));
	}

	_width = 0;
	_height = 0;
}

void glDeferredBufferObject::Begin(U8 nFace) const {
	assert(nFace<6);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, _width, _height);
 
	glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle);
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	GLCheck(glDrawBuffers(3, buffers));
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
}

void glDeferredBufferObject::End(U8 nFace) const {
	assert(nFace<6);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glPopAttrib();//Viewport Bit
}

void glDeferredBufferObject::Bind(U8 unit, U8 texture) {
	//if(_bound) return;
	glActiveTexture(GL_TEXTURE0 + (U32)unit);
	GLCheck(glEnable(_textureType));
	GLCheck(glBindTexture(_textureType, _textureIDs[texture]));
	//GLCheck(glEnable(_textureType));
	_bound = true;
}

void glDeferredBufferObject::Unbind(U8 unit) {
	//if(!_bound) return; //If it's already bound on any slot, including this one
	glActiveTexture(GL_TEXTURE0 + (U32)unit);
	glBindTexture(_textureType, 0 );
	glDisable(_textureType);
	_bound = false;
}
