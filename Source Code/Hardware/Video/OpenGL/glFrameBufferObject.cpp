#include "glResources.h"
#include "resource.h"
#include "glFrameBufferObject.h"
#include "Hardware/Video/GFXDevice.h"

glFrameBufferObject::glFrameBufferObject()
{
	if(!glewIsSupported("glBindFramebuffer"))
	{
		glBindFramebuffer = GLEW_GET_FUN(__glewBindFramebufferEXT);
		glDeleteFramebuffers = GLEW_GET_FUN(__glewDeleteFramebuffersEXT);
		glFramebufferTexture2D = GLEW_GET_FUN(__glewFramebufferTexture2DEXT);
		glGenFramebuffers   = GLEW_GET_FUN(__glewGenFramebuffersEXT);
		glCheckFramebufferStatus = GLEW_GET_FUN(__glewCheckFramebufferStatusEXT);
	}

	if(!glewIsSupported("glBindRenderbuffer"))
	{
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
}

void glFrameBufferObject::Destroy()
{
	for(U8 i = 0; i < _textureId.size(); i++)
		glDeleteTextures(1, &_textureId[i]);

	glDeleteFramebuffers(1, &_frameBufferHandle);
	if(_useDepthBuffer)
		glDeleteRenderbuffers(1, &_depthBufferHandle);

	if(_fboType == FBO_2D_DEFERRED)
	{
		glDeleteFramebuffers(1, &_diffuseBufferHandle);
		glDeleteFramebuffers(1, &_positionBufferHandle);
		glDeleteFramebuffers(1, &_normalBufferHandle);
	}
	_textureId.clear();
	_width = 0;
	_height = 0;
	_useFBO = true;
}


void glFrameBufferObject::Begin(U8 nFace) const
{
	assert(nFace<6);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, _width, _height);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

		if(_useFBO) {
			glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle);
			if(_fboType == FBO_2D_DEFERRED){
				GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,  GL_COLOR_ATTACHMENT2 };
				glDrawBuffers(3, buffers);
			}
			else if(_textureType == GL_TEXTURE_CUBE_MAP) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, _attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X+nFace, _textureId[0], 0);
			}

		}else {
			glPushAttrib(GL_COLOR_BUFFER_BIT | GL_PIXEL_MODE_BIT);
			glDrawBuffer(GL_BACK);
			glReadBuffer(GL_BACK);
		}
	
}

void glFrameBufferObject::End(U8 nFace) const
{
	assert(nFace<6);
	if(_useFBO) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	else {
        glBindTexture(_textureType, _textureId[0]);
        glCopyTexSubImage2D(_textureType, 0, 0, 0, 0, 0, _width, _height);
        glBindTexture(_textureType, 0);

        glPopAttrib();	//Color and Pixel Mode bits
	}
	glPopAttrib();//Viewport Bit
}

void glFrameBufferObject::Bind(U8 unit, U8 texture) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glEnable(_textureType);
	glBindTexture(_textureType, _textureId[texture]);
}

void glFrameBufferObject::Unbind(U8 unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture( _textureType, 0 );
	glDisable(_textureType);
}


bool glFrameBufferObject::Create(FBO_TYPE type, U16 width, U16 height)
{
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
        glGenFramebuffers(1, &_frameBufferHandle);
		glGenRenderbuffers(1, &_depthBufferHandle);
		glGenFramebuffers(1, &_diffuseBufferHandle);
		glGenRenderbuffers(1, &_positionBufferHandle);
		glGenRenderbuffers(1, &_normalBufferHandle);
		glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle);

		glBindRenderbuffer(GL_RENDERBUFFER, _diffuseBufferHandle);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, width,  height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER, _diffuseBufferHandle);

		glBindRenderbuffer(GL_RENDERBUFFER, _positionBufferHandle);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB32F, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_RENDERBUFFER, _positionBufferHandle);

	    glBindRenderbuffer(GL_RENDERBUFFER, _normalBufferHandle);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB16F, width,height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT2,GL_RENDERBUFFER, _normalBufferHandle);

		glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER, _depthBufferHandle);
	
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, textureId, 0);
		_textureId.push_back(textureId);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D, textureId, 0);
		_textureId.push_back(textureId);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0,GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT2,GL_TEXTURE_2D, textureId, 0);
		_textureId.push_back(textureId); 

	}else{

		// Texture
		glGenTextures( 1, &textureId );
		glBindTexture(_textureType, textureId);
		glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
				glTexImage2D(eTarget+i, 0, GL_DEPTH_COMPONENT24, _width, _height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
			else
				glTexImage2D(eTarget+i, 0, GL_RGB, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		}
		if(_useFBO)	{
			// Frame buffer
			glGenFramebuffers(1, &_frameBufferHandle);
			glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle);
			if(_useDepthBuffer /*&& format!=GL_DEPTH_COMPONENT*/)
			{
				// Depth buffer
				glGenRenderbuffers(1, &_depthBufferHandle);
				glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _width, _height);
				// atasarea frame bufferului de depth buffer
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBufferHandle);
			}
			//  attach the framebuffer to our texture, which may be a depth texture
			if(type==FBO_2D_DEPTH) {
				_attachment = GL_DEPTH_ATTACHMENT;
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, eTarget, _textureId[0], 0);
				//  disable drawing to any buffers, we only want the depth
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			} else {
				_attachment = GL_COLOR_ATTACHMENT0;
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, eTarget, _textureId[0], 0);
			}
			_useFBO = checkStatus();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
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




