#include "Headers/glFramebuffer.h"

#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Textures/Headers/glSamplerObject.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

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

glFramebuffer::glFramebuffer(GFXDevice& context, bool useResolveBuffer)
    : Framebuffer(context, useResolveBuffer),
      _resolveBuffer(useResolveBuffer ? MemoryManager_NEW glFramebuffer(context)
                                      : nullptr),
      _hasDepth(false),
      _hasColor(false),
      _resolved(false),
      _isLayeredDepth(false),
      _isCreated(false)
{
    _attOffset.fill(0);
    _attDirty.fill(false);
    _previousMask = FramebufferTarget::BufferMask::BOTH;
}

glFramebuffer::~glFramebuffer() {
    destroy();
}

void glFramebuffer::initAttachment(AttachmentType type,
                                   const TextureDescriptor& texDescriptor,
                                   bool resize) {
    // If it changed
    if (!_attachmentChanged[to_uint(type)]) {
        return;
    }


    DIVIDE_ASSERT(_width != 0 && _height != 0,
                  "glFramebuffer error: Invalid frame buffer dimensions!");

    if (type != AttachmentType::Depth) {
        _hasColor = true;
    } else {
        _hasDepth = true;
    }

    I32 slot = to_int(type);

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
                             currentType == TextureType::TEXTURE_CUBE_MAP ||
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

    if (_attachmentTexture[slot] && !resize) {
        RemoveResource(_attachmentTexture[slot]);
    }

    if (!resize) {
        stringImpl attachmentName = Util::StringFormat(
            "Framebuffer_Att_%s_%d", getAttachmentName(type), getGUID());

        ResourceDescriptor textureAttachment(attachmentName);
        textureAttachment.setThreadedLoading(false);
        textureAttachment.setPropertyDescriptor(sampler);
        textureAttachment.setEnumValue(to_uint(currentType));
        _attachmentTexture[slot] = CreateResource<Texture>(textureAttachment);
    }

    Texture* tex = _attachmentTexture[slot];
    assert(tex != nullptr);
    tex->setNumLayers(texDescriptor._layerCount);
    _mipMapLevel[slot].set(0,
                           sampler.generateMipMaps() 
                              ? 1 + (I16)floorf(log2f(fmaxf(to_float(_width), to_float(_height))))
                              : 1);
    if (resize) {
        tex->resize(NULL, vec2<U16>(_width, _height), _mipMapLevel[slot]);
        return;
    } else {
        Texture::TextureLoadInfo info;
        info._type = currentType;
        tex->loadData(info, NULL, vec2<U16>(_width, _height), _mipMapLevel[slot], format, internalFormat);
    }

    // Attach to frame buffer
    I32 offset = 0;
    GLenum attachment;
    if (type == AttachmentType::Depth) {
        attachment = GL_DEPTH_ATTACHMENT;
        _isLayeredDepth = isLayeredTexture;
    } else {
        if (slot > 0) {
            offset = _attOffset[slot - 1];
        }
        attachment = GL_COLOR_ATTACHMENT0 + offset;
    }
    _attOffset[slot] = offset + 1;

    _attachments[to_uint(type)] = std::make_pair(attachment, tex->getHandle());
    _attachmentChanged[slot] = false;
    _attDirty[slot] = true;
    if (type != AttachmentType::Depth) {
        _colorBuffers.push_back(attachment);
    }
}

void glFramebuffer::toggleAttachment(TextureDescriptor::AttachmentType type, bool state) {
    std::pair<GLenum, U32> att = _attachments[to_uint(type)];
    if (att.second != 0) {
        glNamedFramebufferTexture(_framebufferHandle, att.first, state ? att.second : 0, 0);
    }
}

void glFramebuffer::removeDepthBuffer() {

    toggleAttachment(AttachmentType::Depth, false);
    if (_attachmentTexture[to_uint(AttachmentType::Depth)]) {
        RemoveResource(_attachmentTexture[to_uint(AttachmentType::Depth)]);
    }

    _attachmentChanged[to_uint(AttachmentType::Depth)] = false;
    _attachments[to_uint(AttachmentType::Depth)].second = 0;
    _isLayeredDepth = false;
    _hasDepth = false;
}

void glFramebuffer::addDepthBuffer() {
    TextureDescriptor desc = _attachment[to_uint(AttachmentType::Color0)];
    TextureType texType = desc._type;

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
    depthDescriptor.setLayerCount(desc._layerCount);
    _attachmentChanged[to_uint(AttachmentType::Depth)] = true;
    _attachment[to_uint(AttachmentType::Depth)] = depthDescriptor;
    initAttachment(AttachmentType::Depth, depthDescriptor, false);

    _hasDepth = true;
}

bool glFramebuffer::create(U16 width, U16 height) {
    if (_resolveBuffer) {
        _resolveBuffer->_useDepthBuffer = _useDepthBuffer;
        _resolveBuffer->_disableColorWrites = _disableColorWrites;
        _resolveBuffer->_clearColor.set(_clearColor);
        for (U8 i = 0; i < to_uint(AttachmentType::COUNT); ++i) {
            if (_attachmentChanged[i] && !_resolveBuffer->addAttachment(_attachment[i], (AttachmentType)i)) {
                return false;
            }
        }
        _resolveBuffer->create(width, height);
    }

    bool shouldResize = false;
    if (Config::Profile::USE_2x2_TEXTURES) {
        _width = _height = 2;
    } else {
        shouldResize = (_width != width || _height != height);
        _width = width;
        _height = height;
    }

    _isCreated = _isCreated && !_shouldRebuild;

    if (!_isCreated) {
        assert(_framebufferHandle == 0);
        glCreateFramebuffers(1, &_framebufferHandle);
        shouldResize = false;
        _hasDepth = false;
        _hasColor = false;
        _resolved = false;
        _isLayeredDepth = false;
        _colorBuffers.resize(0);
    }


    // For every attachment, be it a color or depth attachment ...
    for (U8 i = 0; i < to_uint(AttachmentType::COUNT); ++i) {
        initAttachment(static_cast<AttachmentType>(i), _attachment[i], shouldResize);
    }
    
    // If we either specify a depth texture or request a depth buffer ...
    if (_useDepthBuffer && !_hasDepth) {
        addDepthBuffer();
    }
    
    if (!shouldResize) {
        setInitialAttachments();
    }

    // If color writes are disabled, draw only depth info
    if (_disableColorWrites) {
        glNamedFramebufferDrawBuffer(_framebufferHandle, GL_NONE);
        glNamedFramebufferReadBuffer(_framebufferHandle, GL_NONE);
        _hasColor = false;
    } else {
        if (!_colorBuffers.empty()) {
            glNamedFramebufferDrawBuffers(_framebufferHandle, 
                                          static_cast<GLsizei>(_colorBuffers.size()),
                                          _colorBuffers.data());
        }
    }

    clear();

    _isCreated = checkStatus();
    _shouldRebuild = !_isCreated;
    return _isCreated;
}

void glFramebuffer::destroy() {
    for (Texture* texture : _attachmentTexture) {
        RemoveResource(texture);
    }

    if (_framebufferHandle > 0) {
        glDeleteFramebuffers(1, &_framebufferHandle);
        _framebufferHandle = 0;
    }

    _width = _height = 0;

    if (_resolveBuffer) {
        _resolveBuffer->destroy();
    }
}

void glFramebuffer::resolve() {
    if (_resolveBuffer && !_resolved) {
        _resolveBuffer->blitFrom(this, AttachmentType::Color0, _hasColor,
                                 _hasDepth);
        _resolved = true;
    }
}

void glFramebuffer::blitFrom(Framebuffer* inputFB,
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

    GLuint previousFB = GL_API::getActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE);
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_ONLY, input->_framebufferHandle);
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_WRITE_ONLY, this->_framebufferHandle);

    if (blitColor && _hasColor) {
        size_t colorCount = _colorBuffers.size();
        for (U8 i = 0; i < colorCount; ++i) {
            glDrawBuffer(this->_colorBuffers[i]);
            glReadBuffer(input->_colorBuffers[i]);
            glBlitFramebuffer(0, 0, input->_width, input->_height, 0, 0,
                              this->_width, this->_height, GL_COLOR_BUFFER_BIT,
                              GL_NEAREST);
        }
        _context.registerDrawCalls(to_uint(colorCount));
    }

    if (blitDepth && _hasDepth) {
        glBlitFramebuffer(0, 0, input->_width, input->_height, 0, 0,
                          this->_width, this->_height, GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
        _context.registerDrawCall();
    }

    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, previousFB);
}

Texture* glFramebuffer::getAttachment(AttachmentType slot, bool flushStateOnRequest) {
    if (_resolveBuffer) {
        resolve();
        return _resolveBuffer->getAttachment(slot, flushStateOnRequest);
    }

    return Framebuffer::getAttachment(slot, flushStateOnRequest);
}

void glFramebuffer::bind(U8 unit, AttachmentType slot, bool flushStateOnRequest) {
    getAttachment(slot, flushStateOnRequest)->Bind(unit, flushStateOnRequest);
}

void glFramebuffer::resetMipMaps(FramebufferTarget::BufferMask mask) {
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
            resetMipMaps(FramebufferTarget::BufferMask::DEPTH);
            resetMipMaps(FramebufferTarget::BufferMask::COLOR);
        }break;
    };
}

void glFramebuffer::begin(const FramebufferTarget& drawPolicy) {
    DIVIDE_ASSERT(_framebufferHandle != 0,
                  "glFramebuffer error: "
                  "Tried to bind an invalid framebuffer!");
    DIVIDE_ASSERT(!glFramebuffer::_bufferBound,
                  "glFramebuffer error: "
                  "Begin() called without a call to the "
                  "previous bound buffer's End()");

    if (drawPolicy._changeViewport) {
        _viewportChanged = true;
        _context.setViewport(vec4<GLint>(0, 0, _width, _height));
    }

    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, _framebufferHandle);

    if (drawPolicy._clearBuffersOnBind) {
        clear();
    }

    resetMipMaps(drawPolicy._drawMask);

    if (_resolveBuffer) {
        _resolved = false;
    }

    if (_previousMask != drawPolicy._drawMask) {
        switch (drawPolicy._drawMask) {
            case FramebufferTarget::BufferMask::DEPTH: {
                for (U32 i = 0; i < _attachments.size(); ++i) {
                    AttachmentType type = static_cast<AttachmentType>(i);
                    if (type != AttachmentType::Depth) {
                        toggleAttachment(type, false);
                    }
                }
                
                if (_previousMask != FramebufferTarget::BufferMask::BOTH) {
                    toggleAttachment(AttachmentType::Depth, true);
                }
            } break;
            case FramebufferTarget::BufferMask::COLOR: {
                for (U32 i = 0; i < _attachments.size(); ++i) {
                    AttachmentType type = static_cast<AttachmentType>(i);
                    if (type != AttachmentType::Depth) {
                        toggleAttachment(type, true);
                    }
                }
                toggleAttachment(AttachmentType::Depth, false);
            } break;
            case FramebufferTarget::BufferMask::BOTH: {
                if (_previousMask == FramebufferTarget::BufferMask::DEPTH) {
                    for (U32 i = 0; i < _attachments.size(); ++i) {
                        AttachmentType type = static_cast<AttachmentType>(i);
                        if (type != AttachmentType::Depth) {
                            toggleAttachment(type, true);
                        }
                    }
                } else {
                    toggleAttachment(AttachmentType::Depth, true);
                }
            };
        };
        checkStatus();
        _previousMask = drawPolicy._drawMask;
    }

    glFramebuffer::_bufferBound = true;
}

void glFramebuffer::end() {
    DIVIDE_ASSERT(glFramebuffer::_bufferBound,
                  "glFramebuffer error: "
                  "End() called without a previous call to Begin()");

    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, 0);
    if (_viewportChanged) {
        _context.restoreViewport();
        _viewportChanged = false;
    }

    setInitialAttachments();
    resolve();
    glFramebuffer::_bufferBound = false;
}

void glFramebuffer::setInitialAttachments() {
    // Reset attachments if they changed (e.g. after layered rendering);
    for (U32 i = 0; i < _attDirty.size(); ++i) {
        if (_attDirty[i]) {
            toggleAttachment(static_cast<AttachmentType>(i), true);
        }
    }
    _attDirty.fill(false);
}

void glFramebuffer::clear() const {
    if (_hasColor) {
        GLuint index = 0;
        for (; index < _colorBuffers.size(); ++index) {
            glClearNamedFramebufferfv(_framebufferHandle, GL_COLOR, index++, _clearColor._v);
        }
        _context.registerDrawCalls(index);
    }
    if (_hasDepth) {
        glClearNamedFramebufferfv(_framebufferHandle, GL_DEPTH, 0, &_depthValue);
        _context.registerDrawCall();
    }

    
}

void glFramebuffer::drawToLayer(TextureDescriptor::AttachmentType slot,
                                U32 layer,
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
        glNamedFramebufferTextureLayer(_framebufferHandle,
                                       GL_DEPTH_ATTACHMENT,
                                       _attachmentTexture[to_uint(TextureDescriptor::AttachmentType::Depth)]->getHandle(),
                                       0,
                                       layer);
        _attDirty[to_uint(TextureDescriptor::AttachmentType::Depth)] = true;
    }

    if (useColorLayer) {
        U32 offset = 0;
        if (to_uint(slot) > 0) {
            offset = _attOffset[to_uint(slot) - 1];
        }
        glNamedFramebufferTextureLayer(_framebufferHandle,
                                       _colorBuffers[offset],
                                       _attachmentTexture[to_uint(slot)]->getHandle(),
                                       0,
                                       layer);
        _attDirty[to_uint(slot)] = true;
    }

    _attachmentTexture[to_uint(slot)]->refreshMipMaps();

    checkStatus();
}

void glFramebuffer::setMipLevel(U16 mipLevel, U16 mipMaxLevel, U16 writeLevel,
                                TextureDescriptor::AttachmentType slot) {
    GLenum textureType = GLUtil::glTextureTypeTable[to_uint(
        _attachmentTexture[to_uint(slot)]->getTextureType())];
    // Only 2D texture support for now
    DIVIDE_ASSERT(textureType == GL_TEXTURE_2D || textureType == GL_TEXTURE_2D_MULTISAMPLE,
                  "glFramebuffer error: Changing mip level is only available "
                  "for 2D textures!");
    _attachmentTexture[to_uint(slot)]->setMipMapRange(mipLevel, mipMaxLevel);
    glFramebufferTexture(
        GL_FRAMEBUFFER, slot == TextureDescriptor::AttachmentType::Depth
                            ? GL_DEPTH_ATTACHMENT
                            : _colorBuffers[to_uint(slot)],
        _attachmentTexture[to_uint(slot)]->getHandle(), writeLevel);

    checkStatus();
}

void glFramebuffer::resetMipLevel(TextureDescriptor::AttachmentType slot) {
    setMipLevel(_mipMapLevel[to_uint(slot)].x, _mipMapLevel[to_uint(slot)].y, 0, slot);
}

void glFramebuffer::readData(const vec4<U16>& rect,
                             GFXImageFormat imageFormat,
                             GFXDataFormat dataType,
                             void* outData) {
    if (_resolveBuffer) {
        resolve();
        _resolveBuffer->readData(rect, imageFormat, dataType, outData);
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
#if defined(ENABLE_GPU_VALIDATION)
    // check FB status
    switch (glCheckNamedFramebufferStatus(_framebufferHandle, GL_FRAMEBUFFER))
    {
        case GL_FRAMEBUFFER_COMPLETE: {
            return true;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: {
            Console::errorfn(Locale::get(_ID("ERROR_FB_ATTACHMENT_INCOMPLETE")));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: {
            Console::errorfn(Locale::get(_ID("ERROR_FB_NO_IMAGE")));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: {
            Console::errorfn(Locale::get(_ID("ERROR_FB_INCOMPLETE_DRAW_BUFFER")));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: {
            Console::errorfn(Locale::get(_ID("ERROR_FB_INCOMPLETE_READ_BUFFER")));
            return false;
        }
        case GL_FRAMEBUFFER_UNSUPPORTED: {
            Console::errorfn(Locale::get(_ID("ERROR_FB_UNSUPPORTED")));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: {
            Console::errorfn(Locale::get(_ID("ERROR_FB_INCOMPLETE_MULTISAMPLE")));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: {
            Console::errorfn(Locale::get(_ID("ERROR_FB_INCOMPLETE_LAYER_TARGETS")));
            return false;
        }
        case glext::GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT: {
            Console::errorfn(Locale::get(_ID("ERROR_FB_DIMENSIONS")));
            return false;
        }
        case glext::GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT: {
             Console::errorfn(Locale::get(_ID("ERROR_FB_FORMAT")));
             return false;
        }
        default: {
            Console::errorfn(Locale::get(_ID("ERROR_UNKNOWN")));
            return false;
        }
    };
#else
    return true;
#endif
}

};  // namespace Divide