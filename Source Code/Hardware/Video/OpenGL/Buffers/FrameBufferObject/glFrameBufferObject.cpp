#include "Headers/glFrameBufferObject.h"
#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Headers/ParamHandler.h"

bool glFrameBufferObject::Create(GLushort width, 
								 GLushort height,
								 GLubyte imageLayers)
{
	_width = width;
	_height = height;
	_imageLayers = imageLayers;
	bool useDepthTexture = false;

	assert(!_attachement.empty());

	if(_frameBufferHandle <= 0){
		GLCheck(glGenFramebuffers(1, &_frameBufferHandle));
	}
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));	
	//For every attachement, be it a color or depth attachement ...
	for_each(TextureAttachements::value_type& it, _attachement){
		///get the slot. This is also the COLOR attachment offset
		U8 slot = it.first;
		///If it changed
		if(_attachementDirty[it.first]){
			///Get the current attachement's descriptor
			TextureDescriptor& texDescriptor = it.second;
			///And get the image formats and data type
			if(_textureType != glTextureTypeTable[texDescriptor._type]){
				ERROR_FN(Locale::get("ERROR_FBO_ATTACHEMENT_DIFFERENT"), (I32)slot);
			}
			GLenum format = glImageFormatTable[texDescriptor._format];
			GLenum internalFormat = glImageFormatTable[texDescriptor._internalFormat];
			GLenum dataType = glDataFormat[texDescriptor._dataType];

			///generate a new texture attachement
			if(slot != TextureDescriptor::Depth){
				GLCheck(glGenTextures( 1, &_textureId[slot] ));
				GLCheck(glBindTexture(_textureType, _textureId[slot]));

				///anisotrophic filtering is only added to color attachements
				if (texDescriptor._anisotrophyLevel > 1 && 	texDescriptor._generateMipMaps) {
					if(!glewIsSupported("GL_EXT_texture_filter_anisotropic")){
						ERROR_FN(Locale::get("ERROR_NO_ANISO_SUPPORT"));
					}else{
						GLCheck(glTexParameteri(_textureType, GL_TEXTURE_MAX_ANISOTROPY_EXT,texDescriptor._anisotrophyLevel));
					}
				}
			}else{///or a depth texture, depending on the slot
				GLCheck(glGenTextures( 1, &_depthId ));
				GLCheck(glBindTexture(_textureType, _depthId));
				
				///depth attachements may need comparison functions
				if(texDescriptor._useRefCompare){
					GLCheck(glTexParameteri(_textureType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE));
					GLCheck(glTexParameteri(_textureType, GL_TEXTURE_COMPARE_FUNC,  glCompareFuncTable[texDescriptor._cmpFunc]));
					GLCheck(glTexParameteri(_textureType, GL_DEPTH_TEXTURE_MODE, glImageFormatTable[texDescriptor._depthCompareMode]));
				}
			}

			///General texture parameters for either color or depth 
			if(texDescriptor._generateMipMaps){
				///(depth doesn't need mipmaps, but no need for another "if" to complicate things)
				GLCheck(glTexParameteri(_textureType, GL_TEXTURE_BASE_LEVEL, texDescriptor._mipMinLevel));
				GLCheck(glTexParameteri(_textureType, GL_TEXTURE_MAX_LEVEL,  texDescriptor._mipMaxLevel));
			}
			GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[texDescriptor._magFilter]));
			GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[texDescriptor._minFilter]));
			GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S,     glWrapTable[texDescriptor._wrapU]));

			///Anything other then a 1D texture, needs a T-wrap mode
			if(_textureType != GL_TEXTURE_1D){
				GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[texDescriptor._wrapV]));
			}
			///3D texture types need a R-wrap mode
			if(_textureType == GL_TEXTURE_3D || 
			   _textureType == GL_TEXTURE_CUBE_MAP ||
			   _textureType == GL_TEXTURE_CUBE_MAP_ARRAY) {
				GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_R, glWrapTable[texDescriptor._wrapW]));
			}

			///generate empty texture data using each texture type's specific function
			switch(_textureType){
				case GL_TEXTURE_1D:{
					GLCheck(glTexImage1D(GL_TEXTURE_1D,
										 0,
										 internalFormat,
										 _width,
										 0,
										 format,
										 dataType,
										 NULL));
				}break;
				
				case GL_TEXTURE_2D_MULTISAMPLE:
				case GL_TEXTURE_2D:{
					GLCheck(glTexImage2D(_textureType, 
										 0,
										 internalFormat,
										 _width,
										 _height,
										 0,
										 format,
										 dataType,
										 NULL));
				}break;

				case GL_TEXTURE_CUBE_MAP:{
					for(GLubyte j=0; j<6; j++){
							GLCheck(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+j,
												 0,
												 internalFormat,
												 _width,
												 _height,
												 0,
												 format,
												 dataType,
												 NULL));
						}
				}break;
				case GL_TEXTURE_2D_ARRAY:
				case GL_TEXTURE_3D:{
					GLCheck(glTexImage3D(_textureType,
										 0,
										 internalFormat,
										 _width,
										 _height,
										 _imageLayers,//Use as depth for GL_TEXTURE_3D
										 0, 
										 format,
										 dataType,
										 NULL));

					}break;
	
				case GL_TEXTURE_CUBE_MAP_ARRAY: {
					assert(false); ///not implemented yet
					for(GLubyte j=0; j<6; j++){
						GLCheck(glTexImage3D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+j, 
											 0,
											 internalFormat,
											 _width,
											 _height,
											 _imageLayers,
											 0,
											 format,
											 dataType,
											 NULL));
					}
				}break;
			};
			if(slot != TextureDescriptor::Depth){
				///Generate mip maps for color attachements
				if(texDescriptor._generateMipMaps){
					if(glGenerateMipmap != NULL)
						GLCheck(glGenerateMipmap(_textureType));
					else 
						ERROR_FN(Locale::get("ERROR_NO_MIP_MAPS"));
				}
				///Attach to frame buffer
				switch(_textureType){
					case GL_TEXTURE_1D:{
						GLCheck(glFramebufferTexture1D(GL_FRAMEBUFFER,
							                           GL_COLOR_ATTACHMENT0 + slot,
												       GL_TEXTURE_1D,
												       _textureId[slot],
												       0));
					}break;
					case GL_TEXTURE_2D_MULTISAMPLE:
					case GL_TEXTURE_2D:
					case GL_TEXTURE_CUBE_MAP: {
						 GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,
						                                GL_COLOR_ATTACHMENT0 + slot,
												        _textureType,
												        _textureId[slot],
												        0));
					}break;
					case GL_TEXTURE_3D:
					case GL_TEXTURE_2D_ARRAY:
					/*case GL_TEXTURE_CUBE_MAP_ARRAY:*/{
						GLCheck(glFramebufferTexture3D(GL_FRAMEBUFFER, 
													   GL_COLOR_ATTACHMENT0 + slot,
													   _textureType,
													   _textureId[slot],
													   0,
													   0 ));
					}break;
					case GL_TEXTURE_CUBE_MAP_ARRAY: {
						assert(false); ///not implemented yet
					}break;
				};
			}else{ ///depth attachement
				useDepthTexture = true;
			}
			///unbind the texture (kinda' late...)
			GLCheck(glBindTexture(_textureType, 0));
		}
	}

	///If we either specify a depth texture or request a depth buffer ...		
	if(_useDepthBuffer || useDepthTexture){
		///generate a render buffer
		GLCheck(glGenRenderbuffers(1, &_depthBufferHandle));
		GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle));
		GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height));
	
		if(_useDepthBuffer && !useDepthTexture){
			///set up render buffer storage
			GLCheck(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBufferHandle));

		}else if(useDepthTexture){ ///Use a texture attachement
			GLenum internalFormat = _attachement[TextureDescriptor::Depth]._internalFormat;
			///set up render buffer storage, either normal or multisampled if needed
			if(_textureType == GL_TEXTURE_2D_MULTISAMPLE){
				U8 msaaSamples = ParamHandler::getInstance().getParam<U8>("rendering.FSAAsamples",2);
				GLCheck(glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaSamples, internalFormat, _width, _height));
			}else{
				GLCheck(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBufferHandle));
			}
			///Attach to depth buffer
			switch(_textureType){
				case GL_TEXTURE_1D:{
					GLCheck(glFramebufferTexture1D(GL_FRAMEBUFFER,
							                       GL_DEPTH_ATTACHMENT,
												   GL_TEXTURE_1D,
												   _depthId,
												   0));
				}break;
				case GL_TEXTURE_2D_MULTISAMPLE:
				case GL_TEXTURE_2D:
				case GL_TEXTURE_CUBE_MAP: {
					GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER,
					                               GL_DEPTH_ATTACHMENT,
						  				           _textureType,
											       _depthId,
												   0));
				}break;
				case GL_TEXTURE_3D:
				case GL_TEXTURE_2D_ARRAY:
				/*case GL_TEXTURE_CUBE_MAP_ARRAY:*/{
					GLCheck(glFramebufferTexture3D(GL_FRAMEBUFFER, 
												   GL_DEPTH_ATTACHMENT,
												   _textureType,
												   _depthId,
												   0,
												   0 ));
				}break;
				case GL_TEXTURE_CUBE_MAP_ARRAY: {
					assert(false); ///not implemented yet
				}break;
			};
		}
	}
	///If color writes are disabled, draw only depth info
	if(_disableColorWrites){
		GLCheck(glDrawBuffer(GL_NONE));
		GLCheck(glReadBuffer(GL_NONE));
	}
	checkStatus();
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GLCheck(glBindRenderbuffer(GL_RENDERBUFFER,0));
	return true;
}

void glFrameBufferObject::Destroy() {
	Unbind();

	for(U8 i = 0; i < 4; i++){
		if(_textureId[i] > 0){
			GLCheck(glDeleteTextures(1, &_textureId[i]));
			_textureId[i] = 0;
		}
	}

	if(_depthId > 0){
		GLCheck(glDeleteTextures(1, &_depthId));
		_depthId = 0;
	}

	if(_frameBufferHandle > 0){
		GLCheck(glDeleteFramebuffers(1, &_frameBufferHandle));
		_frameBufferHandle = 0;
	}
	if(_depthBufferHandle > 0){
		GLCheck(glDeleteRenderbuffers(1, &_depthBufferHandle));
		_depthBufferHandle = 0;
	}

	_width = 0;
	_height = 0;
}

void glFrameBufferObject::Bind(GLubyte unit, GLubyte texture) {
	FrameBufferObject::Bind(unit,texture);
	if(_fixedPipeline){
		GLCheck(glEnable(_textureType));
	}
    GL_API::setActiveTextureUnit(unit);
	GLCheck(glBindTexture(_textureType, _textureId[texture]));
}

void glFrameBufferObject::Unbind(GLubyte unit) {
	FrameBufferObject::Unbind(unit);
    GL_API::setActiveTextureUnit(unit);
	GLCheck(glBindTexture(_textureType, 0 ));
    if(_fixedPipeline){
		GLCheck(glDisable(_textureType));
	}
}

void glFrameBufferObject::Begin(GLubyte nFace) const {
	assert(nFace<6);
	GLCheck(glPushAttrib(GL_VIEWPORT_BIT));
	GLCheck(glViewport(0, 0, _width, _height));
 
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));
	if(_textureType == GL_TEXTURE_CUBE_MAP) {
		GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X+nFace, _textureId[0], 0));
	}
	GLCheck(glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ));
	GLCheck(glClearColor( 0.0f, 0.0f, 0.3f, 1.0f ));
}

void glFrameBufferObject::End(GLubyte nFace) const {
	assert(nFace<6);
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GLCheck(glPopAttrib());//Viewport Bit
}

bool glFrameBufferObject::checkStatus() {
	//Not defined in GLEW
	#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS 0x8CDA
	#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9

    // check FBO status
    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status)
    {
    case GL_FRAMEBUFFER_COMPLETE:
        return true;

	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:{
		ERROR_FN(Locale::get("ERROR_FBO_ATTACHMENT_INCOMPLETE"));
        return false;
											  }
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:{
        ERROR_FN(Locale::get("ERROR_FBO_NO_IMAGE"));
        return false;
											  }
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:{
        ERROR_FN(Locale::get("ERROR_FBO_DIMENSIONS"));
        return false;
											  }
	case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:{
        ERROR_FN(Locale::get("ERROR_FBO_FORMAT"));
        return false;
										   }
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:{
        ERROR_FN(Locale::get("ERROR_FBO_INCOMPLETE_DRAW_BUFFER"));
        return false;
											   }
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:{
        ERROR_FN(Locale::get("ERROR_FBO_INCOMPLETE_READ_BUFFER"));
        return false;
											   }
	case GL_FRAMEBUFFER_UNSUPPORTED:{
        ERROR_FN(Locale::get("ERROR_FBO_UNSUPPORTED"));
        return false;
									}
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:{
		ERROR_FN(Locale::get("ERROR_FBO_INCOMPLETE_MULTISAMPLE"));
		return false;
												   }
	default:{
		ERROR_FN(Locale::get("ERROR_UNKNOWN"));
        return false;
		}
    };
}

void glFrameBufferObject::BlitFrom(FrameBufferObject* inputFBO){
	GLCheck(glBindFramebuffer(GL_READ_FRAMEBUFFER, inputFBO->getHandle()));
	GLCheck(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->_frameBufferHandle));
	GLCheck(glBlitFramebuffer(0, 0, inputFBO->getWidth(), inputFBO->getHeight(), 0, 0, this->_width, this->_height, GL_COLOR_BUFFER_BIT, GL_NEAREST));
	GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}