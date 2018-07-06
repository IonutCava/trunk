#include "Headers/glFrameBuffer.h"
#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/OpenGL/Textures/Headers/glSamplerObject.h"

#include "core.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"

bool glFrameBuffer::_viewportChanged = false;
bool glFrameBuffer::_mipMapsDirty = false;
GLint glFrameBuffer::_maxColorAttachments = -1;
vec2<U16> glFrameBuffer::_prevViewportDim;

glFrameBuffer::glFrameBuffer(glFrameBuffer* resolveBuffer) : FrameBuffer(resolveBuffer != nullptr),
                                                             _resolveBuffer(resolveBuffer),
                                                             _hasDepth(false),
                                                             _hasColor(false),
                                                             _resolved(false),
                                                             _isLayeredDepth(false)
{
    if (_maxColorAttachments == -1)
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &_maxColorAttachments);

    memset(_textureId, 0, 5 * sizeof(GLuint));
    memset(_textureType, 0, 5 * sizeof(GLuint));
    memset(_mipMapEnabled, false, 5 * sizeof(bool));
}

glFrameBuffer::~glFrameBuffer()
{
    SAFE_DELETE(_resolveBuffer);
    Destroy();
}

void glFrameBuffer::InitAttachment(TextureDescriptor::AttachmentType type, const TextureDescriptor& texDescriptor){
    //If it changed
    if (!_attachmentDirty[type])
        return;

    assert(_width != 0 && _height != 0);

    if(type != TextureDescriptor::Depth) _hasColor = true;
    else                                 _hasDepth = true;

    GLint slot = (GLint)type;

    TextureType currentType = texDescriptor._type;

    if (_multisampled){
        if(currentType == TEXTURE_2D)       currentType = TEXTURE_2D_MS;
        if(currentType == TEXTURE_2D_ARRAY) currentType = TEXTURE_2D_ARRAY_MS;
    }else{
        if(currentType == TEXTURE_2D_MS)       currentType = TEXTURE_2D;
        if(currentType == TEXTURE_2D_ARRAY_MS) currentType = TEXTURE_2D_ARRAY;
    }
    _textureType[slot] = glTextureTypeTable[currentType];

    const SamplerDescriptor& sampler = texDescriptor.getSampler();

    _mipMapEnabled[slot] = (!_multisampled && sampler.generateMipMaps());
            
    if (_textureId[slot] <= 0)
        glGenTextures(1, &_textureId[slot]);

    GLuint textureType = _textureType[slot];

    glBindTexture(textureType, _textureId[slot]);
    

    //generate a new texture attachment
    //anisotropic filtering is only added to color attachments
    if (sampler.anisotropyLevel() > 1 && _mipMapEnabled[slot] && slot != TextureDescriptor::Depth) {
        GLint anisoLevel = std::min<GLint>((GLint)sampler.anisotropyLevel(), ParamHandler::getInstance().getParam<GLint>("rendering.anisotropicFilteringLevel"));
        glTexParameteri(textureType, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisoLevel);
    }

    //depth attachments may need comparison functions
    if(sampler._useRefCompare){
        glTexParameteri(textureType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTexParameteri(textureType, GL_TEXTURE_COMPARE_FUNC, glCompareFuncTable[sampler._cmpFunc]);
    }
  
    //General texture parameters for either color or depth
    glTexParameterf(textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[sampler.magFilter()]);
    glTexParameterf(textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[sampler.minFilter()]);
    glTexParameterf(textureType, GL_TEXTURE_WRAP_S, glWrapTable[sampler.wrapU()]);
    if (sampler.wrapU() == TEXTURE_CLAMP_TO_BORDER ||
        sampler.wrapV() == TEXTURE_CLAMP_TO_BORDER ||
        sampler.wrapW() == TEXTURE_CLAMP_TO_BORDER){
        glTexParameterfv(textureType, GL_TEXTURE_BORDER_COLOR, sampler.borderColor());
    }        

    //Anything other then a 1D texture, needs a T-wrap mode
    if (textureType != GL_TEXTURE_1D){
        glTexParameterf(textureType, GL_TEXTURE_WRAP_T, glWrapTable[sampler.wrapV()]);
    }

    //3D texture types need a R-wrap mode
    if (textureType == GL_TEXTURE_3D || textureType == GL_TEXTURE_CUBE_MAP || textureType == GL_TEXTURE_CUBE_MAP_ARRAY) {
        glTexParameterf(textureType, GL_TEXTURE_WRAP_R, glWrapTable[sampler.wrapW()]);
    }

    GLenum format = glImageFormatTable[texDescriptor._format];
    GLenum internalFormat = glImageFormatTable[texDescriptor._internalFormat];
    GLenum dataType = glDataFormat[texDescriptor._dataType];

    //generate empty texture data using each texture type's specific function
    switch(textureType){
        case GL_TEXTURE_1D:{
            glTexImage1D(GL_TEXTURE_1D, 0, internalFormat, _width, 0, format, dataType, NULL);
        }break;
        case GL_TEXTURE_2D_MULTISAMPLE:{
            glTexImage2DMultisample(textureType, GFX_DEVICE.MSAASamples(), internalFormat, _width, _height,GL_TRUE);
        }break;
        case GL_TEXTURE_2D:{
            glTexImage2D(textureType, 0, internalFormat, _width, _height, 0, format, dataType, NULL);
        }break;
        case GL_TEXTURE_CUBE_MAP:{
            for(GLubyte j=0; j<6; j++)
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, internalFormat, _width, _height, 0, format, dataType, NULL);
        }break;
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:{
                glTexImage3DMultisample(textureType, GFX_DEVICE.MSAASamples(), internalFormat, _width, _height, texDescriptor._layerCount, GL_TRUE);
        }break;
        case GL_TEXTURE_2D_ARRAY:
        case GL_TEXTURE_3D:{
                // Use _imageLayers as depth for GL_TEXTURE_3D
                glTexImage3D(textureType, 0, internalFormat, _width, _height, texDescriptor._layerCount, 0, format, dataType, NULL);
        }break;
        case GL_TEXTURE_CUBE_MAP_ARRAY: {
            assert(false); //not implemented yet
            for(GLubyte j=0; j<6; j++)
                glTexImage3D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, internalFormat, _width, _height, texDescriptor._layerCount, 0, format, dataType, NULL);
        }break;
    };
            
    //Generate mipmaps if needed (first call to glGenerateMipMap allocates all levels)
    if (_mipMapEnabled[slot]) {
        glTexParameteri(textureType, GL_TEXTURE_BASE_LEVEL, texDescriptor._mipMinLevel);
        if (!texDescriptor._mipMaxLevel){
            glTexParameteri(textureType, GL_TEXTURE_MAX_LEVEL, (GLint)floorf(log2f(fmaxf(_width, _height))));
        }else{
            glTexParameteri(textureType, GL_TEXTURE_MAX_LEVEL, texDescriptor._mipMaxLevel);
        }
        glGenerateMipmap(textureType);
    }
        
    //unbind the texture
    glBindTexture(textureType, 0);

    //Attach to frame buffer
    if (type == TextureDescriptor::Depth){
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _textureId[slot], 0);
        _isLayeredDepth = (currentType == TEXTURE_2D_ARRAY || currentType == TEXTURE_2D_ARRAY_MS || currentType == TEXTURE_CUBE_ARRAY || currentType == TEXTURE_3D);
    }else{
        
        _totalLayerCount += texDescriptor._layerCount;

        if(texDescriptor.isCubeTexture() && !_layeredRendering ){
            for (GLuint i = 0; i < 6; ++i){
                GLenum attachPoint = GL_COLOR_ATTACHMENT0 + slot + i;
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachPoint, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, _textureId[slot], 0);
                _colorBuffers.push_back(attachPoint);
            }
        }else if (texDescriptor._layerCount > 1 && !_layeredRendering){
            for (GLuint i = 0; i < texDescriptor._layerCount; ++i){
                GLenum attachPoint = GL_COLOR_ATTACHMENT0 + slot + i;
                glFramebufferTextureLayer(GL_FRAMEBUFFER, attachPoint, _textureId[slot], 0, i);
                _colorBuffers.push_back(attachPoint);
            }
        }else{ // if we require layered rendering, or have a non-layered / non-cubemap texture, attach it to a single binding point
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, _textureId[slot], 0);
            _colorBuffers.push_back(GL_COLOR_ATTACHMENT0 + slot);
        }
    }
}

void glFrameBuffer::AddDepthBuffer(){
        
    TextureDescriptor desc = _attachment[TextureDescriptor::Color0];
    TextureType texType = desc._type;
    if (texType == TEXTURE_2D_ARRAY)    texType = TEXTURE_2D;
    if (texType == TEXTURE_2D_ARRAY_MS) texType = TEXTURE_2D_MS;
    if (texType == TEXTURE_CUBE_ARRAY)  texType = TEXTURE_CUBE_MAP;

    GFXDataFormat dataType = desc._dataType;
    bool fpDepth = (dataType == FLOAT_16 || dataType == FLOAT_32);
    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TEXTURE_FILTER_NEAREST, TEXTURE_FILTER_NEAREST);
    screenSampler.setWrapMode(desc.getSampler().wrapU(), desc.getSampler().wrapV(), desc.getSampler().wrapW());
    screenSampler.toggleMipMaps(false);
    TextureDescriptor depthDescriptor(texType,
                                      DEPTH_COMPONENT,
                                      fpDepth ? DEPTH_COMPONENT32F : DEPTH_COMPONENT,
                                      fpDepth ? dataType : UNSIGNED_INT);
        
    screenSampler._useRefCompare = true; //< Use compare function
    screenSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal
    depthDescriptor.setSampler(screenSampler);
    _attachmentDirty[TextureDescriptor::Depth] = true;
    _attachment[TextureDescriptor::Depth] = depthDescriptor;
    InitAttachment(TextureDescriptor::Depth, depthDescriptor);

    _hasDepth = true;
}

bool glFrameBuffer::Create(GLushort width, GLushort height) {

    _totalLayerCount = 0;
    _hasDepth = false;
    _hasColor = false;
    _resolved = false;
    _isLayeredDepth = false;
    memset(_textureId, 0, 5 * sizeof(GLuint));
    memset(_textureType, 0, 5 * sizeof(GLuint));
    memset(_mipMapEnabled, false, 5 * sizeof(bool));

    if (_resolveBuffer){
        _resolveBuffer->_useDepthBuffer = _useDepthBuffer;
        _resolveBuffer->_disableColorWrites = _disableColorWrites;
        _resolveBuffer->_layeredRendering = _layeredRendering;
        _resolveBuffer->_clearColor.set(_clearColor);
        for (U8 i = 0; i < 5; ++i){
            if(_attachmentDirty[i] && !_resolveBuffer->AddAttachment(_attachment[i], (TextureDescriptor::AttachmentType)i))
                 return false;
        }
        _resolveBuffer->Create(width, height);
    }

    _clearBufferMask = 0;
    if(glGenerateMipmap == nullptr){
        ERROR_FN(Locale::get("ERROR_NO_MIP_MAPS"));
        assert(glGenerateMipmap);
    }

    if(_frameBufferHandle <= 0){
        glGenFramebuffers(1, &_frameBufferHandle);
    }
#ifdef _DEBUG
    else assert(_width != width && _height != height);
#endif

    _width = width;
    _height = height;

    GL_API::setActiveFB(_frameBufferHandle, true, true);

    _colorBuffers.resize(0);

    //For every attachment, be it a color or depth attachment ...
    for (U8 i = 0; i < 5; ++i){
        InitAttachment((TextureDescriptor::AttachmentType)i, _attachment[i]);
    }

    assert(_maxColorAttachments > (I32)_totalLayerCount);
    //If we either specify a depth texture or request a depth buffer ...
    if(_useDepthBuffer && !_hasDepth) AddDepthBuffer();

    //If color writes are disabled, draw only depth info
    if(_disableColorWrites){
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        _hasColor = false;
    }else{
        if (!_colorBuffers.empty())
            glDrawBuffers((GLsizei)_colorBuffers.size(), &_colorBuffers[0]);
    }

    if(_hasColor) _clearBufferMask = _clearBufferMask | GL_COLOR_BUFFER_BIT;
    if(_hasDepth) _clearBufferMask = _clearBufferMask | GL_DEPTH_BUFFER_BIT;

    checkStatus();

    GL_API::clearColor( _clearColor, _frameBufferHandle);

    glClear(_clearBufferMask);

    GL_API::setActiveFB(0, true, true, true);

    return true;
}

void glFrameBuffer::Destroy() {
    Unbind();

    for(GLuint& texture : _textureId){
        if (texture > 0){
            glDeleteTextures(1, &texture);
            texture = 0;
        }
    }

    if(_frameBufferHandle > 0){
        glDeleteFramebuffers(1, &_frameBufferHandle);
        _frameBufferHandle = 0;
    }

    _width = _height = 0;

    if (_resolveBuffer)
        _resolveBuffer->Destroy();
}

void glFrameBuffer::resolve(){
    if (_resolveBuffer && !_resolved){
        _resolveBuffer->BlitFrom(this, TextureDescriptor::Color0, _hasColor, _hasDepth);
        _resolved = true;
   }
}

void glFrameBuffer::BlitFrom(FrameBuffer* inputFB, TextureDescriptor::AttachmentType slot, bool blitColor, bool blitDepth) {
    if (!blitColor && !blitDepth)
        return;

    glFrameBuffer* input = dynamic_cast<glFrameBuffer*>(inputFB);
    
    if (_resolveBuffer && (inputFB->getGUID() != _resolveBuffer->getGUID())) // prevent stack overflow
        input->resolve();

    GLuint previousFB = GL_API::getActiveFB();
    GL_API::setActiveFB(input->_frameBufferHandle, true, false, true);
    GL_API::setActiveFB(this->_frameBufferHandle, false, true, true);

    if (blitColor && _hasColor){
        glDrawBuffer(this->_colorBuffers[0]);
        glReadBuffer(input->_colorBuffers[0]);
        glBlitFramebuffer(0, 0, input->_width, input->_height, 0, 0, this->_width, this->_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        for (U8 i = 1; i < this->_colorBuffers.size(); ++i){
            GL_API::setActiveFB(this->_frameBufferHandle, false, true, true);
            glDrawBuffer(this->_colorBuffers[i]);
            GL_API::setActiveFB(input->_frameBufferHandle, true, false, true);
            glReadBuffer(input->_colorBuffers[i]);
            glBlitFramebuffer(0, 0, input->_width, input->_height, 0, 0, this->_width, this->_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        
        _mipMapsDirty = true;
    }
    if (blitDepth && _hasDepth)
        glBlitFramebuffer(0, 0, input->_width, input->_height, 0, 0, this->_width, this->_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    GL_API::setActiveFB(previousFB, true, true, true);
}

void glFrameBuffer::Bind(GLubyte unit, TextureDescriptor::AttachmentType slot) {
    if (_resolveBuffer){
        resolve();
        _resolveBuffer->Bind(unit, slot);
        return;
    }

    FrameBuffer::Bind(unit, slot);
    GL_API::setActiveTextureUnit(unit);
    glSamplerObject::Unbind(unit);

    glBindTexture(_textureType[slot], _textureId[slot]);

    glTexture::textureBoundMap[unit] = std::make_pair(_textureId[slot], _textureType[slot]);
}

void glFrameBuffer::Unbind(GLubyte unit) const {
    if (_resolveBuffer){
        _resolveBuffer->Unbind(unit);
        return;
    }
    FrameBuffer::Unbind(unit);
    GL_API::setActiveTextureUnit(unit);

    GLenum textureType = glTexture::textureBoundMap[unit].second;
    if (textureType != GL_NONE){
        glBindTexture(textureType, 0);
        glTexture::textureBoundMap[unit] = std::make_pair(0, GL_NONE);
    }
}

void glFrameBuffer::Begin(const FrameBufferTarget& drawPolicy) {
    assert(_frameBufferHandle != 0);
   
    if(_viewportChanged){
        GL_API::restoreViewport();
        _viewportChanged = false;
    }

    GL_API::setViewport(vec4<GLint>(0, 0, _width, _height));
    _viewportChanged = true;

    GL_API::setActiveFB(_frameBufferHandle, false, true);
    // this is checked so it isn't called twice on the GPU
    GL_API::clearColor(_clearColor, _frameBufferHandle);
    if (_clearBuffersState && drawPolicy._clearBuffersOnBind)   glClear(_clearBufferMask);
    if(!drawPolicy._depthOnly) _mipMapsDirty = true;

    if (_resolveBuffer)
        _resolved = false;
}

void glFrameBuffer::End() {
    GL_API::setActiveFB(0, true, true);
    GL_API::restoreViewport();
    _viewportChanged = false;

    resolve();
}

void glFrameBuffer::DrawToLayer(TextureDescriptor::AttachmentType slot, U8 layer, bool includeDepth) const {
    GLuint textureType = _textureType[slot];
    // only for array textures (it's better to simply ignore the command if the format isn't supported (debugging reasons)
    if(textureType != GL_TEXTURE_2D_ARRAY && textureType != GL_TEXTURE_CUBE_MAP_ARRAY && textureType != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
        return;
    
    bool useDepthLayer = (_hasDepth && includeDepth) || (_hasDepth && slot == TextureDescriptor::Depth);
    bool useColorLayer = (_hasColor && slot < TextureDescriptor::Depth);

    if (useDepthLayer && _isLayeredDepth)  glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _textureId[TextureDescriptor::Depth], 0, layer);
    if (useColorLayer)  glDrawBuffer(_colorBuffers[layer]);

    if(_clearBuffersState){
        if (useDepthLayer){
            if (useColorLayer)
                glClear(_clearBufferMask);
            else
                glClear(GL_DEPTH_BUFFER_BIT);
        }else{
            if(useColorLayer)
                glClear(GL_COLOR_BUFFER_BIT);
        }
    }
}

void glFrameBuffer::UpdateMipMaps(TextureDescriptor::AttachmentType slot) const {
    if(!_mipMapEnabled[slot] || !_bound || !_mipMapsDirty)
        return;

    glGenerateMipmap(_textureType[slot]);
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
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:{
        ERROR_FN(Locale::get("ERROR_FB_INCOMPLETE_LAYER_TARGETS"));
        return false;
    }
    default:{
        ERROR_FN(Locale::get("ERROR_UNKNOWN"));
        return false;
        }
    };
}