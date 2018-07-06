#include "Headers/glDeferredBufferObject.h"

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Headers/glFrameBufferObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glDeferredBufferObject::glDeferredBufferObject() : glFrameBufferObject(FBO_2D_DEFERRED),
												   _normalBufferHandle(0),
												   _positionBufferHandle(0),
												   _diffuseBufferHandle(0){
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
	GLCheck(glGenRenderbuffers(1, &_diffuseBufferHandle));
	GLCheck(glGenRenderbuffers(1, &_positionBufferHandle));
	GLCheck(glGenRenderbuffers(1, &_normalBufferHandle));
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));
	
	TextureDescriptor albedo = _attachement[TextureDescriptor::Color0];
	TextureDescriptor position = _attachement[TextureDescriptor::Color1];
	TextureDescriptor normals = _attachement[TextureDescriptor::Color2];

	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _diffuseBufferHandle));
	GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, glImageFormatTable[albedo._internalFormat], width,  height));
	GLCheck(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER, _diffuseBufferHandle));

	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _positionBufferHandle));
	GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, glImageFormatTable[position._internalFormat], width, height));
	GLCheck(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_RENDERBUFFER, _positionBufferHandle));

	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _normalBufferHandle));
	GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, glImageFormatTable[normals._internalFormat], width,height));
	GLCheck(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT2,GL_RENDERBUFFER, _normalBufferHandle));

	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle));
	GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,width, height));
	GLCheck(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER, _depthBufferHandle));
	
	GLCheck(glGenTextures(1, &_textureId[0]));
	GLCheck(glBindTexture(_textureType, _textureId[0]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[albedo._magFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[albedo._minFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[albedo._wrapU]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[albedo._wrapV]));
	///General texture parameters for either color or depth 
	if(albedo._generateMipMaps){
		///(depth doesn't need mipmaps, but no need for another "if" to complicate things)
		GLCheck(glTexParameteri(_textureType, GL_TEXTURE_BASE_LEVEL, albedo._mipMinLevel));
		GLCheck(glTexParameteri(_textureType, GL_TEXTURE_MAX_LEVEL,  albedo._mipMaxLevel));
	}

	GLCheck(glTexImage2D(_textureType,
						 0,
						 glImageFormatTable[albedo._internalFormat],
						 width,
						 height,
						 0,
						 glImageFormatTable[albedo._format],
						 glDataFormat[albedo._dataType],
						 NULL));

	GLCheck(glGenTextures(1, &_textureId[1]));
	GLCheck(glBindTexture(_textureType, _textureId[1]));

	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[position._magFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[position._minFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[position._wrapU]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[position._wrapV]));
	///General texture parameters for either color or depth 
	if(position._generateMipMaps){
		///(depth doesn't need mipmaps, but no need for another "if" to complicate things)
		GLCheck(glTexParameteri(_textureType, GL_TEXTURE_BASE_LEVEL, position._mipMinLevel));
		GLCheck(glTexParameteri(_textureType, GL_TEXTURE_MAX_LEVEL,  position._mipMaxLevel));
	}

	GLCheck(glTexImage2D(_textureType, 
						 0,
						 glImageFormatTable[position._internalFormat],
						 width,
						 height,
						 0,
						 glImageFormatTable[position._format],
						 glDataFormat[position._dataType],
						 NULL));

	GLCheck(glGenTextures(1, &_textureId[2]));
	GLCheck(glBindTexture(_textureType, _textureId[2]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[normals._magFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[normals._minFilter]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[normals._wrapU]));
	GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[normals._wrapV]));

	///General texture parameters for either color or depth 
	if(normals._generateMipMaps){
		///(depth doesn't need mipmaps, but no need for another "if" to complicate things)
		GLCheck(glTexParameteri(_textureType, GL_TEXTURE_BASE_LEVEL, normals._mipMinLevel));
		GLCheck(glTexParameteri(_textureType, GL_TEXTURE_MAX_LEVEL,  normals._mipMaxLevel));
	}

	GLCheck(glTexImage2D(_textureType, 
						 0,
						 glImageFormatTable[normals._internalFormat],
						 width,
						 height,
						 0,
						 glImageFormatTable[normals._format],
						 glDataFormat[normals._dataType],
						 NULL));

	GLCheck(glBindTexture(_textureType,0));

	GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,
								   GL_COLOR_ATTACHMENT0,
								   _textureType,
								   _textureId[0],
								   0));

	GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,
								   GL_COLOR_ATTACHMENT1,
								   _textureType,
								   _textureId[1],
								   0));

	GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,
								   GL_COLOR_ATTACHMENT2,
								   _textureType,
								   _textureId[2],
								   0));
	
	checkStatus();
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GLCheck(glBindRenderbuffer(GL_RENDERBUFFER,0));
	return true;
}

void glDeferredBufferObject::Destroy(){
	glFrameBufferObject::Destroy();
	GLCheck(glDeleteFramebuffers(1, &_normalBufferHandle));
	GLCheck(glDeleteFramebuffers(1, &_positionBufferHandle));
	GLCheck(glDeleteFramebuffers(1, &_diffuseBufferHandle));

}

void glDeferredBufferObject::Begin(GLubyte nFace) const {
	assert(nFace<6);
	GLCheck(glPushAttrib(GL_VIEWPORT_BIT));
	GLCheck(glViewport(0, 0, _width, _height));
 
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	GLCheck(glDrawBuffers(3, buffers));
	GLCheck(glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ));
	GLCheck(glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ));
}