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

	GLCheck(glGenTextures(4, _textureId));

    //Albedo
	GLCheck(glBindTexture(_textureType, _textureId[0]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[albedo.getSampler().magFilter()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[albedo.getSampler().minFilter()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[albedo.getSampler().wrapU()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[albedo.getSampler().wrapV()]));

	GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[albedo._internalFormat],
						 width, height, 0, glImageFormatTable[albedo._format],
						 glDataFormat[albedo._dataType], NULL));

    //Positions
	GLCheck(glBindTexture(_textureType, _textureId[1]));

	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[position.getSampler().magFilter()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[position.getSampler().minFilter()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[position.getSampler().wrapU()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[position.getSampler().wrapV()]));

	GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[position._internalFormat],
						 width, height, 0, glImageFormatTable[position._format],
						 glDataFormat[position._dataType], NULL));

    //Normals
	GLCheck(glBindTexture(_textureType, _textureId[2]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[normals.getSampler().magFilter()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[normals.getSampler().minFilter()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[normals.getSampler().wrapU()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[normals.getSampler().wrapV()]));

	GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[normals._internalFormat],
						 width, height, 0, glImageFormatTable[normals._format],
						 glDataFormat[normals._dataType], NULL));

    //Blend
 	GLCheck(glBindTexture(_textureType, _textureId[3]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[blend.getSampler().magFilter()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[blend.getSampler().minFilter()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[blend.getSampler().wrapU()]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[blend.getSampler().wrapV()]));

	GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[blend._internalFormat],
						 width, height, 0, glImageFormatTable[blend._format],
						 glDataFormat[blend._dataType], NULL));

	GLCheck(glBindTexture(_textureType,0));

    for(U8 i = 0; i < 4; i++){
	    GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, _textureType, _textureId[i], 0));
    }

	checkStatus();
	GLCheck(glClear(_clearBufferMask));
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	return true;
}

void glDeferredBufferObject::Begin(GLubyte nFace) const {
	assert(nFace<6);
    GL_API::setViewport(vec4<U32>(0,0,_width,_height));
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	GLCheck(glDrawBuffers(4, buffers));
	if(_clearBuffersState)
		GLCheck(glClear(_clearBufferMask));
	if(_clearColorState)
		GL_API::clearColor( _clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a );
}