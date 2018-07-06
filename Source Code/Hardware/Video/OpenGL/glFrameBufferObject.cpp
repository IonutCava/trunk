#include "glResources.h"
#include "resource.h"
#include "glFrameBufferObject.h"
#include "Hardware/Video/GFXDevice.h"

glFrameBufferObject::glFrameBufferObject() {
	if(!glewIsSupported("glBindFramebuffer")){
		glBindFramebuffer = GLEW_GET_FUN(__glewBindFramebufferEXT);
		glDeleteFramebuffers = GLEW_GET_FUN(__glewDeleteFramebuffersEXT);
		glFramebufferTexture2D = GLEW_GET_FUN(__glewFramebufferTexture2DEXT);
		glGenFramebuffers   = GLEW_GET_FUN(__glewGenFramebuffersEXT);
		glCheckFramebufferStatus = GLEW_GET_FUN(__glewCheckFramebufferStatusEXT);
	}

	if(!glewIsSupported("glBindRenderbuffer")) {
		glDeleteRenderbuffers = GLEW_GET_FUN(__glewDeleteRenderbuffersEXT);
		glGenRenderbuffers = GLEW_GET_FUN(__glewGenRenderbuffersEXT);
		glBindRenderbuffer = GLEW_GET_FUN(__glewBindRenderbufferEXT);
		glRenderbufferStorage = GLEW_GET_FUN(__glewRenderbufferStorageEXT);
		glFramebufferRenderbuffer = GLEW_GET_FUN(__glewFramebufferRenderbufferEXT);
	}

	_frameBufferHandle=0;
	_depthBufferHandle=0;
	_diffuseBufferHandle=0;
	_positionBufferHandle=0;
	_normalBufferHandle=0;
	_width = 0;
	_height = 0;
	_useFBO = true;
	_bound = false;
}

void glFrameBufferObject::Destroy() {
	Unbind();
	if(!_textureId.empty()){
		GLCheck(glDeleteTextures(_textureId.size(), &_textureId[0]));
	}
	GLCheck(glDeleteFramebuffers(1, &_frameBufferHandle));

	if(_useDepthBuffer){
		GLCheck(glDeleteRenderbuffers(1, &_depthBufferHandle));
	}

	if(_fboType == FBO_2D_DEFERRED)	{
		GLCheck(glDeleteFramebuffers(1, &_diffuseBufferHandle));
		GLCheck(glDeleteFramebuffers(1, &_positionBufferHandle));
		GLCheck(glDeleteFramebuffers(1, &_normalBufferHandle));
	}
	_textureId.clear();
	_width = 0;
	_height = 0;
	_useFBO = true;
}


void glFrameBufferObject::Begin(U8 nFace) const {
	assert(nFace<6);
	GLCheck(glPushAttrib(GL_VIEWPORT_BIT));
	GLCheck(glViewport(0, 0, _width, _height));
	GLCheck(glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ));
	GLCheck(glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ));

		if(_useFBO) {
			GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));
			if(_fboType == FBO_2D_DEFERRED){
				GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,  GL_COLOR_ATTACHMENT2 };
				GLCheck(glDrawBuffers(3, buffers));
			}
			else if(_textureType == GL_TEXTURE_CUBE_MAP) {
				GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, _attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X+nFace, _textureId[0], 0));
			}

		}else {
			GLCheck(glPushAttrib(GL_COLOR_BUFFER_BIT | GL_PIXEL_MODE_BIT));
			GLCheck(glDrawBuffer(GL_BACK));
			GLCheck(glReadBuffer(GL_BACK));
		}
	
}

void glFrameBufferObject::End(U8 nFace) const {
	assert(nFace<6);
	if(_useFBO) {
		GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
	else {
        GLCheck(glBindTexture(_textureType, _textureId[0]));
        GLCheck(glCopyTexSubImage2D(_textureType, 0, 0, 0, 0, 0, _width, _height));
        GLCheck(glBindTexture(_textureType, 0));

        GLCheck(glPopAttrib());	//Color and Pixel Mode bits
	}
	GLCheck(glPopAttrib());//Viewport Bit
}

void glFrameBufferObject::Bind(U8 unit, U8 texture) {
	//if(_bound) return;
	GLCheck(glActiveTexture(GL_TEXTURE0 + unit));
	GLCheck(glEnable(_textureType));
	GLCheck(glBindTexture(_textureType, _textureId[texture]));
	_bound = true;
}

void glFrameBufferObject::Unbind(U8 unit) {
	//if(!_bound) return; //If it's already bound on any slot, including this one
	GLCheck(glActiveTexture(GL_TEXTURE0 + unit));
	GLCheck(glBindTexture(_textureType, 0 ));
	GLCheck(glDisable(_textureType));
	_bound = false;
}


bool glFrameBufferObject::Create(FBO_TYPE type, U16 width, U16 height){
	Destroy();
	Console::getInstance().printfn("Generating framebuffer of dimmensions [%d x %d]",width,height);
	_width = width;
	_height = height;
	_useFBO = true;
	_useDepthBuffer = true;
	_fboType = type;
	_textureType = type == FBO_CUBE_COLOR? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
	U32 textureId;

	if(type == FBO_2D_DEFERRED) //3 textures in one render target
	{
        GLCheck(glGenFramebuffers(1, &_frameBufferHandle));
		GLCheck(glGenRenderbuffers(1, &_depthBufferHandle));
		GLCheck(glGenFramebuffers(1, &_diffuseBufferHandle));
		GLCheck(glGenRenderbuffers(1, &_positionBufferHandle));
		GLCheck(glGenRenderbuffers(1, &_normalBufferHandle));
		GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));

		GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _diffuseBufferHandle));
		GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, width,  height));
		GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER, _diffuseBufferHandle));

		GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _positionBufferHandle));
		GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB32F, width, height));
		GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_RENDERBUFFER, _positionBufferHandle));

	    GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _normalBufferHandle));
		GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB16F, width,height));
		GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT2,GL_RENDERBUFFER, _normalBufferHandle));

		GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle));
		GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,width, height));
		GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER, _depthBufferHandle));
	
		GLCheck(glGenTextures(1, &textureId));
		GLCheck(glBindTexture(GL_TEXTURE_2D, textureId));
		GLCheck(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,GL_RGB, GL_UNSIGNED_BYTE, NULL));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, textureId, 0));
		_textureId.push_back(textureId);

		GLCheck(glGenTextures(1, &textureId));
		GLCheck(glBindTexture(GL_TEXTURE_2D, textureId));
		GLCheck(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, NULL));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D, textureId, 0));
		_textureId.push_back(textureId);

		GLCheck(glGenTextures(1, &textureId));
		GLCheck(glBindTexture(GL_TEXTURE_2D, textureId));
		GLCheck(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0,GL_RGB, GL_FLOAT, NULL));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT2,GL_TEXTURE_2D, textureId, 0));
		_textureId.push_back(textureId); 

	}else{

		// Texture
		GLCheck(glGenTextures( 1, &textureId ));
		GLCheck(glBindTexture(_textureType, textureId));
		GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		_textureId.push_back(textureId);

		U32 eTarget;
		U8 nFrames;
		if(type!=FBO_CUBE_COLOR) {
			eTarget = GL_TEXTURE_2D;
			nFrames = 1;
		} else {
			eTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
			nFrames = 6;
		}
		for(U8 i=0; i<nFrames; i++){
			if(type==FBO_2D_DEPTH)
				GLCheck(glTexImage2D(eTarget+i, 0, GL_DEPTH_COMPONENT, _width, _height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0));
			else
				GLCheck(glTexImage2D(eTarget+i, 0, GL_RGB, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0));
		}
		if(_useFBO)	{
			// Frame buffer
			GLCheck(glGenFramebuffers(1, &_frameBufferHandle));
			GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));
			if(_useDepthBuffer /*&& format!=GL_DEPTH_COMPONENT*/)
			{
				// Depth buffer
				GLCheck(glGenRenderbuffers(1, &_depthBufferHandle));
				GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle));
				GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height));
				// atasarea frame bufferului de depth buffer
				GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBufferHandle));
			}
			//  attach the framebuffer to our texture, which may be a depth texture
			if(type==FBO_2D_DEPTH) {
				_attachment = GL_DEPTH_ATTACHMENT;
				GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, eTarget, _textureId[0], 0));
				//  disable drawing to any buffers, we only want the depth
				GLCheck(glDrawBuffer(GL_NONE));
				GLCheck(glReadBuffer(GL_NONE));
			} else {
				_attachment = GL_COLOR_ATTACHMENT0;
				GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, eTarget, _textureId[0], 0));
			}
			_useFBO = checkStatus();
			GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		}
	}

	return true;
}



bool glFrameBufferObject::checkStatus()
{
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
        Console::getInstance().errorfn("Framebuffer incomplete: Attachment is NOT complete.");
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        Console::getInstance().errorfn("Framebuffer incomplete: No image is attached to FBO.");
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        Console::getInstance().errorfn("Framebuffer incomplete: Attached images have different dimensions.");
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
        Console::getInstance().errorfn("Framebuffer incomplete: Color attached images have different internal formats.");
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        Console::getInstance().errorfn("Framebuffer incomplete: Draw buffer.");
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        Console::getInstance().errorfn("Framebuffer incomplete: Read buffer.");
        return false;

    case GL_FRAMEBUFFER_UNSUPPORTED:
        Console::getInstance().errorfn("Unsupported by FBO implementation.");
        return false;

    default:
        Console::getInstance().errorfn("Unknow error.");
        return false;
    }
}




