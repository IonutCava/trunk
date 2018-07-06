#include "Headers/FrameBuffer.h"

FrameBuffer::FrameBuffer(FBType type) : _frameBufferHandle(0),
                                        _width(0),
                                        _height(0),
                                        _textureType(0),
                                        _fbType(type),
                                        _useDepthBuffer(false),
                                        _disableColorWrites(false),
                                        _bound(false),
                                        _fpDepth(false),
                                        _clearBuffersState(true),
                                        _clearColorState(true)
{
    _clearColor.set(DefaultColors::WHITE());
}

FrameBuffer::~FrameBuffer()
{
    _attachment.clear();
    _attachmentDirty.clear();
}

bool FrameBuffer::AddAttachment(const TextureDescriptor& descriptor, TextureDescriptor::AttachmentType slot){
    bool invalidAtt = false;
    //Validation
    switch(_fbType){
        case FB_2D_COLOR_MS: { // will convert to TEXTURE_2D_MULTISAMPLE if needed later
            if(descriptor._type != TEXTURE_2D && descriptor._type != TEXTURE_2D_MS){
                invalidAtt = true;
            }
        }break;
        case FB_2D_DEFERRED:
        case FB_2D_DEPTH: {
            if(descriptor._type != TEXTURE_2D){
                invalidAtt = true;
            }
        }break;
        case FB_2D_COLOR:
        case FB_CUBE_COLOR:{
            if(descriptor._type != TEXTURE_2D && descriptor._type != TEXTURE_CUBE_MAP){
                invalidAtt = true;
            }
        }break;
        case FB_CUBE_COLOR_ARRAY:{
            if(descriptor._type != TEXTURE_CUBE_ARRAY){
                invalidAtt = true;
            }
        }break;
        case FB_2D_ARRAY_DEPTH:
        case FB_2D_ARRAY_COLOR:
        case FB_CUBE_DEPTH_ARRAY:{
            if(descriptor._type != TEXTURE_2D_ARRAY){
                invalidAtt = true;
            }
        }break;
    };

    if(invalidAtt){
        ERROR_FN(Locale::get("ERROR_FB_ATTACHMENT_DIFFERENT"),(I32) slot);
        return false;
    }

    _attachmentDirty[slot] = true;
    _attachment[slot] = descriptor;
    return true;
}

void FrameBuffer::Bind(U8 unit, TextureDescriptor::AttachmentType slot) const {
    _bound = true;
}

void FrameBuffer::Unbind(U8 unit) const {
    _bound = false;
}