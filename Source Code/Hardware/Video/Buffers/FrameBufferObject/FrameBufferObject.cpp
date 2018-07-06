#include "Headers/FrameBufferObject.h"
FrameBufferObject::FrameBufferObject(FBOType type) : _frameBufferHandle(0),
													 _depthBufferHandle(0),
												     _width(0),
						  						     _height(0),
													 _textureType(0),
													 _fboType(type),
													 _useDepthBuffer(false),
													 _disableColorWrites(false)
{
}

FrameBufferObject::~FrameBufferObject()
{
	_attachement.clear();
	_attachementDirty.clear();
}

bool FrameBufferObject::AddAttachment(const TextureDescriptor& descriptor,
									  TextureDescriptor::AttachmentType slot){
//Validation
	switch(_fboType){
		case FBO_2D_DEFERRED:
		case FBO_2D_COLOR_MS:
		case FBO_2D_DEPTH: {
			if(descriptor._type != TEXTURE_2D){
				ERROR_FN(Locale::get("ERROR_FBO_ATTACHEMENT_DIFFERENT"),(I32) slot);
				return false;
			}
		}break;
		case FBO_2D_COLOR:
		case FBO_CUBE_COLOR:{
			if(descriptor._type != TEXTURE_2D && descriptor._type != TEXTURE_CUBE_MAP){
				ERROR_FN(Locale::get("ERROR_FBO_ATTACHEMENT_DIFFERENT"),(I32) slot);
				return false;
			}
		}break;
		case FBO_CUBE_COLOR_ARRAY:{
			if(descriptor._type != TEXTURE_CUBE_ARRAY){
				ERROR_FN(Locale::get("ERROR_FBO_ATTACHEMENT_DIFFERENT"),(I32) slot);
				return false;
			}
		}break;
		case FBO_2D_ARRAY_DEPTH:
		case FBO_2D_ARRAY_COLOR:
		case FBO_CUBE_DEPTH_ARRAY:{
			if(descriptor._type != TEXTURE_2D_ARRAY){
				ERROR_FN(Locale::get("ERROR_FBO_ATTACHEMENT_DIFFERENT"),(I32) slot);
				return false;
			}
		}break;
	};
	_attachementDirty[slot] = true;
	_attachement[slot] = descriptor;
	return true;
}

void FrameBufferObject::Bind(U8 unit, U8 texture) {
}

void FrameBufferObject::Unbind(U8 unit) {
}