#include "Headers/glFramebuffer.h"
#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/OpenGL/Textures/Headers/glSamplerObject.h"

#include "core.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Headers/GFXDevice.h"

namespace Divide {

bool glFramebuffer::_bufferBound = false;
bool glFramebuffer::_viewportChanged = false;

namespace {
    const char* getAttachmentName(TextureDescriptor::AttachmentType type) {
        switch(type) {
            case TextureDescriptor::Depth  : return "Depth"; 
            case TextureDescriptor::Color0 : return "Color0";
            case TextureDescriptor::Color1 : return "Color1";
            case TextureDescriptor::Color2 : return "Color2";
            case TextureDescriptor::Color3 : return "Color3";
        }
        return "ERROR_TYPE";
    }

    U8 getAttachmentSlot(TextureDescriptor::AttachmentType type) {
         switch(type) {
            case TextureDescriptor::Depth  : return 4; 
            case TextureDescriptor::Color0 : return 0;
            case TextureDescriptor::Color1 : return 1;
            case TextureDescriptor::Color2 : return 2;
            case TextureDescriptor::Color3 : return 3;
         }
        return 255;
    }

    TextureDescriptor::AttachmentType getAttachmentType(U8 slot) {
         switch(slot) {
            case 4 : return TextureDescriptor::Depth; 
            case 0 : return TextureDescriptor::Color0;
            case 1 : return TextureDescriptor::Color1;
            case 2 : return TextureDescriptor::Color2;
            case 3 : return TextureDescriptor::Color3;
         }
        return TextureDescriptor::AttachmentType_PLACEHOLDER;
    }
};

glFramebuffer::glFramebuffer(glFramebuffer* resolveBuffer) : Framebuffer(resolveBuffer != nullptr),
                                                             _resolveBuffer(resolveBuffer),
                                                             _hasDepth(false),
                                                             _hasColor(false),
                                                             _resolved(false),
                                                             _isLayeredDepth(false)
{
    for (U8 i = 0; i < TextureDescriptor::AttachmentType_PLACEHOLDER; ++i) {
        _mipMapLevel[i].set(0);
        _attOffset[i]   = 0;
    }
}

glFramebuffer::~glFramebuffer()
{
    SAFE_DELETE(_resolveBuffer);
    Destroy();
}

void glFramebuffer::InitAttachment(TextureDescriptor::AttachmentType type, const TextureDescriptor& texDescriptor) {
    //If it changed
    if (!_attachmentDirty[type]) {
        return;
    }

    DIVIDE_ASSERT(_width != 0 && _height != 0, "glFramebuffer error: Invalid frame buffer dimensions!");
    
    if (type != TextureDescriptor::Depth) {
        _hasColor = true;
    } else {
        _hasDepth = true;
    }

    U8 slot = getAttachmentSlot(type);

    TextureType currentType = texDescriptor._type;

    if (_multisampled){
        if (currentType == TEXTURE_2D) {
            currentType = TEXTURE_2D_MS;
        }
        if (currentType == TEXTURE_2D_ARRAY) {
            currentType = TEXTURE_2D_ARRAY_MS;
        }
    } else {
        if (currentType == TEXTURE_2D_MS) {
            currentType = TEXTURE_2D;
        }
        if (currentType == TEXTURE_2D_ARRAY_MS) {
            currentType = TEXTURE_2D_ARRAY;
        }
    }
    
    bool isLayeredTexture = (currentType == TEXTURE_2D_ARRAY || currentType == TEXTURE_2D_ARRAY_MS || currentType == TEXTURE_CUBE_ARRAY || currentType == TEXTURE_3D);

    SamplerDescriptor sampler = texDescriptor.getSampler();
    if (_multisampled) {
        sampler.toggleMipMaps(false);
    }

    if (_attachmentTexture[slot]) {
        RemoveResource(_attachmentTexture[slot]);
    }
    std::string attachmentName("Framebuffer_Att_");
    attachmentName.append(getAttachmentName(type));
    attachmentName.append(Util::toString(getGUID()));
    ResourceDescriptor textureAttachment(attachmentName);
    textureAttachment.setThreadedLoading(false);
    textureAttachment.setPropertyDescriptor(sampler);
    textureAttachment.setEnumValue(currentType);
    _attachmentTexture[slot] = CreateResource<Texture>(textureAttachment);
    
    Texture* tex = _attachmentTexture[slot];
    tex->setNumLayers(texDescriptor._layerCount);
    _mipMapLevel[slot].set(texDescriptor._mipMinLevel,
                           texDescriptor._mipMaxLevel > 0 ? texDescriptor._mipMaxLevel : 1 + (I16)floorf(log2f(fmaxf((F32)_width, (F32)_height))));
    
    tex->loadData(isLayeredTexture ? 0 : GLUtil::GL_ENUM_TABLE::glTextureTypeTable[currentType], NULL, vec2<U16>(_width, _height), _mipMapLevel[slot], texDescriptor._internalFormat, texDescriptor._internalFormat);
    tex->refreshMipMaps();
    tex->Bind(0); 

               
    //Attach to frame buffer
    if (type == TextureDescriptor::Depth) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex->getHandle(), 0);
        _isLayeredDepth = isLayeredTexture;
    } else {
        GLint offset = 0;
        if (slot > getAttachmentSlot(TextureDescriptor::Color0)) {
            offset = _attOffset[slot - 1];
        }
        if (texDescriptor.isCubeTexture() && !_layeredRendering) {
            for (GLuint i = 0; i < 6; ++i) {
                GLenum attachPoint = GL_COLOR_ATTACHMENT0 + i + offset;
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachPoint, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, tex->getHandle(), 0);
                _colorBuffers.push_back(attachPoint);
            }
            _attOffset[slot] = _attOffset[slot - 1] + 6;
        } else if (texDescriptor._layerCount > 1 && !_layeredRendering) {
            for (GLuint i = 0; i < texDescriptor._layerCount; ++i) {
                GLenum attachPoint = GL_COLOR_ATTACHMENT0 + i + offset;
                glFramebufferTextureLayer(GL_FRAMEBUFFER, attachPoint, tex->getHandle(), 0, i);
                _colorBuffers.push_back(attachPoint);
            }
            _attOffset[slot] = _attOffset[slot - 1] + texDescriptor._layerCount;
        } else { // if we require layered rendering, or have a non-layered / non-cubemap texture, attach it to a single binding point
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, tex->getHandle(), 0);
            _colorBuffers.push_back(GL_COLOR_ATTACHMENT0 + slot);
        }
    }
}

void glFramebuffer::AddDepthBuffer() {
    TextureDescriptor desc = _attachment[TextureDescriptor::Color0];
    TextureType texType = desc._type;

    if (texType == TEXTURE_2D_ARRAY) {
        texType = TEXTURE_2D;
    }
    if (texType == TEXTURE_2D_ARRAY_MS) {
        texType = TEXTURE_2D_MS;
    }
    if (texType == TEXTURE_CUBE_ARRAY) {
        texType = TEXTURE_CUBE_MAP;
    }

    GFXDataFormat dataType = desc._dataType;
    bool fpDepth = (dataType == FLOAT_16 || dataType == FLOAT_32);
    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TEXTURE_FILTER_NEAREST, TEXTURE_FILTER_NEAREST);
    screenSampler.setWrapMode(desc.getSampler().wrapU(), desc.getSampler().wrapV(), desc.getSampler().wrapW());
    screenSampler.toggleMipMaps(false);
    TextureDescriptor depthDescriptor(texType,
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

bool glFramebuffer::Create(GLushort width, GLushort height) {
    _hasDepth = false;
    _hasColor = false;
    _resolved = false;
    _isLayeredDepth = false;

    if (_resolveBuffer) {
        _resolveBuffer->_useDepthBuffer = _useDepthBuffer;
        _resolveBuffer->_disableColorWrites = _disableColorWrites;
        _resolveBuffer->_layeredRendering = _layeredRendering;
        _resolveBuffer->_clearColor.set(_clearColor);
        for (U8 i = 0; i < TextureDescriptor::AttachmentType_PLACEHOLDER; ++i) {
            if(_attachmentDirty[i] && !_resolveBuffer->AddAttachment(_attachment[i], (TextureDescriptor::AttachmentType)i)) {
                 return false;
            }
        }
        _resolveBuffer->Create(width, height);
    }

    _clearBufferMask = 0;

    if (_framebufferHandle <= 0) {
        glGenFramebuffers(1, &_framebufferHandle);
    }

    if (Config::Profile::USE_2x2_TEXTURES) {
        _width = _height = 2;
    } else {
        _width = width;
        _height = height;
    }

    GL_API::setActiveFB(_framebufferHandle, Framebuffer::FB_READ_WRITE);

    _colorBuffers.resize(0);

    //For every attachment, be it a color or depth attachment ...
    for (U8 i = 0; i < TextureDescriptor::AttachmentType_PLACEHOLDER; ++i) {
        InitAttachment((TextureDescriptor::AttachmentType)i, _attachment[i]);
    }
    //If we either specify a depth texture or request a depth buffer ...
    if (_useDepthBuffer && !_hasDepth) {
        AddDepthBuffer();
    }
    //If color writes are disabled, draw only depth info
    if (_disableColorWrites) {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        _hasColor = false;
    } else {
        if (!_colorBuffers.empty()) {
            glDrawBuffers((GLsizei)_colorBuffers.size(), _colorBuffers.data());
        }
    }

    if (_hasColor) {
        _clearBufferMask = _clearBufferMask | GL_COLOR_BUFFER_BIT;
    }

    if (_hasDepth) {
        _clearBufferMask = _clearBufferMask | GL_DEPTH_BUFFER_BIT;
    }

    checkStatus();

    GL_API::clearColor( _clearColor );

    glClear(_clearBufferMask);

    GL_API::setActiveFB(0, Framebuffer::FB_READ_WRITE);

    return true;
}

void glFramebuffer::Destroy() {
    
    for (U8 i = 0; i < TextureDescriptor::AttachmentType_PLACEHOLDER; ++i) {
        if (_attachmentTexture[i]) {
            RemoveResource(_attachmentTexture[i]);
        }
    }

    if (_framebufferHandle > 0) {
        glDeleteFramebuffers(1, &_framebufferHandle);
        _framebufferHandle = 0;
    }

    _width = _height = 0;

    if (_resolveBuffer) {
        _resolveBuffer->Destroy();
    }
}

void glFramebuffer::resolve() {
    if (_resolveBuffer && !_resolved) {
        _resolveBuffer->BlitFrom(this, TextureDescriptor::Color0, _hasColor, _hasDepth);
        _resolved = true;
    }
}

void glFramebuffer::BlitFrom(Framebuffer* inputFB, TextureDescriptor::AttachmentType slot, bool blitColor, bool blitDepth) {
    if (!blitColor && !blitDepth) {
        return;
    }

    glFramebuffer* input = dynamic_cast<glFramebuffer*>(inputFB);
    
    // prevent stack overflow
    if (_resolveBuffer && (inputFB->getGUID() != _resolveBuffer->getGUID())) {
        input->resolve();
    }

    GLuint previousFB = GL_API::getActiveFB(Framebuffer::FB_READ_WRITE);
    GL_API::setActiveFB(input->_framebufferHandle, Framebuffer::FB_READ_ONLY);
    GL_API::setActiveFB(this->_framebufferHandle,  Framebuffer::FB_WRITE_ONLY);

    if (blitColor && _hasColor) {
        for (U8 i = 0; i < this->_colorBuffers.size(); ++i) {
            glDrawBuffer(this->_colorBuffers[i]);
            glReadBuffer(input->_colorBuffers[i]);
            glBlitFramebuffer(0, 0, input->_width, input->_height, 0, 0, this->_width, this->_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            GFX_DEVICE.registerDrawCall();
        }
    }

    if (blitDepth && _hasDepth) {
        glBlitFramebuffer(0, 0, input->_width, input->_height, 0, 0, this->_width, this->_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        GFX_DEVICE.registerDrawCall();
    }

    GL_API::setActiveFB(previousFB, Framebuffer::FB_READ_WRITE);
}

void glFramebuffer::Bind(GLubyte unit, TextureDescriptor::AttachmentType slot) {
    if (_resolveBuffer) {
        resolve();
        _resolveBuffer->Bind(unit, slot);
    } else {
        _attachmentTexture[slot]->Bind(unit);
    }
}

void glFramebuffer::Begin(const FramebufferTarget& drawPolicy) {
    DIVIDE_ASSERT(_framebufferHandle != 0, "glFramebuffer error: Tried to bind and invalid framebuffer!");
    DIVIDE_ASSERT(!glFramebuffer::_bufferBound, "glFramebuffer error: Begin() called without a call to the previous bound buffer's End()");

    if (drawPolicy._changeViewport) {
        GFX_DEVICE.setViewport(vec4<GLint>(0, 0, _width, _height));
        _viewportChanged = true;
    }

    GL_API::setActiveFB(_framebufferHandle, Framebuffer::FB_WRITE_ONLY);
    // this is checked so it isn't called twice on the GPU
    GL_API::clearColor( _clearColor );
    if (_clearBuffersState && drawPolicy._clearBuffersOnBind) {
        glClear(_clearBufferMask);
        GFX_DEVICE.registerDrawCall();
    }

    if (!drawPolicy._depthOnly && !drawPolicy._colorOnly) {
        for (U8 i = 0; i < TextureDescriptor::AttachmentType_PLACEHOLDER; ++i) {
            if (_attachmentTexture[i]) {
                _attachmentTexture[i]->refreshMipMaps();
            }
        }
    } else if(drawPolicy._depthOnly) {
        _attachmentTexture[TextureDescriptor::Depth]->refreshMipMaps();
    } else {
        for(U8 i = 0; i < TextureDescriptor::AttachmentType_PLACEHOLDER - 1; ++i) {
            if (_attachmentTexture[i]) {
                _attachmentTexture[i]->refreshMipMaps();
            }
        }
    }

    if (_resolveBuffer) {
        _resolved = false;
    }

    glFramebuffer::_bufferBound = true;
}

void glFramebuffer::End() {
    DIVIDE_ASSERT(glFramebuffer::_bufferBound, "glFramebuffer error: End() called without a previous call to Begin()");

    GL_API::setActiveFB(0, Framebuffer::FB_READ_WRITE);
    if (_viewportChanged) {
        GFX_DEVICE.restoreViewport();
        _viewportChanged = false;
    }
    resolve();

    glFramebuffer::_bufferBound = false;
}

void glFramebuffer::DrawToLayer(TextureDescriptor::AttachmentType slot, U8 layer, bool includeDepth) {
    GLuint textureType = GLUtil::GL_ENUM_TABLE::glTextureTypeTable[_attachmentTexture[slot]->getTextureType()];
    // only for array textures (it's better to simply ignore the command if the format isn't supported (debugging reasons)
    if (textureType != GL_TEXTURE_2D_ARRAY && 
        textureType != GL_TEXTURE_CUBE_MAP_ARRAY && 
        textureType != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        return;
    }

    bool useDepthLayer = (_hasDepth && includeDepth) || (_hasDepth && slot == TextureDescriptor::Depth);
    bool useColorLayer = (_hasColor && slot < TextureDescriptor::Depth);

    if (useDepthLayer && _isLayeredDepth) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _attachmentTexture[TextureDescriptor::Depth]->getHandle(), 0, layer);
    }

    if (useColorLayer) {
        GLint offset = slot > TextureDescriptor::Color0 ? _attOffset[slot - 1] : 0;
        glDrawBuffer(_colorBuffers[layer + offset]);
    }

    _attachmentTexture[slot]->refreshMipMaps();

    if (_clearBuffersState) {
        if (useDepthLayer) {
            useColorLayer ? glClear(_clearBufferMask) : glClear(GL_DEPTH_BUFFER_BIT);
        } else {
            if (useColorLayer) {
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }
        GFX_DEVICE.registerDrawCall();
    }
}

void glFramebuffer::SetMipLevel(GLushort mipLevel,  GLushort mipMaxLevel, GLushort writeLevel, TextureDescriptor::AttachmentType slot) {
    GLuint textureType = GLUtil::GL_ENUM_TABLE::glTextureTypeTable[_attachmentTexture[slot]->getTextureType()];
    // Only 2D texture support for now
    DIVIDE_ASSERT(textureType == GL_TEXTURE_2D, "glFramebuffer error: Changing mip level is only available for 2D textures!");
    _attachmentTexture[slot]->setMipMapRange(mipLevel, mipLevel);
    glFramebufferTexture(GL_FRAMEBUFFER,
                         slot == TextureDescriptor::Depth ? GL_DEPTH_ATTACHMENT : _colorBuffers[slot],
                         _attachmentTexture[slot]->getHandle(),
                         writeLevel);
}

void glFramebuffer::ResetMipLevel(TextureDescriptor::AttachmentType slot) {
    SetMipLevel(_mipMapLevel[slot].x, _mipMapLevel[slot].y, 0, slot);
}

void glFramebuffer::ReadData(const vec4<U16>& rect, GFXImageFormat imageFormat, GFXDataFormat dataType, void* outData) {
    if (_resolveBuffer) {
        resolve();
        _resolveBuffer->ReadData(rect, imageFormat, dataType, outData);
    } else {
        GL_API::setActiveFB(_framebufferHandle, Framebuffer::FB_READ_ONLY);
        glReadPixels(rect.x, rect.y, rect.z, rect.w, GLUtil::GL_ENUM_TABLE::glImageFormatTable[imageFormat], 
                                                     GLUtil::GL_ENUM_TABLE::glDataFormat[dataType], outData);
    }
}

bool glFramebuffer::checkStatus() const {
    // check FB status
    switch(glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
        case GL_FRAMEBUFFER_COMPLETE : {
            return true;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT : {
            ERROR_FN(Locale::get("ERROR_FB_ATTACHMENT_INCOMPLETE"));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT : {
            ERROR_FN(Locale::get("ERROR_FB_NO_IMAGE"));
            return false;
        }
        case 0x8CD9 /*GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS*/ : {
            ERROR_FN(Locale::get("ERROR_FB_DIMENSIONS"));
            return false;
                                                  }
        case 0x8CDA /*GL_FRAMEBUFFER_INCOMPLETE_FORMATS*/ : {
            ERROR_FN(Locale::get("ERROR_FB_FORMAT"));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER : {
            ERROR_FN(Locale::get("ERROR_FB_INCOMPLETE_DRAW_BUFFER"));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER : {
            ERROR_FN(Locale::get("ERROR_FB_INCOMPLETE_READ_BUFFER"));
            return false;
        }
        case GL_FRAMEBUFFER_UNSUPPORTED : {
            ERROR_FN(Locale::get("ERROR_FB_UNSUPPORTED"));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE : {
            ERROR_FN(Locale::get("ERROR_FB_INCOMPLETE_MULTISAMPLE"));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS : {
            ERROR_FN(Locale::get("ERROR_FB_INCOMPLETE_LAYER_TARGETS"));
            return false;
        }
        default : {
            ERROR_FN(Locale::get("ERROR_UNKNOWN"));
            return false;
        }
    };
}

}; //namespace Divide