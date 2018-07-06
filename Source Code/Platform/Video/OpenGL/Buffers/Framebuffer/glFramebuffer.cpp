#include "Headers/glFramebuffer.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Textures/Headers/glSamplerObject.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

bool glFramebuffer::_bufferBound = false;
bool glFramebuffer::_viewportChanged = false;

namespace {
typedef TextureDescriptor::AttachmentType AttachmentType;

const char* getAttachmentName(AttachmentType type) {
    switch (type) {
        case AttachmentType::Depth:
            return "Depth";
        case AttachmentType::Color0:
            return "Color0";
        case AttachmentType::Color1:
            return "Color1";
        case AttachmentType::Color2:
            return "Color2";
        case AttachmentType::Color3:
            return "Color3";
    }
    return "ERROR_TYPE";
}
};

glFramebuffer::glFramebuffer(bool useResolveBuffer)
    : Framebuffer(useResolveBuffer),
      _resolveBuffer(useResolveBuffer ? MemoryManager_NEW glFramebuffer()
                                      : nullptr),
      _hasDepth(false),
      _hasColor(false),
      _resolved(false),
      _isLayeredDepth(false)
{
    _attOffset.fill(0);
}

glFramebuffer::~glFramebuffer() {
    Destroy();
}

void glFramebuffer::InitAttachment(AttachmentType type,
                                   const TextureDescriptor& texDescriptor) {
    // If it changed
    if (!_attachmentDirty[to_uint(type)]) {
        return;
    }

    DIVIDE_ASSERT(_width != 0 && _height != 0,
                  "glFramebuffer error: Invalid frame buffer dimensions!");

    if (type != AttachmentType::Depth) {
        _hasColor = true;
    } else {
        _hasDepth = true;
    }

    U8 slot = to_uint(type);

    TextureType currentType = texDescriptor._type;

    if (_multisampled) {
        if (currentType == TextureType::TEXTURE_2D) {
            currentType = TextureType::TEXTURE_2D_MS;
        }
        if (currentType == TextureType::TEXTURE_2D_ARRAY) {
            currentType = TextureType::TEXTURE_2D_ARRAY_MS;
        }
    } else {
        if (currentType == TextureType::TEXTURE_2D_MS) {
            currentType = TextureType::TEXTURE_2D;
        }
        if (currentType == TextureType::TEXTURE_2D_ARRAY_MS) {
            currentType = TextureType::TEXTURE_2D_ARRAY;
        }
    }

    bool isLayeredTexture = (currentType == TextureType::TEXTURE_2D_ARRAY ||
                             currentType == TextureType::TEXTURE_2D_ARRAY_MS ||
                             currentType == TextureType::TEXTURE_CUBE_ARRAY ||
                             currentType == TextureType::TEXTURE_3D);

    SamplerDescriptor sampler = texDescriptor.getSampler();
    GFXImageFormat internalFormat = texDescriptor._internalFormat;
    GFXImageFormat format = texDescriptor._baseFormat;

    if (sampler.srgb()) {
        if (internalFormat == GFXImageFormat::RGBA8) {
            internalFormat = GFXImageFormat::SRGBA8;
        }
        if (internalFormat == GFXImageFormat::RGB8) {
            internalFormat = GFXImageFormat::SRGB8;
        }
    } else {
        if (internalFormat == GFXImageFormat::SRGBA8) {
            internalFormat = GFXImageFormat::RGBA8;
        }
        if (internalFormat == GFXImageFormat::SRGB8) {
            internalFormat = GFXImageFormat::RGB8;
        }
    }
    if (_multisampled) {
        sampler.toggleMipMaps(false);
    }

    if (_attachmentTexture[slot]) {
        RemoveResource(_attachmentTexture[slot]);
    }

    stringImpl attachmentName = Util::StringFormat(
        "Framebuffer_Att_%s_%d", getAttachmentName(type), getGUID());

    ResourceDescriptor textureAttachment(attachmentName);
    textureAttachment.setThreadedLoading(false);
    textureAttachment.setPropertyDescriptor(sampler);
    textureAttachment.setEnumValue(to_uint(currentType));
    _attachmentTexture[slot] = CreateResource<Texture>(textureAttachment);

    Texture* tex = _attachmentTexture[slot];
    tex->setNumLayers(texDescriptor._layerCount);
    _mipMapLevel[slot].set(
        texDescriptor._mipMinLevel,
        texDescriptor._mipMaxLevel > 0
            ? texDescriptor._mipMaxLevel
            : 1 + (I16)floorf(log2f(fmaxf((F32)_width, (F32)_height))));

    tex->loadData(
        isLayeredTexture
            ? 0
            : to_uint(GLUtil::glTextureTypeTable[to_uint(
                  currentType)]),
        NULL, vec2<U16>(_width, _height), _mipMapLevel[slot], format,
        internalFormat);

    tex->refreshMipMaps();

    // Attach to frame buffer
    if (type == AttachmentType::Depth) {
        GLUtil::DSAWrapper::dsaNamedFramebufferTexture(
            _framebufferHandle, GL_DEPTH_ATTACHMENT, tex->getHandle(), 0);
        _isLayeredDepth = isLayeredTexture;
    } else {
        GLint offset = 0;
        if (slot > to_uint(AttachmentType::Color0)) {
            offset = _attOffset[slot - 1];
        }
        if (texDescriptor.isCubeTexture() && !_layeredRendering) {
            for (GLuint i = 0; i < 6; ++i) {
                GLenum attachPoint = GL_COLOR_ATTACHMENT0 + (i + offset);
                GLUtil::DSAWrapper::dsaNamedFramebufferTexture(
                    _framebufferHandle, attachPoint, tex->getHandle(), i);
                _colorBuffers.push_back(attachPoint);
            }
            if (slot > 0) {
                _attOffset[slot] = _attOffset[slot - 1] + 6;
            }
        } else if (texDescriptor._layerCount > 1 && !_layeredRendering) {
            for (GLuint i = 0; i < texDescriptor._layerCount; ++i) {
                GLenum attachPoint = GL_COLOR_ATTACHMENT0 + (i + offset);
                GLUtil::DSAWrapper::dsaNamedFramebufferTextureLayer(
                    _framebufferHandle, attachPoint, tex->getHandle(), 0, i);
                _colorBuffers.push_back(attachPoint);
            }
            if (slot > 0) {
                _attOffset[slot] = _attOffset[slot - 1] + texDescriptor._layerCount;
            }
            // If we require layered rendering, or have a non-layered /
            // non-cubemap texture, attach it to a single binding point
        } else {
            GLUtil::DSAWrapper::dsaNamedFramebufferTexture(
                _framebufferHandle,
                GL_COLOR_ATTACHMENT0 + static_cast<GLuint>(slot),
                tex->getHandle(), 0);
            _colorBuffers.push_back(GL_COLOR_ATTACHMENT0 +
                                    static_cast<GLuint>(slot));
        }
    }
}

void glFramebuffer::AddDepthBuffer() {
    TextureDescriptor desc = _attachment[to_uint(AttachmentType::Color0)];
    TextureType texType = desc._type;

    if (texType == TextureType::TEXTURE_2D_ARRAY) {
        texType = TextureType::TEXTURE_2D;
    }
    if (texType == TextureType::TEXTURE_2D_ARRAY_MS) {
        texType = TextureType::TEXTURE_2D_MS;
    }
    if (texType == TextureType::TEXTURE_CUBE_ARRAY) {
        texType = TextureType::TEXTURE_CUBE_MAP;
    }

    GFXDataFormat dataType = desc._dataType;
    bool fpDepth = (dataType == GFXDataFormat::FLOAT_16 ||
                    dataType == GFXDataFormat::FLOAT_32);
    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TextureFilter::NEAREST);
    screenSampler.setWrapMode(desc.getSampler().wrapU(),
                              desc.getSampler().wrapV(),
                              desc.getSampler().wrapW());
    
    TextureDescriptor depthDescriptor(
        texType, fpDepth ? GFXImageFormat::DEPTH_COMPONENT32F
                         : GFXImageFormat::DEPTH_COMPONENT,
        fpDepth ? dataType : GFXDataFormat::UNSIGNED_INT);

    screenSampler._useRefCompare = true;  //< Use compare function
    screenSampler._cmpFunc =
        ComparisonFunction::LEQUAL;  //< Use less or equal
    depthDescriptor.setSampler(screenSampler);
    _attachmentDirty[to_uint(AttachmentType::Depth)] = true;
    _attachment[to_uint(AttachmentType::Depth)] = depthDescriptor;
    InitAttachment(AttachmentType::Depth, depthDescriptor);

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
        for (U8 i = 0; i < to_uint(AttachmentType::COUNT); ++i) {
            if (_attachmentDirty[i] &&
                !_resolveBuffer->AddAttachment(_attachment[i],
                                               (AttachmentType)i)) {
                return false;
            }
        }
        _resolveBuffer->Create(width, height);
    }

    if (_framebufferHandle <= 0) {
        GLUtil::DSAWrapper::dsaCreateFramebuffers(1, &_framebufferHandle);
    }

    if (Config::Profile::USE_2x2_TEXTURES) {
        _width = _height = 2;
    } else {
        _width = width;
        _height = height;
    }

    _colorBuffers.resize(0);

    // For every attachment, be it a color or depth attachment ...
    for (U8 i = 0; i < to_uint(AttachmentType::COUNT); ++i) {
        InitAttachment(static_cast<AttachmentType>(i), _attachment[i]);
    }
    // If we either specify a depth texture or request a depth buffer ...
    if (_useDepthBuffer && !_hasDepth) {
        AddDepthBuffer();
    }
    // If color writes are disabled, draw only depth info
    if (_disableColorWrites) {
        GLUtil::DSAWrapper::dsaNamedFramebufferDrawBuffer(_framebufferHandle,
                                                          GL_NONE);
        GLUtil::DSAWrapper::dsaNamedFramebufferReadBuffer(_framebufferHandle,
                                                          GL_NONE);
        _hasColor = false;
    } else {
        if (!_colorBuffers.empty()) {
            GLUtil::DSAWrapper::dsaNamedFramebufferDrawBuffers(
                _framebufferHandle, 
                static_cast<GLsizei>(_colorBuffers.size()),
                _colorBuffers.data());
        }
    }

    if (!_hasColor && _hasDepth) {
        _clearBufferMask = GL_DEPTH_BUFFER_BIT;
    } else if (_hasColor && !_hasDepth) {
        _clearBufferMask = GL_COLOR_BUFFER_BIT;
    } else {
        _clearBufferMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    }

    return checkStatus();
}

void glFramebuffer::Destroy() {
    for (Texture* texture : _attachmentTexture) {
        RemoveResource(texture);
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
        _resolveBuffer->BlitFrom(this, AttachmentType::Color0, _hasColor,
                                 _hasDepth);
        _resolved = true;
    }
}

void glFramebuffer::BlitFrom(Framebuffer* inputFB,
                             AttachmentType slot,
                             bool blitColor,
                             bool blitDepth) {
    if (!inputFB || (!blitColor && !blitDepth)) {
        return;
    }

    glFramebuffer* input = static_cast<glFramebuffer*>(inputFB);

    // prevent stack overflow
    if (_resolveBuffer && (inputFB->getGUID() != _resolveBuffer->getGUID())) {
        input->resolve();
    }

    GLuint previousFB =
        GL_API::getActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE);
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_ONLY,
                        input->_framebufferHandle);
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_WRITE_ONLY,
                        this->_framebufferHandle);

    if (blitColor && _hasColor) {
        for (U8 i = 0; i < this->_colorBuffers.size(); ++i) {
            glDrawBuffer(this->_colorBuffers[i]);
            glReadBuffer(input->_colorBuffers[i]);
            glBlitFramebuffer(0, 0, input->_width, input->_height, 0, 0,
                              this->_width, this->_height, GL_COLOR_BUFFER_BIT,
                              GL_NEAREST);
            GFX_DEVICE.registerDrawCall();
        }
    }

    if (blitDepth && _hasDepth) {
        glBlitFramebuffer(0, 0, input->_width, input->_height, 0, 0,
                          this->_width, this->_height, GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
        GFX_DEVICE.registerDrawCall();
    }

    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE,
                        previousFB);
}

Texture* glFramebuffer::GetAttachment(AttachmentType slot) {
    if (_resolveBuffer) {
        resolve();
        return _resolveBuffer->GetAttachment(slot);
    }

    return Framebuffer::GetAttachment(slot);
}

void glFramebuffer::Bind(GLubyte unit, AttachmentType slot) {
    GetAttachment(slot)->Bind(unit);
}

void glFramebuffer::ResetMipMaps(FramebufferTarget::BufferMask mask) {
    switch (mask) {
        case FramebufferTarget::BufferMask::DEPTH: {
            U32 idx = to_uint(AttachmentType::Depth);
            if (_attachmentTexture[idx] != nullptr) {
                _attachmentTexture[idx]->refreshMipMaps();
            }
        }break;
        case FramebufferTarget::BufferMask::COLOR: {
            U32 idx = 0;
            for (; idx < to_uint(AttachmentType::COUNT); ++idx) {
                if (_attachmentTexture[idx] != nullptr &&
                    static_cast<AttachmentType>(idx) != AttachmentType::Depth)
                {
                    _attachmentTexture[idx]->refreshMipMaps();
                }
            }
             to_uint(TextureDescriptor::AttachmentType::Color0);
        }break;
        case FramebufferTarget::BufferMask::BOTH: {
            ResetMipMaps(FramebufferTarget::BufferMask::DEPTH);
            ResetMipMaps(FramebufferTarget::BufferMask::COLOR);
        }break;
    };
}

void glFramebuffer::Begin(const FramebufferTarget& drawPolicy) {
    DIVIDE_ASSERT(_framebufferHandle != 0,
                  "glFramebuffer error: "
                  "Tried to bind an invalid framebuffer!");
    DIVIDE_ASSERT(!glFramebuffer::_bufferBound,
                  "glFramebuffer error: "
                  "Begin() called without a call to the "
                  "previous bound buffer's End()");

    if (drawPolicy._changeViewport) {
        _viewportChanged = true;
        GFX_DEVICE.setViewport(vec4<GLint>(0, 0, _width, _height));
    }

    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, _framebufferHandle);

    // this is checked so it isn't called twice on the GPU
    GL_API::clearColor(_clearColor, _prevClearColor);

    if (_clearBuffersState && drawPolicy._clearBuffersOnBind) {
        if (drawPolicy._drawMask == FramebufferTarget::BufferMask::DEPTH &&
            _hasDepth) {
            glClear(GL_DEPTH_BUFFER_BIT);
        } else if (drawPolicy._drawMask ==
                       FramebufferTarget::BufferMask::COLOR &&
                   _hasColor) {
            glClear(GL_COLOR_BUFFER_BIT);
        } else {
            glClear(_clearBufferMask);
        }
        GFX_DEVICE.registerDrawCall();
    }

    ResetMipMaps(drawPolicy._drawMask);

    if (_resolveBuffer) {
        _resolved = false;
    }

    glFramebuffer::_bufferBound = true;
}

void glFramebuffer::End() {
    DIVIDE_ASSERT(glFramebuffer::_bufferBound,
                  "glFramebuffer error: "
                  "End() called without a previous call to Begin()");

    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, 0);
    if (_viewportChanged) {
        GFX_DEVICE.restoreViewport();
        _viewportChanged = false;
    }
    resolve();

    GL_API::clearColor(_prevClearColor);
    glFramebuffer::_bufferBound = false;
}

void glFramebuffer::DrawToLayer(TextureDescriptor::AttachmentType slot,
                                U8 layer,
                                bool includeDepth) {
    DIVIDE_ASSERT(slot < TextureDescriptor::AttachmentType::COUNT,
                  "glFrameBuffer::DrawToLayer Error: invalid slot received!");

    GLenum textureType = GLUtil::glTextureTypeTable[to_uint(
        _attachmentTexture[to_uint(slot)]->getTextureType())];
    // only for array textures (it's better to simply ignore the command if the
    // format isn't supported (debugging reasons)
    if (textureType != GL_TEXTURE_2D_ARRAY &&
        textureType != GL_TEXTURE_CUBE_MAP_ARRAY &&
        textureType != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        return;
    }

    bool useDepthLayer =
        (_hasDepth && includeDepth) ||
        (_hasDepth && slot == TextureDescriptor::AttachmentType::Depth);
    bool useColorLayer =
        (_hasColor && slot < TextureDescriptor::AttachmentType::Depth);

    if (useDepthLayer && _isLayeredDepth) {
        GLUtil::DSAWrapper::dsaNamedFramebufferTextureLayer(_framebufferHandle,
            GL_DEPTH_ATTACHMENT, _attachmentTexture[to_uint(TextureDescriptor::AttachmentType::Depth)]->getHandle(),
            0, layer);
    }

    if (useColorLayer) {
        GLint offset = slot > TextureDescriptor::AttachmentType::Color0
                           ? _attOffset[to_uint(slot) - 1]
                           : 0;
        GLUtil::DSAWrapper::dsaNamedFramebufferDrawBuffer(_framebufferHandle, _colorBuffers[layer + offset]);
    }

    _attachmentTexture[to_uint(slot)]->refreshMipMaps();

    if (_clearBuffersState) {
        if (useDepthLayer) {
            useColorLayer ? glClear(_clearBufferMask)
                          : glClear(GL_DEPTH_BUFFER_BIT);
        } else {
            if (useColorLayer) {
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }
        GFX_DEVICE.registerDrawCall();
    }
}

void glFramebuffer::SetMipLevel(GLushort mipLevel,
                                GLushort mipMaxLevel,
                                GLushort writeLevel,
                                TextureDescriptor::AttachmentType slot) {
    GLenum textureType = GLUtil::glTextureTypeTable[to_uint(
        _attachmentTexture[to_uint(slot)]->getTextureType())];
    // Only 2D texture support for now
    DIVIDE_ASSERT(textureType == GL_TEXTURE_2D,
                  "glFramebuffer error: Changing mip level is only available "
                  "for 2D textures!");
    _attachmentTexture[to_uint(slot)]->setMipMapRange(mipLevel, mipMaxLevel);
    glFramebufferTexture(
        GL_FRAMEBUFFER, slot == TextureDescriptor::AttachmentType::Depth
                            ? GL_DEPTH_ATTACHMENT
                            : _colorBuffers[to_uint(slot)],
        _attachmentTexture[to_uint(slot)]->getHandle(), writeLevel);
}

void glFramebuffer::ResetMipLevel(TextureDescriptor::AttachmentType slot) {
    SetMipLevel(_mipMapLevel[to_uint(slot)].x, _mipMapLevel[to_uint(slot)].y, 0,
                slot);
}

void glFramebuffer::ReadData(const vec4<U16>& rect,
                             GFXImageFormat imageFormat,
                             GFXDataFormat dataType,
                             void* outData) {
    if (_resolveBuffer) {
        resolve();
        _resolveBuffer->ReadData(rect, imageFormat, dataType, outData);
    } else {
        GL_API::setPixelPackUnpackAlignment();
        GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_ONLY, _framebufferHandle);
        glReadPixels(
            rect.x, rect.y, rect.z, rect.w,
            GLUtil::glImageFormatTable[to_uint(imageFormat)],
            GLUtil::glDataFormat[to_uint(dataType)], outData);
    }
}

bool glFramebuffer::checkStatus() const {
    // check FB status
    switch (GLUtil::DSAWrapper::dsaCheckNamedFramebufferStatus(
        _framebufferHandle, GL_FRAMEBUFFER))
    {
        case GL_FRAMEBUFFER_COMPLETE: {
            return true;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: {
            Console::errorfn(Locale::get("ERROR_FB_ATTACHMENT_INCOMPLETE"));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: {
            Console::errorfn(Locale::get("ERROR_FB_NO_IMAGE"));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: {
            Console::errorfn(Locale::get("ERROR_FB_INCOMPLETE_DRAW_BUFFER"));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: {
            Console::errorfn(Locale::get("ERROR_FB_INCOMPLETE_READ_BUFFER"));
            return false;
        }
        case GL_FRAMEBUFFER_UNSUPPORTED: {
            Console::errorfn(Locale::get("ERROR_FB_UNSUPPORTED"));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: {
            Console::errorfn(Locale::get("ERROR_FB_INCOMPLETE_MULTISAMPLE"));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: {
            Console::errorfn(Locale::get("ERROR_FB_INCOMPLETE_LAYER_TARGETS"));
            return false;
        }
        case glext::GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT: {
            Console::errorfn(Locale::get("ERROR_FB_DIMENSIONS"));
            return false;
        }
        case glext::GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT: {
             Console::errorfn(Locale::get("ERROR_FB_FORMAT"));
             return false;
        }
        default: {
            Console::errorfn(Locale::get("ERROR_UNKNOWN"));
            return false;
        }
    };
}

};  // namespace Divide