#include "Headers/glFrameBufferObject.h"

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glTextureBufferObject::glTextureBufferObject(bool cubeMap) : glFrameBufferObject() {

	if(cubeMap){
		_fboType = FBO_CUBE_COLOR;
		_textureType = GL_TEXTURE_CUBE_MAP;
	}else{
		_fboType = FBO_2D_COLOR;
		_textureType = GL_TEXTURE_2D;
	}
	
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

bool glTextureBufferObject::Create(U16 width, U16 height, IMAGE_FORMATS internalFormatEnum, 
								                          IMAGE_FORMATS formatEnum){

	GLenum internalFormat = glFormatTable[internalFormatEnum];
	GLenum format = glFormatTable[formatEnum];

	D_PRINT_FN(Locale::get("GL_FBO_GEN_COLOR"),width,height);
	Destroy();
	_width = width;
	_height = height;
	_useDepthBuffer = true;

	// Texture
	GLCheck(glGenTextures( 1, &_textureId ));
	GLCheck(glBindTexture(_textureType, _textureId));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

	U32 eTarget;
	U8 nFrames;
	if(_fboType != FBO_CUBE_COLOR) {
		eTarget = GL_TEXTURE_2D;
		nFrames = 1;
	} else {
		eTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		nFrames = 6;
	}
	for(U8 i=0; i<nFrames; i++){
		GLCheck(glTexImage2D(eTarget+i, 0, internalFormat, _width, _height, 0, format, GL_UNSIGNED_BYTE, 0));
	}

	// Frame buffer
	GLCheck(glGenFramebuffers(1, &_frameBufferHandle));
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));
	if(_useDepthBuffer /*&& format!=GL_DEPTH_COMPONENT*/)
	{
		// Depth buffer
		GLCheck(glGenRenderbuffers(1, &_depthBufferHandle));
		GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle));
		GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height));
		// attach framebuffer to renderbuffer
		GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBufferHandle));
	}
	
	//  attach the framebuffer to our texture, which may be a depth texture
	GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, eTarget, _textureId, 0));
	
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	return true;
}

void glTextureBufferObject::Destroy() {
	if(_textureType != 0) Unbind();
	if(_textureId > 0){
		/*GLCheck(*/glDeleteTextures(1, &_textureId)/*)*/;
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


void glTextureBufferObject::Begin(U8 nFace) const {
	assert(nFace<6);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, _width, _height);
 
	glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle);

	if(_textureType == GL_TEXTURE_CUBE_MAP) {
		GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X+nFace, _textureId, 0));
	}

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
}

void glTextureBufferObject::End(U8 nFace) const {
	assert(nFace<6);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glPopAttrib();//Viewport Bit
}

void glTextureBufferObject::Bind(U8 unit, U8 texture) {
	//if(_bound) return;
	glActiveTexture(GL_TEXTURE0 + (U32)unit);
	GLCheck(glEnable(_textureType));
	GLCheck(glBindTexture(_textureType, _textureId));
	//GLCheck(glEnable(_textureType));
	_bound = true;
}

void glTextureBufferObject::Unbind(U8 unit) {
	//if(!_bound) return; //If it's already bound on any slot, including this one
	glActiveTexture(GL_TEXTURE0 + (U32)unit);
	glBindTexture(_textureType, 0 );
	glDisable(_textureType);
	_bound = false;
}






