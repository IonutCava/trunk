#include "config.h"

#include "Headers/glFramebuffer.h"

#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Textures/Headers/glSamplerObject.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

namespace {
    const char* getAttachmentName(RTAttachment::Type type) {
        switch(type) {
            case RTAttachment::Type::Colour  : return "Colour";
            case RTAttachment::Type::Depth   : return "Depth";
            case RTAttachment::Type::Stencil : return "Stencil";
        };

        return "ERROR";
    };
};

bool glFramebuffer::_bufferBound = false;
bool glFramebuffer::_viewportChanged = false;
bool glFramebuffer::_zWriteEnabled = true;

IMPLEMENT_CUSTOM_ALLOCATOR(glFramebuffer, 0, 0)
glFramebuffer::glFramebuffer(GFXDevice& context, bool useResolveBuffer)
    : RenderTarget(context, useResolveBuffer),
      _resolveBuffer(useResolveBuffer ? MemoryManager_NEW glFramebuffer(context)
                                      : nullptr),
      _resolved(false),
      _isLayeredDepth(false),
      _isCreated(false),
      _framebufferHandle(0)
{
    _previousMask.enableAll();

    I32 maxColourAttachments = ParamHandler::instance().getParam<I32>(_ID("rendering.maxRenderTargetOutputs"), 32);
    assert(maxColourAttachments > RenderTarget::g_maxColourAttachments);
    ACKNOWLEDGE_UNUSED(maxColourAttachments);
    
    _attachments.init<glRTAttachment>(RenderTarget::g_maxColourAttachments);
}

glFramebuffer::~glFramebuffer() {
    destroy();
    MemoryManager::DELETE(_resolveBuffer);
}

void glFramebuffer::initAttachment(RTAttachment::Type type,
                                   U8 index,
                                   TextureDescriptor& texDescriptor,
                                   bool resize) {
    const RTAttachment_ptr& attachment = _attachments.get(type, index);
    // If it changed
    bool shouldResize = resize && attachment->used();
    if (!attachment->changed() && !shouldResize) {
        return;
    }

    assert(_width != 0 && _height != 0 && "glFramebuffer error: Invalid frame buffer dimensions!");

    if (_multisampled) {
        if (texDescriptor._type == TextureType::TEXTURE_2D) {
            texDescriptor._type = TextureType::TEXTURE_2D_MS;
        }
        if (texDescriptor._type == TextureType::TEXTURE_2D_ARRAY) {
            texDescriptor._type = TextureType::TEXTURE_2D_ARRAY_MS;
        }
    } else {
        if (texDescriptor._type == TextureType::TEXTURE_2D_MS) {
            texDescriptor._type = TextureType::TEXTURE_2D;
        }
        if (texDescriptor._type == TextureType::TEXTURE_2D_ARRAY_MS) {
            texDescriptor._type = TextureType::TEXTURE_2D_ARRAY;
        }
    }

    if (texDescriptor.getSampler().srgb()) {
        if (texDescriptor._internalFormat == GFXImageFormat::RGBA8) {
            texDescriptor._internalFormat = GFXImageFormat::SRGBA8;
        }
        if (texDescriptor._internalFormat == GFXImageFormat::RGB8) {
            texDescriptor._internalFormat = GFXImageFormat::SRGB8;
        }
    } else {
        if (texDescriptor._internalFormat == GFXImageFormat::SRGBA8) {
            texDescriptor._internalFormat = GFXImageFormat::RGBA8;
        }
        if (texDescriptor._internalFormat == GFXImageFormat::SRGB8) {
            texDescriptor._internalFormat = GFXImageFormat::RGB8;
        }
    }

    if (_multisampled) {
        texDescriptor.getSampler().toggleMipMaps(false);
    }

    glRTAttachment* att = static_cast<glRTAttachment*>(attachment.get());

    bool newTexture = false;
    Texture_ptr tex = nullptr;
    if (!att->asTexture()) {
        stringImpl attachmentName = Util::StringFormat(
            "Framebuffer_Att_%s_%d_%d", getAttachmentName(type), index, getGUID());

        ResourceDescriptor textureAttachment(attachmentName);
        textureAttachment.setThreadedLoading(false);
        textureAttachment.setPropertyDescriptor(texDescriptor.getSampler());
        textureAttachment.setEnumValue(to_uint(texDescriptor._type));
        tex = CreateResource<Texture>(textureAttachment);
        assert(tex);
        att->setTexture(tex);
        newTexture = true;
    }
    
    att->mipMapLevel(0,
                     texDescriptor.getSampler().generateMipMaps()
                         ? 1 + (I16)floorf(log2f(fmaxf(to_float(_width), to_float(_height))))
                         : 1);

    tex->setNumLayers(texDescriptor._layerCount);
    tex->lockAutomaticMipMapGeneration(!texDescriptor.automaticMipMapGeneration());

    if (resize) {
        tex->resize(NULL, vec2<U16>(_width, _height), att->mipMapLevel());
    } else {
        if (newTexture) {
            Texture::TextureLoadInfo info;
            info._type = texDescriptor._type;
            tex->loadData(info, texDescriptor, NULL, vec2<U16>(_width, _height), att->mipMapLevel());
        }
    }

    // Attach to frame buffer
    GLenum attachmentEnum;
    if (type == RTAttachment::Type::Depth) {
        attachmentEnum = GL_DEPTH_ATTACHMENT;
        _isLayeredDepth = (texDescriptor._type == TextureType::TEXTURE_2D_ARRAY ||
            texDescriptor._type == TextureType::TEXTURE_2D_ARRAY_MS ||
            texDescriptor._type == TextureType::TEXTURE_CUBE_MAP ||
            texDescriptor._type == TextureType::TEXTURE_CUBE_ARRAY ||
            texDescriptor._type == TextureType::TEXTURE_3D);
    } else {
        attachmentEnum = GLenum((U32)GL_COLOR_ATTACHMENT0 + index);
    }

    att->setInfo(attachmentEnum, tex->getHandle());
    att->clearRefreshFlag();
    att->flagDirty();
    att->enabled(true);
}

void glFramebuffer::toggleAttachment(RTAttachment::Type type, U8 index, bool state) {
    glRTAttachment* att = static_cast<glRTAttachment*>(_attachments.get(type, index).get());
    std::pair<GLenum, U32> info = att->getInfo();
    if (info.second != 0 && state != att->toggledState()) {
        glNamedFramebufferTexture(_framebufferHandle, info.first, state ? info.second : 0, 0);
        att->toggledState(state);
    }
}

void glFramebuffer::addDepthBuffer() {
    TextureDescriptor& desc = _attachments.get(RTAttachment::Type::Colour, 0)->descriptor();

    TextureType texType = desc._type;

    GFXDataFormat dataType = desc.dataType();
    bool fpDepth = (dataType == GFXDataFormat::FLOAT_16 ||
                    dataType == GFXDataFormat::FLOAT_32);

    const SamplerDescriptor& srcDesc = desc.getSampler();

    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TextureFilter::NEAREST);
    screenSampler.setWrapMode(srcDesc.wrapU(), srcDesc.wrapV(), srcDesc.wrapW());
    
    TextureDescriptor depthDescriptor(
        texType,
        fpDepth ? GFXImageFormat::DEPTH_COMPONENT32F
                : GFXImageFormat::DEPTH_COMPONENT,
        fpDepth ? GFXDataFormat::FLOAT_32 
                : GFXDataFormat::UNSIGNED_INT);

    //screenSampler._useRefCompare = true;  //< Use compare function
    screenSampler._cmpFunc = ComparisonFunction::LEQUAL;  //< Use less or equal
    screenSampler.toggleMipMaps(srcDesc.generateMipMaps());

    depthDescriptor.setSampler(screenSampler);
    depthDescriptor.setLayerCount(desc._layerCount);
    _attachments.get(RTAttachment::Type::Depth, 0)->fromDescriptor(depthDescriptor);
    initAttachment(RTAttachment::Type::Depth, 0, _attachments.get(RTAttachment::Type::Depth, 0)->descriptor(), false);
}

bool glFramebuffer::create(U16 width, U16 height) {
    if (_resolveBuffer) {
        _resolveBuffer->_useDepthBuffer = _useDepthBuffer;

        for (U8 i = 0; i < to_const_ubyte(RTAttachment::Type::COUNT); ++i) {
            RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
            for (U8 j = 0; j < _attachments.attachmentCount(type); ++j) {
                const RTAttachment_ptr& att = _attachments.get(type, j);
                if (att->changed()) {
                    _resolveBuffer->_attachments.get(type, j)->fromDescriptor(att->descriptor());
                }
                _resolveBuffer->_attachments.get(type, j)->clearColour(att->clearColour());
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
        _resolved = false;
        _isLayeredDepth = false;

    if (Config::ENABLE_GPU_VALIDATION) {
        // label this FB to be able to tell that it's internally created and nor from a 3rd party lib
        glObjectLabel(GL_FRAMEBUFFER,
                      _framebufferHandle,
                      -1,
                      Util::StringFormat("DVD_FB_%d", _framebufferHandle).c_str());
    }

    }


    vectorImpl<GLenum> colorBuffsers;
    // For every attachment, be it a colour or depth attachment ...
    for (U8 i = 0; i < to_const_ubyte(RTAttachment::Type::COUNT); ++i) {
        RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
        for (U8 j = 0; j < _attachments.attachmentCount(type); ++j) {
            const RTAttachment_ptr& att = _attachments.get(type, j);
            initAttachment(static_cast<RTAttachment::Type>(i), j, att->descriptor(), shouldResize);
            if (type == RTAttachment::Type::Colour && att->enabled()) {
                colorBuffsers.push_back(static_cast<glRTAttachment*>(att.get())->getInfo().first);
            }
        }
    }
    
    // If we either specify a depth texture or request a depth buffer ...
    if (_useDepthBuffer && !hasDepth()) {
        addDepthBuffer();
    }
    
    if (!shouldResize) {
        setInitialAttachments();
    }

    // If colour writes are disabled, draw only depth info
    if (!colorBuffsers.empty()) {
        glNamedFramebufferDrawBuffers(_framebufferHandle, 
                                      static_cast<GLsizei>(colorBuffsers.size()),
                                      colorBuffsers.data());
    }

    _isCreated = checkStatus();
    if (_isCreated) {
        clear(RenderTarget::defaultPolicy());
    }
    _shouldRebuild = !_isCreated;
    return _isCreated;
}

void glFramebuffer::destroy() {
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
        _resolveBuffer->blitFrom(this, hasColour(), hasDepth());
        _resolved = true;
    }
}

void glFramebuffer::blitFrom(RenderTarget* inputFB,
                             bool blitColour,
                             bool blitDepth ) {
    if (!inputFB || (!blitColour && !blitDepth)) {
        return;
    }

    glFramebuffer* input = static_cast<glFramebuffer*>(inputFB);

    // prevent stack overflow
    if (_resolveBuffer && (inputFB->getGUID() != _resolveBuffer->getGUID())) {
        input->resolve();
    }

    GLuint previousFB = GL_API::getActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE);
    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_ONLY, input->_framebufferHandle);
    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY, this->_framebufferHandle);

    if (blitColour && hasColour()) {
        RTAttachment::Type type = RTAttachment::Type::Colour;
        U8 colourCount = _attachments.attachmentCount(type);
        for (U8 j = 0; j < colourCount; ++j) {
            glRTAttachment* thisAtt = static_cast<glRTAttachment*>(_attachments.get(type, j).get());
            glRTAttachment* inputAtt = static_cast<glRTAttachment*>(input->_attachments.get(type, j).get());
            if (thisAtt->enabled()) {
                glDrawBuffer(thisAtt->getInfo().first);
                glReadBuffer(inputAtt->getInfo().first);
                glBlitFramebuffer(0,
                                  0,
                                  input->_width,
                                  input->_height,
                                  0,
                                  0,
                                  this->_width,
                                  this->_height,
                                  GL_COLOR_BUFFER_BIT,
                                  GL_NEAREST);
            }
            _context.registerDrawCall();
        }
    }

    if (blitDepth && hasDepth()) {
        glBlitFramebuffer(0,
                          0,
                          input->_width,
                          input->_height,
                          0,
                          0,
                          this->_width,
                          this->_height,
                          GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
        _context.registerDrawCall();
    }

    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, previousFB);
}

void glFramebuffer::blitFrom(RenderTarget* inputFB,
                             U8 index,
                             bool blitColour,
                             bool blitDepth) {
    if (!inputFB || (!blitColour && !blitDepth)) {
        return;
    }
    
    glFramebuffer* input = static_cast<glFramebuffer*>(inputFB);

    // prevent stack overflow
    if (_resolveBuffer && (inputFB->getGUID() != _resolveBuffer->getGUID())) {
        input->resolve();
    }

    GLuint previousFB = GL_API::getActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE);
    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_ONLY, input->_framebufferHandle);
    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY, this->_framebufferHandle);

    if (blitColour && hasColour()) {
        glRTAttachment* thisAtt = static_cast<glRTAttachment*>(_attachments.get(RTAttachment::Type::Colour, index).get());
        glRTAttachment* inputAtt = static_cast<glRTAttachment*>(input->_attachments.get(RTAttachment::Type::Colour, index).get());
        if (thisAtt->enabled()) {
            glDrawBuffer(thisAtt->getInfo().first);
            glReadBuffer(inputAtt->getInfo().first);
            glBlitFramebuffer(0,
                                0,
                                input->_width,
                                input->_height,
                                0,
                                0,
                                this->_width,
                                this->_height,
                                GL_COLOR_BUFFER_BIT,
                                GL_NEAREST);
            _context.registerDrawCall();
        }
    }

    if (blitDepth && hasDepth()) {
        glBlitFramebuffer(0,
                          0,
                          input->_width,
                          input->_height,
                          0,
                          0,
                          this->_width,
                          this->_height,
                          GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
        _context.registerDrawCall();
    }

    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, previousFB);
}

const RTAttachment& glFramebuffer::getAttachment(RTAttachment::Type type, U8 index, bool flushStateOnRequest) {
    if (_resolveBuffer) {
        resolve();
        return _resolveBuffer->getAttachment(type, flushStateOnRequest);
    }

    const RTAttachment_ptr& att = _attachments.get(type, index);
    if (flushStateOnRequest) {
        att->flush();
    }

    return *att;
}

void glFramebuffer::bind(U8 unit, RTAttachment::Type type, U8 index, bool flushStateOnRequest) {
    const Texture_ptr& attachment = getAttachment(type, index, flushStateOnRequest).asTexture();
    if (attachment != nullptr) {
        attachment->bind(unit, false);
    }
}

void glFramebuffer::resetMipMaps(const RTDrawDescriptor& drawPolicy) {
    for (U8 i = 0; i < to_const_uint(RTAttachment::Type::COUNT); ++i) {
        RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
        for (U8 j = 0; j < _attachments.attachmentCount(type); ++j)  {
            if (drawPolicy._drawMask.isEnabled(type, j) &&
                _attachments.get(type, j)->asTexture())
            {
                _attachments.get(type, j)->asTexture()->refreshMipMaps();
            }
        }
    }
}

void glFramebuffer::begin(const RTDrawDescriptor& drawPolicy) {
    static vectorImpl<GLenum> colourBuffers;

    assert(_framebufferHandle != 0 && "glFramebuffer error: Tried to bind an invalid framebuffer!");

    if (Config::ENABLE_GPU_VALIDATION) {
        assert(!glFramebuffer::_bufferBound && "glFramebuffer error: Begin() called without a call to the previous bound buffer's End()");
    }

    if (drawPolicy._changeViewport) {
        _viewportChanged = true;
        _context.setViewport(vec4<GLint>(0, 0, _width, _height));
    }

    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, _framebufferHandle);

    if (_resolveBuffer) {
        _resolved = false;
    }

    if (_previousMask != drawPolicy._drawMask) {
        // handle colour buffers first
        colourBuffers.resize(0);
        RTAttachment::Type type = RTAttachment::Type::Colour;
        U8 bufferCount = _attachments.attachmentCount(type);
        for (U8 j = 0; j < bufferCount; ++j) {
            glRTAttachment* thisAtt = static_cast<glRTAttachment*>(_attachments.get(type, j).get());
            thisAtt->enabled(drawPolicy._drawMask.isEnabled(type, j) && thisAtt->used());
            colourBuffers.push_back(thisAtt->enabled() ? thisAtt->getInfo().first : GL_NONE);
        }
        glDrawBuffers(to_uint(bufferCount), colourBuffers.data());
        
        bool drawToDepth = drawPolicy._drawMask.isEnabled(RTAttachment::Type::Depth, 0);
        if (drawToDepth != _zWriteEnabled) {
            _zWriteEnabled = drawToDepth;
            glDepthMask(_zWriteEnabled ? GL_TRUE : GL_FALSE);
        }

        checkStatus();
        _previousMask = drawPolicy._drawMask;
    }

    clear(drawPolicy);

    resetMipMaps(drawPolicy);

    if (Config::ENABLE_GPU_VALIDATION) {
        glFramebuffer::_bufferBound = true;
    }
}

void glFramebuffer::end() {
    if (Config::ENABLE_GPU_VALIDATION) {
        assert(glFramebuffer::_bufferBound && "glFramebuffer error: End() called without a previous call to Begin()");
    }

    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, 0);
    if (_viewportChanged) {
        _context.restoreViewport();
        _viewportChanged = false;
    }

    setInitialAttachments();
    resolve();

    if (Config::ENABLE_GPU_VALIDATION) {
        glFramebuffer::_bufferBound = false;
    }
}

void glFramebuffer::setInitialAttachments() {
    // Reset attachments if they changed (e.g. after layered rendering);
    for (U8 i = 0; i < to_const_ubyte(RTAttachment::Type::COUNT); ++i) {
        RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
        for (U8 j = 0; j < _attachments.attachmentCount(type); ++j) {
            if (_attachments.get(type, j)->dirty()) {
                toggleAttachment(type, j, true);
                _attachments.get(type, j)->clean();
            }
        }
    }
}

void glFramebuffer::clear(const RTDrawDescriptor& drawPolicy) const {
    if (hasColour() && drawPolicy._clearColourBuffersOnBind) {
        for (U8 index = 0; index < _attachments.attachmentCount(RTAttachment::Type::Colour); ++index) {
            const RTAttachment_ptr& att = _attachments.get(RTAttachment::Type::Colour, index);
            if (att->enabled()) {
                GFXDataFormat dataType = att->descriptor().dataType();
                if(dataType == GFXDataFormat::FLOAT_16 ||dataType == GFXDataFormat::FLOAT_32) {
                    glClearNamedFramebufferfv(_framebufferHandle, GL_COLOR, index, att->clearColour()._v);
                } else {
                    glClearNamedFramebufferuiv(_framebufferHandle, GL_COLOR, index, Util::ToUIntColour(att->clearColour())._v);
                }
                _context.registerDrawCall();
            }
        }
    }

    if (hasDepth() && drawPolicy._clearDepthBufferOnBind) {
        glClearNamedFramebufferfv(_framebufferHandle, GL_DEPTH, 0, &_depthValue);
        _context.registerDrawCall();
    }
}

void glFramebuffer::drawToLayer(RTAttachment::Type type,
                                U8 index,
                                U32 layer,
                                bool includeDepth) {

    const RTAttachment_ptr& att = _attachments.get(type, index);

    GLenum textureType = GLUtil::glTextureTypeTable[to_uint(att->descriptor().type())];
    // only for array textures (it's better to simply ignore the command if the
    // format isn't supported (debugging reasons)
    if (textureType != GL_TEXTURE_2D_ARRAY &&
        textureType != GL_TEXTURE_CUBE_MAP_ARRAY &&
        textureType != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        return;
    }

    bool useDepthLayer =
        (hasDepth() && includeDepth) ||
        (hasDepth() && type == RTAttachment::Type::Depth);
    bool useColourLayer =
        (hasColour() && type < RTAttachment::Type::Depth);

    if (useDepthLayer && _isLayeredDepth) {
        const RTAttachment_ptr& attDepth = _attachments.get(RTAttachment::Type::Depth, 0);
        glFramebufferTextureLayer(GL_FRAMEBUFFER,
                                  GL_DEPTH_ATTACHMENT,
                                  attDepth->asTexture()->getHandle(),
                                  0,
                                  layer);
        attDepth->flagDirty();
    }

    if (useColourLayer) {
        glRTAttachment* glAtt = static_cast<glRTAttachment*>(att.get());

        glFramebufferTextureLayer(GL_FRAMEBUFFER,
                                  glAtt->getInfo().first,
                                  att->asTexture()->getHandle(),
                                  0,
                                  layer);
        att->flagDirty();
        att->asTexture()->refreshMipMaps();
    }

    checkStatus();
}

void glFramebuffer::setMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel, RTAttachment::Type type, U8 index) {
    glRTAttachment* glAtt = static_cast<glRTAttachment*>(_attachments.get(type, index).get());

    if (glAtt->used()) {
        glAtt->asTexture()->setMipMapRange(mipMinLevel, mipMaxLevel);
        setMipLevel(writeLevel, type, index);
    }

}

void glFramebuffer::setMipLevel(U16 writeLevel, RTAttachment::Type type, U8 index) {
    glRTAttachment* glAtt = static_cast<glRTAttachment*>(_attachments.get(type, index).get());

    if (glAtt->used()) {
        // This is needed because certain drivers need all attachments to use the same mip level
        // This is also VERY SLOW so it might be worth optimising it per-driver version / IHV
        for (U8 i = 0; i < to_const_ubyte(RTAttachment::Type::COUNT); ++i) {
            RTAttachment::Type tempType = static_cast<RTAttachment::Type>(i);
            for (U8 j = 0; j < _attachments.attachmentCount(tempType); ++j) {
                if (type != tempType || j != index) {
                    toggleAttachment(tempType, j, false);
                    _attachments.get(tempType, j)->flagDirty();
                }
            }
        }

        glFramebufferTexture(GL_FRAMEBUFFER,
                             glAtt->getInfo().first,
                             glAtt->asTexture()->getHandle(),
                             writeLevel);
        checkStatus();
    }
}

void glFramebuffer::resetMipLevel(RTAttachment::Type type, U8 index) {
    glRTAttachment* glAtt = static_cast<glRTAttachment*>(_attachments.get(type, index).get());
    const vec2<U16>& mips = glAtt->mipMapLevel();

    setMipLevel(mips.x, mips.y, 0, type, index);
}

void glFramebuffer::readData(const vec4<U16>& rect,
                             GFXImageFormat imageFormat,
                             GFXDataFormat dataType,
                             bufferPtr outData) {
    if (_resolveBuffer) {
        resolve();
        _resolveBuffer->readData(rect, imageFormat, dataType, outData);
    } else {
        GL_API::setPixelPackUnpackAlignment();
        GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_ONLY, _framebufferHandle);
        glReadPixels(
            rect.x, rect.y, rect.z, rect.w,
            GLUtil::glImageFormatTable[to_uint(imageFormat)],
            GLUtil::glDataFormat[to_uint(dataType)], outData);
    }
}

bool glFramebuffer::checkStatus() const {
    if (Config::ENABLE_GPU_VALIDATION) {
        // check FB status
        switch (glCheckNamedFramebufferStatus(_framebufferHandle, GL_FRAMEBUFFER))
        {
            case GL_FRAMEBUFFER_COMPLETE: {
                return true;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: {
                Console::errorfn(Locale::get(_ID("ERROR_RT_ATTACHMENT_INCOMPLETE")));
                return false;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: {
                Console::errorfn(Locale::get(_ID("ERROR_RT_NO_IMAGE")));
                return false;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: {
                Console::errorfn(Locale::get(_ID("ERROR_RT_INCOMPLETE_DRAW_BUFFER")));
                return false;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: {
                Console::errorfn(Locale::get(_ID("ERROR_RT_INCOMPLETE_READ_BUFFER")));
                return false;
            }
            case GL_FRAMEBUFFER_UNSUPPORTED: {
                Console::errorfn(Locale::get(_ID("ERROR_RT_UNSUPPORTED")));
                return false;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: {
                Console::errorfn(Locale::get(_ID("ERROR_RT_INCOMPLETE_MULTISAMPLE")));
                return false;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: {
                Console::errorfn(Locale::get(_ID("ERROR_RT_INCOMPLETE_LAYER_TARGETS")));
                return false;
            }
            case gl::GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT: {
                Console::errorfn(Locale::get(_ID("ERROR_RT_DIMENSIONS")));
                return false;
            }
            case gl::GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT: {
                 Console::errorfn(Locale::get(_ID("ERROR_RT_FORMAT")));
                 return false;
            }
            default: {
                Console::errorfn(Locale::get(_ID("ERROR_UNKNOWN")));
                return false;
            }
        };
    }

    return true;
}

};  // namespace Divide