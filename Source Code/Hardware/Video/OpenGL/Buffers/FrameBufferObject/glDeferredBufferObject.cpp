#include "Headers/glDeferredBufferObject.h"

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Headers/glFrameBufferObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glDeferredBufferObject::glDeferredBufferObject() : glFrameBufferObject(FBO_2D_DEFERRED)
{
	_textureType = GL_TEXTURE_2D;
}

bool glDeferredBufferObject::Create(GLushort width,
									GLushort height,
									GLubyte imageLayers){
	D_PRINT_FN(Locale::get("GL_FBO_GEN_DEFERRED"),width,height);
	Destroy();
	_width = width;
	_height = height;
	_useDepthBuffer = true;

	GLCheck(glGenFramebuffers(1, &_frameBufferHandle));
	GLCheck(glGenRenderbuffers(1, &_depthBufferHandle));

	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));

	TextureDescriptor albedo   = _attachement[TextureDescriptor::Color0];
	TextureDescriptor position = _attachement[TextureDescriptor::Color1];
	TextureDescriptor normals  = _attachement[TextureDescriptor::Color2];
	TextureDescriptor blend    = _attachement[TextureDescriptor::Color3];

    //Depth Buffer
	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle));
	GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,width, height));
	GLCheck(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER, _depthBufferHandle));
	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER,0));

    //Albedo
	GLCheck(glGenTextures(1, &_textureId[0]));
	GLCheck(glBindTexture(_textureType, _textureId[0]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[albedo._magFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[albedo._minFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[albedo._wrapU]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[albedo._wrapV]));

	GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[albedo._internalFormat],
						 width, height, 0, glImageFormatTable[albedo._format],
						 glDataFormat[albedo._dataType], NULL));

    //Positions
	GLCheck(glGenTextures(1, &_textureId[1]));
	GLCheck(glBindTexture(_textureType, _textureId[1]));

	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[position._magFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[position._minFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[position._wrapU]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[position._wrapV]));

	GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[position._internalFormat],
						 width, height, 0, glImageFormatTable[position._format],
						 glDataFormat[position._dataType], NULL));

    //Normals
	GLCheck(glGenTextures(1, &_textureId[2]));
	GLCheck(glBindTexture(_textureType, _textureId[2]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[normals._magFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[normals._minFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[normals._wrapU]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[normals._wrapV]));

	GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[normals._internalFormat],
						 width, height, 0, glImageFormatTable[normals._format],
						 glDataFormat[normals._dataType], NULL));

    //Blend
    GLCheck(glGenTextures(1, &_textureId[3]));
	GLCheck(glBindTexture(_textureType, _textureId[3]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[blend._magFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[blend._minFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[blend._wrapU]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[blend._wrapV]));

	GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[blend._internalFormat],
						 width, height, 0, glImageFormatTable[blend._format],
						 glDataFormat[blend._dataType], NULL));

	GLCheck(glBindTexture(_textureType,0));

    for(U8 i = 0; i < 4; i++){
	    GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, _textureType, _textureId[i], 0));
    }

	checkStatus();
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	return true;
}

void glDeferredBufferObject::Begin(GLubyte nFace) const {
	assert(nFace<6);
    GL_API::setViewport(0,0,_width,_height);
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	GLCheck(glDrawBuffers(4, buffers));
	GLCheck(glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
	GL_API::clearColor( 0.0f, 0.0f, 0.0f, 1.0f );
}