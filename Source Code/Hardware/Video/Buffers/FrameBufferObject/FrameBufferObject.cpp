#include "Headers/FrameBufferObject.h"

FrameBufferObject::FrameBufferObject(FBOType type) : _frameBufferHandle(0),
                                                     _width(0),
                                                     _height(0),
                                                     _textureType(0),
                                                     _fboType(type),
                                                     _useDepthBuffer(false),
                                                     _disableColorWrites(false),
                                                     _bound(false),
                                                     _fpDepth(false),
                                                     _clearBuffersState(true),
                                                     _clearColorState(true)
{
    _clearColor.set(DefaultColors::WHITE());
}

FrameBufferObject::~FrameBufferObject()
{
    _attachement.clear();
    _attachementDirty.clear();
}

bool FrameBufferObject::AddAttachment(const TextureDescriptor& descriptor,
                                      TextureDescriptor::AttachmentType slot){
    bool invalidAtt = false;
    //Validation
    switch(_fboType){
        case FBO_2D_COLOR_MS: { // will convert to TEXTURE_2D_MULTISAMPLE if needed later
            if(descriptor._type != TEXTURE_2D && descriptor._type != TEXTURE_2D_MS){
                invalidAtt = true;
            }
        }break;
        case FBO_2D_DEFERRED:
        case FBO_2D_DEPTH: {
            if(descriptor._type != TEXTURE_2D){
                invalidAtt = true;
            }
        }break;
        case FBO_2D_COLOR:
        case FBO_CUBE_COLOR:{
            if(descriptor._type != TEXTURE_2D && descriptor._type != TEXTURE_CUBE_MAP){
                invalidAtt = true;
            }
        }break;
        case FBO_CUBE_COLOR_ARRAY:{
            if(descriptor._type != TEXTURE_CUBE_ARRAY){
                invalidAtt = true;
            }
        }break;
        case FBO_2D_ARRAY_DEPTH:
        case FBO_2D_ARRAY_COLOR:
        case FBO_CUBE_DEPTH_ARRAY:{
            if(descriptor._type != TEXTURE_2D_ARRAY){
                invalidAtt = true;
            }
        }break;
    };

    if(invalidAtt){
        ERROR_FN(Locale::get("ERROR_FBO_ATTACHEMENT_DIFFERENT"),(I32) slot);
        return false;
    }

    _attachementDirty[slot] = true;
    _attachement[slot] = descriptor;
    return true;
}

void FrameBufferObject::Bind(U8 unit, TextureDescriptor::AttachmentType slot) const {
    _bound = true;
}

void FrameBufferObject::Unbind(U8 unit) const {
    _bound = false;
}