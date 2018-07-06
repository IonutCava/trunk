#include "Headers/FrameBufferObject.h"
FrameBufferObject::FrameBufferObject(FBOType type) : _frameBufferHandle(0),
													 _depthBufferHandle(0),
												     _width(0),
						  						     _height(0), 
													 _textureType(0),
													 _fboType(type),
													 _bound(false),
													 _useDepthBuffer(false),
													 _disableColorWrites(false),
											 		 _fixedPipeline(false)
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
	if(_bound && _prevTexture == texture && _prevUnit == unit) 
		D_ERROR_FN(Locale::get("ERROR_FBO_DOUBLE_BIND"));

	_bound = true;
	_prevTexture = texture;
	_prevUnit = unit;
}	

void FrameBufferObject::Unbind(U8 unit) {
	if(!_bound)	D_ERROR_FN(Locale::get("ERROR_FBO_INVALID_UNBIND"));

	_bound = false;
	_fixedPipeline = false;
	_prevTexture = -1;
	_prevUnit = -1;
}