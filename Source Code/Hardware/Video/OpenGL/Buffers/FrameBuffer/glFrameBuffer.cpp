#include "Headers/glFrameBuffer.h"
#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/OpenGL/Textures/Headers/glSamplerObject.h"

#include "core.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"

bool glFrameBuffer::_viewportChanged = false;
bool glFrameBuffer::_mipMapsDirty = false;

glFrameBuffer::glFrameBuffer(FBType type) : FrameBuffer(type)
{
    memset(_textureId, 0, 5 * sizeof(GLuint));
    memset(_mipMapEnabled, false, 5 * sizeof(bool));

    _imageLayers = 0;
    _hasDepth = _hasColor = false;

    _color0WrapMode[0] = TEXTURE_CLAMP_TO_EDGE;
    _color0WrapMode[1] = TEXTURE_CLAMP_TO_EDGE;
    _color0WrapMode[2] = TEXTURE_CLAMP_TO_EDGE;

    switch(type){
        default:
        case FB_2D_DEPTH:
        case FB_2D_DEFERRED: _textureType = GL_TEXTURE_2D; break;
        case FB_2D_COLOR_MS: _textureType = GL_TEXTURE_2D_MULTISAMPLE; break;
        case FB_CUBE_DEPTH:
        case FB_CUBE_COLOR: _textureType = GL_TEXTURE_CUBE_MAP; break;
        case FB_2D_ARRAY_COLOR:
        case FB_2D_ARRAY_DEPTH : _textureType = GL_TEXTURE_2D_ARRAY; break;
        case FB_CUBE_COLOR_ARRAY:
        case FB_CUBE_DEPTH_ARRAY: _textureType = GL_TEXTURE_CUBE_MAP_ARRAY; break;
    };

    if(type == FB_CUBE_DEPTH || type == FB_2D_ARRAY_DEPTH || type == FB_CUBE_DEPTH_ARRAY || type == FB_2D_DEPTH){
        toggleColorWrites(false);
    }
    // if MSAA is disabled, this will be a simple color buffer
    if(type == FB_2D_COLOR_MS && !GL_API::useMSAA()){
        _textureType = GL_TEXTURE_2D;
    }
}

glFrameBuffer::~glFrameBuffer()
{
}

void glFrameBuffer::InitAttachment(TextureDescriptor::AttachmentType type, const TextureDescriptor& texDescriptor){
    if(type != TextureDescriptor::Depth) _hasColor = true;
    else                                 _hasDepth = true;
        
    //If it changed
    if(_attachmentDirty[type]){
        U8 slot = type;
        TextureType currentType = texDescriptor._type;
        // convert to proper type if we are using MSAA
        if(_fbType == FB_2D_COLOR_MS && GL_API::useMSAA() && currentType == TEXTURE_2D){
            currentType = TEXTURE_2D_MS;
        }
        //And get the image formats and data type
        if(_textureType != glTextureTypeTable[currentType]){
            ERROR_FN(Locale::get("ERROR_FB_ATTACHMENT_DIFFERENT"), (I32)slot);
        }

        GLenum format = glImageFormatTable[texDescriptor._format];
        GLenum internalFormat = glImageFormatTable[texDescriptor._internalFormat];
        GLenum dataType = glDataFormat[texDescriptor._dataType];

        const SamplerDescriptor& sampler = texDescriptor.getSampler();
        _mipMapEnabled[slot] = sampler.generateMipMaps();
            
        if (type == TextureDescriptor::Color0){
            _color0WrapMode[0] = sampler.wrapU();
            _color0WrapMode[1] = sampler.wrapV();
            _color0WrapMode[2] = sampler.wrapW();
        }
        glGenTextures( 1, &_textureId[slot]);
        glBindTexture(_textureType, _textureId[slot]);

        if(currentType == TEXTURE_2D_MS){
            _textureType = GL_TEXTURE_2D;
        }

        //generate a new texture attachment
        //anisotropic filtering is only added to color attachments
        if (sampler.anisotropyLevel() > 1 && _mipMapEnabled[slot] && type != TextureDescriptor::Depth) {
           U8 anisoLevel = std::min<I32>((I32)sampler.anisotropyLevel(), ParamHandler::getInstance().getParam<U8>("rendering.anisotropicFilteringLevel"));
           glTexParameteri(_textureType, GL_TEXTURE_MAX_ANISOTROPY_EXT,anisoLevel);
        }

        //depth attachments may need comparison functions
        if(sampler._useRefCompare){
            glTexParameteri(_textureType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
            glTexParameteri(_textureType, GL_TEXTURE_COMPARE_FUNC,  glCompareFuncTable[sampler._cmpFunc]);
        }
  
        //General texture parameters for either color or depth
        glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[sampler.magFilter()]);
        glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[sampler.minFilter()]);
        glTexParameterf(_textureType, GL_TEXTURE_WRAP_S,     glWrapTable[sampler.wrapU()]);
        if (sampler.wrapU() == TEXTURE_CLAMP_TO_BORDER ||
            sampler.wrapV() == TEXTURE_CLAMP_TO_BORDER ||
            sampler.wrapW() == TEXTURE_CLAMP_TO_BORDER){
            glTexParameterfv(_textureType, GL_TEXTURE_BORDER_COLOR, sampler.borderColor());
        }
        

        //Anything other then a 1D texture, needs a T-wrap mode
        if(_textureType != GL_TEXTURE_1D){
            glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[sampler.wrapV()]);
        }

        //3D texture types need a R-wrap mode
        if(_textureType == GL_TEXTURE_3D ||  _textureType == GL_TEXTURE_CUBE_MAP || _textureType == GL_TEXTURE_CUBE_MAP_ARRAY) {
            glTexParameterf(_textureType, GL_TEXTURE_WRAP_R, glWrapTable[sampler.wrapW()]);
        }

        if(currentType == TEXTURE_2D_MS){
            _textureType = GL_TEXTURE_2D_MULTISAMPLE;
        }
        //generate empty texture data using each texture type's specific function
        switch(_textureType){
            case GL_TEXTURE_1D:{
                glTexImage1D(GL_TEXTURE_1D, 0, internalFormat, _width, 0, format, dataType, NULL);
            }break;
            case GL_TEXTURE_2D_MULTISAMPLE:{
                GLubyte samples = ParamHandler::getInstance().getParam<GLubyte>("rendering.FSAAsamples",2);
                glTexImage2DMultisample(_textureType,samples, internalFormat, _width, _height,GL_TRUE);
            }break;
            case GL_TEXTURE_2D:{
                glTexImage2D(_textureType, 0, internalFormat, _width, _height, 0, format, dataType, NULL);
            }break;
            case GL_TEXTURE_CUBE_MAP:{
                for(GLubyte j=0; j<6; j++)
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, internalFormat, _width, _height, 0, format, dataType, NULL);
            }break;
            case GL_TEXTURE_2D_ARRAY:
            case GL_TEXTURE_3D:{
                 // Use _imageLayers as depth for GL_TEXTURE_3D
                glTexImage3D(_textureType, 0, internalFormat, _width, _height, _imageLayers, 0, format, dataType, NULL);
            }break;
            case GL_TEXTURE_CUBE_MAP_ARRAY: {
                assert(false); //not implemented yet
                for(GLubyte j=0; j<6; j++)
                    glTexImage3D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, internalFormat, _width, _height, _imageLayers, 0, format, dataType, NULL);
            }break;
        };
            
        //Generate mipmaps if needed
        if(_mipMapEnabled[slot]) {
            glTexParameteri(_textureType, GL_TEXTURE_BASE_LEVEL, texDescriptor._mipMinLevel);
            glTexParameteri(_textureType, GL_TEXTURE_MAX_LEVEL,  texDescriptor._mipMaxLevel);
            glGenerateMipmap(_textureType);
        }
        
        //unbind the texture
        glBindTexture(_textureType, 0);

        GLenum attachment = (type == TextureDescriptor::Depth) ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0 + slot;
        //Attach to frame buffer
        glFramebufferTexture(GL_FRAMEBUFFER, attachment, _textureId[slot], 0);
    }
}

void glFrameBuffer::AddDepthBuffer(){
        
    TextureType texType = (_textureType == GL_TEXTURE_2D_ARRAY || _textureType == GL_TEXTURE_CUBE_MAP_ARRAY) ? TEXTURE_2D_ARRAY : TEXTURE_2D;
    if(_fbType == FB_2D_COLOR_MS && GL_API::useMSAA() && texType == TEXTURE_2D){
        texType = TEXTURE_2D_MS;
    }
    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TEXTURE_FILTER_NEAREST, TEXTURE_FILTER_NEAREST);
    screenSampler.setWrapMode(_color0WrapMode[0], _color0WrapMode[1], _color0WrapMode[2]);
    screenSampler.toggleMipMaps(false);
    TextureDescriptor depthDescriptor(texType,
                                      DEPTH_COMPONENT,
                                      _fpDepth ? DEPTH_COMPONENT32F : DEPTH_COMPONENT,
                                      _fpDepth ? FLOAT_32 : UNSIGNED_INT);
        
    screenSampler._useRefCompare = true; //< Use compare function
    screenSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal
    depthDescriptor.setSampler(screenSampler);
    _attachmentDirty[TextureDescriptor::Depth] = true;
    _attachment[TextureDescriptor::Depth] = depthDescriptor;
    InitAttachment(TextureDescriptor::Depth, depthDescriptor);

    _hasDepth = true;
}

bool glFrameBuffer::Create(GLushort width, GLushort height, GLubyte imageLayers) {

    _width = width;
    _height = height;
    _imageLayers = imageLayers;

    _clearBufferMask = 0;
    assert(!_attachment.empty());
                    
    if(glGenerateMipmap == nullptr){
        ERROR_FN(Locale::get("ERROR_NO_MIP_MAPS"));
        assert(glGenerateMipmap);
    }

    if(_frameBufferHandle <= 0){
        glGenFramebuffers(1, &_frameBufferHandle);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle);
        
    _colorBuffers.resize(0);
    //For every attachment, be it a color or depth attachment ...
    FOR_EACH(TextureAttachments::value_type& it, _attachment){
       InitAttachment(it.first, it.second);
       if(it.first != TextureDescriptor::Depth)
           _colorBuffers.insert(_colorBuffers.begin(), GL_COLOR_ATTACHMENT0 + it.first);
    }

    if (!_colorBuffers.empty())
        glDrawBuffers((GLsizei)_colorBuffers.size(), &_colorBuffers[0]);

    //If we either specify a depth texture or request a depth buffer ...
    if(_useDepthBuffer && !_hasDepth) AddDepthBuffer();

    //If color writes are disabled, draw only depth info
    if(_disableColorWrites){
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        _hasColor = false;
    }

    if(_hasColor) _clearBufferMask = _clearBufferMask | GL_COLOR_BUFFER_BIT;
    if(_hasDepth) _clearBufferMask = _clearBufferMask | GL_DEPTH_BUFFER_BIT;

    checkStatus();

    if(_clearColorState) GL_API::clearColor( _clearColor );

    glClear(_clearBufferMask);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

void glFrameBuffer::Destroy() {
    Unbind();

    for(U8 i = 0; i < 5; i++){
        if(_textureId[i] > 0){
            glDeleteTextures(1, &_textureId[i]);
            _textureId[i] = 0;
        }
    }

    if(_frameBufferHandle > 0){
        glDeleteFramebuffers(1, &_frameBufferHandle);
        _frameBufferHandle = 0;
    }

    _width = _height = 0;
}

void glFrameBuffer::BlitFrom(FrameBuffer* inputFB) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, inputFB->getHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->_frameBufferHandle);
    glBlitFramebuffer(0, 0, inputFB->getWidth(), inputFB->getHeight(), 0, 0, 
                              this->_width, this->_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    _mipMapsDirty = true;
}

void glFrameBuffer::Bind(GLubyte unit, TextureDescriptor::AttachmentType slot) const {
    FrameBuffer::Bind(unit, slot);
    GL_API::setActiveTextureUnit(unit);
    glSamplerObject::Unbind(unit);
    glBindTexture(_textureType, _textureId[slot]);
    glTexture::textureBoundMap[unit] = std::make_pair(_textureId[slot], _textureType);
}

void glFrameBuffer::Unbind(GLubyte unit) const {
    FrameBuffer::Unbind(unit);
    GL_API::setActiveTextureUnit(unit);
    glBindTexture(_textureType, 0 );
    glTexture::textureBoundMap[unit] = std::make_pair(0, GL_NONE);
}

void glFrameBuffer::Begin(const FrameBufferTarget& drawPolicy) {
    if(_viewportChanged) {
        GL_API::restoreViewport();
        _viewportChanged = false;
    }

    GL_API::setViewport(vec4<GLint>(0,0,_width,_height));
    _viewportChanged = true;

    glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle);
    // this is checked so it isn't called twice on the GPU
    if (_clearColorState)     GL_API::clearColor(_clearColor);
    if (_clearBuffersState)   glClear(_clearBufferMask);
    if(!drawPolicy._depthOnly) _mipMapsDirty = true;
}

void glFrameBuffer::End() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_API::restoreViewport();
    _viewportChanged = false;
}

void glFrameBuffer::DrawToLayer(TextureDescriptor::AttachmentType slot, U8 layer, bool includeDepth) const {
    // only for array textures (it's better to simply ignore the command if the format isn't supported (debugging reasons)
    if(_textureType != GL_TEXTURE_2D_ARRAY && _textureType != GL_TEXTURE_CUBE_MAP_ARRAY)
        return;

    assert(layer < _imageLayers);

    if(_hasDepth && includeDepth)
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _textureId[TextureDescriptor::Depth], 0, layer);
    if(_hasColor)
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, _textureId[slot], 0, layer);

    if(_clearBuffersState){
        if(includeDepth){
            glClear(_clearBufferMask);
        }else{
            if(_hasColor)
                glClear(GL_COLOR_BUFFER_BIT);
        }
    }
}

void glFrameBuffer::DrawToFace(TextureDescriptor::AttachmentType slot, GLubyte nFace, bool includeDepth) const {
    if(_textureType != GL_TEXTURE_CUBE_MAP && _textureType != GL_TEXTURE_CUBE_MAP_ARRAY)
        return;

    assert(nFace<6);

    if(_hasDepth && includeDepth)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X+nFace, _textureId[TextureDescriptor::Depth], 0);
    if(_hasColor)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X+nFace, _textureId[slot], 0);

    if(_clearBuffersState){
        if(includeDepth)   glClear(_clearBufferMask);
        else{
            if(_hasColor)  glClear(GL_COLOR_BUFFER_BIT);
        }
    }
}

void glFrameBuffer::UpdateMipMaps(TextureDescriptor::AttachmentType slot) const {
    if(!_mipMapEnabled[slot] || !_bound || !_mipMapsDirty)
        return;

    glGenerateMipmap(_textureType);
    _mipMapsDirty = false;
}

bool glFrameBuffer::checkStatus() const {
    //Not defined in GLEW
    #ifndef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
        #define GL_FRAMEBUFFER_INCOMPLETE_FORMATS 0x8CDA
    #endif
    #ifndef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
        #define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9
    #endif

    // check FB status
    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status)
    {
    case GL_FRAMEBUFFER_COMPLETE:
        return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:{
        ERROR_FN(Locale::get("ERROR_FB_ATTACHMENT_INCOMPLETE"));
        return false;
                                              }
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:{
        ERROR_FN(Locale::get("ERROR_FB_NO_IMAGE"));
        return false;
                                              }
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:{
        ERROR_FN(Locale::get("ERROR_FB_DIMENSIONS"));
        return false;
                                              }
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:{
        ERROR_FN(Locale::get("ERROR_FB_FORMAT"));
        return false;
                                           }
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:{
        ERROR_FN(Locale::get("ERROR_FB_INCOMPLETE_DRAW_BUFFER"));
        return false;
                                               }
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:{
        ERROR_FN(Locale::get("ERROR_FB_INCOMPLETE_READ_BUFFER"));
        return false;
                                               }
    case GL_FRAMEBUFFER_UNSUPPORTED:{
        ERROR_FN(Locale::get("ERROR_FB_UNSUPPORTED"));
        return false;
                                    }
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:{
        ERROR_FN(Locale::get("ERROR_FB_INCOMPLETE_MULTISAMPLE"));
        return false;
                                                   }
    default:{
        ERROR_FN(Locale::get("ERROR_UNKNOWN"));
        return false;
        }
    };
}