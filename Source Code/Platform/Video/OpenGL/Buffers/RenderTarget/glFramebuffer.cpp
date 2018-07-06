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
glFramebuffer::glFramebuffer(GFXDevice& context)
    : RenderTarget(context),
      _resolveBuffer(nullptr),
      _resolved(false),
      _isLayeredDepth(false),
      _framebufferHandle(0)
{
    _previousMask.enableAll();

    I32 maxColourAttachments = ParamHandler::instance().getParam<I32>(_ID("rendering.maxRenderTargetOutputs"), 32);
    assert(maxColourAttachments > RenderTarget::g_maxColourAttachments);
    ACKNOWLEDGE_UNUSED(maxColourAttachments);
    
    _attachments.init(RenderTarget::g_maxColourAttachments);
}

glFramebuffer::~glFramebuffer()
{
    destroy();
    MemoryManager::DELETE(_resolveBuffer);
}

void glFramebuffer::updateDescriptor(RTAttachment::Type type, U8 index) {
    const RTAttachment_ptr& attachment = _attachments.get(type, index);

    if (!attachment->used() && !attachment->changed()) {
        return;
    }

    TextureDescriptor& texDescriptor = attachment->descriptor();

    bool multisampled = _resolveBuffer != nullptr;
    if ((type == RTAttachment::Type::Colour && index == 0) ||
         type == RTAttachment::Type::Depth) {
        multisampled = texDescriptor._type == TextureType::TEXTURE_2D_MS ||
                       texDescriptor._type == TextureType::TEXTURE_2D_ARRAY_MS;
        multisampled = multisampled &&
                       ParamHandler::instance().getParam<I32>(_ID("rendering.MSAAsampless"), 0) > 0;
    }

    if (multisampled) {
        texDescriptor.getSampler().toggleMipMaps(false);
        if (!_resolveBuffer) {
             _resolveBuffer = MemoryManager_NEW glFramebuffer(_context);
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

}

void glFramebuffer::initAttachment(RTAttachment::Type type, U8 index ) {
    const RTAttachment_ptr& attachment = _attachments.get(type, index);

    if (!attachment->used() && !attachment->changed()) {
        return;
    }

    TextureDescriptor& texDescriptor = attachment->descriptor();

    assert(_width != 0 && _height != 0 && "glFramebuffer error: Invalid frame buffer dimensions!");

    attachment->mipMapLevel(0,
                            texDescriptor.getSampler().generateMipMaps()
                                ? 1 + (I16)floorf(log2f(fmaxf(to_float(_width), to_float(_height))))
                                : 1);

    // check if we have a valid attachment
    if (attachment->used()) {
        const Texture_ptr& tex = attachment->asTexture();
        // Do we need to resize the attachment?
        if (tex->getWidth() != _width || tex->getHeight() != _height) {
            tex->resize(NULL, vec2<U16>(_width, _height), attachment->mipMapLevel());
        }
    } else {
        stringImpl attachmentName = Util::StringFormat(
            "Framebuffer_Att_%s_%d_%d", getAttachmentName(type), index, getGUID());

        ResourceDescriptor textureAttachment(attachmentName);
        textureAttachment.setThreadedLoading(false);
        textureAttachment.setPropertyDescriptor(texDescriptor.getSampler());
        textureAttachment.setEnumValue(to_uint(texDescriptor._type));
        Texture_ptr tex = CreateResource<Texture>(textureAttachment);
        assert(tex);
        Texture::TextureLoadInfo info;
        info._type = texDescriptor._type;
        tex->loadData(info, texDescriptor, NULL, vec2<U16>(_width, _height), attachment->mipMapLevel());
        tex->setNumLayers(texDescriptor._layerCount);
        tex->lockAutomaticMipMapGeneration(!texDescriptor.automaticMipMapGeneration());
        attachment->setTexture(tex);
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

    attachment->binding(to_uint(attachmentEnum));
       
    attachment->flagDirty();
    attachment->clearChanged();
    attachment->enabled(true);
}

void glFramebuffer::toggleAttachment(RTAttachment::Type type, U8 index, bool state) {
    RTAttachment* att = _attachments.get(type, index).get();
    GLuint handle = (att->asTexture() ? att->asTexture()->getHandle() : 0);
    if (handle != 0 && state != att->toggledState()) {
        glNamedFramebufferTexture(_framebufferHandle,
                                  static_cast<GLenum>(att->binding()),
                                  state ? handle : 0,
                                  0);
        att->toggledState(state);
    }
}

bool glFramebuffer::create(U16 width, U16 height) {
    for (U8 i = 0; i < to_const_ubyte(RTAttachment::Type::COUNT); ++i) {
        RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
        for (U8 j = 0; j < _attachments.attachmentCount(type); ++j) {
            updateDescriptor(type, j);
        }
    }
    
    if (_resolveBuffer) {
        for (U8 i = 0; i < to_const_ubyte(RTAttachment::Type::COUNT); ++i) {
            RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
            for (U8 j = 0; j < _attachments.attachmentCount(type); ++j) {
                const RTAttachment_ptr& att = _attachments.get(type, j);
                const RTAttachment_ptr& resAtt = _resolveBuffer->_attachments.get(type, j);
                resAtt->fromDescriptor(att->descriptor());
                resAtt->clearColour(att->clearColour());
            }
        }

        _resolveBuffer->create(width, height);
    }

    bool shouldResetAttachments = (_width != width || _height != height);
    _width = width;
    _height = height;

    if (Config::Profile::USE_2x2_TEXTURES) {
        _width = _height = 2;
    }

    if (_framebufferHandle == 0) {
        glCreateFramebuffers(1, &_framebufferHandle);
        shouldResetAttachments = false;
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
            initAttachment(static_cast<RTAttachment::Type>(i), j);
            if (type == RTAttachment::Type::Colour && att->enabled()) {
                colorBuffsers.push_back(static_cast<GLenum>(att.get()->binding()));
            }
        }
    }

    if (!shouldResetAttachments) {
        resetAttachments();
    }

    // If colour writes are disabled, draw only depth info
    if (!colorBuffsers.empty()) {
        glNamedFramebufferDrawBuffers(_framebufferHandle, 
                                      static_cast<GLsizei>(colorBuffsers.size()),
                                      colorBuffsers.data());
    }

    if (checkStatus()) {
        clear(RenderTarget::defaultPolicy());
        return true;
    }

    return false;
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
            RTAttachment* thisAtt = _attachments.get(type, j).get();
            RTAttachment* inputAtt = input->_attachments.get(type, j).get();
            if (thisAtt->enabled()) {
                glDrawBuffer(static_cast<GLenum>(thisAtt->binding()));
                glReadBuffer(static_cast<GLenum>(inputAtt->binding()));
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
        RTAttachment* thisAtt = _attachments.get(RTAttachment::Type::Colour, index).get();
        RTAttachment* inputAtt = input->_attachments.get(RTAttachment::Type::Colour, index).get();
        if (thisAtt->enabled()) {
            glDrawBuffer(static_cast<GLenum>(thisAtt->binding()));
            glReadBuffer(static_cast<GLenum>(inputAtt->binding()));
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
            if (drawPolicy.drawMask().isEnabled(type, j) &&
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

    if (drawPolicy.isEnabledState(RTDrawDescriptor::State::CHANGE_VIEWPORT)) {
        _viewportChanged = true;
        _context.setViewport(vec4<GLint>(0, 0, _width, _height));
    }

    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, _framebufferHandle);

    if (_resolveBuffer) {
        _resolved = false;
    }

    if (_previousMask != drawPolicy.drawMask()) {
        // handle colour buffers first
        colourBuffers.resize(0);
        RTAttachment::Type type = RTAttachment::Type::Colour;
        U8 bufferCount = _attachments.attachmentCount(type);
        for (U8 j = 0; j < bufferCount; ++j) {
            RTAttachment* thisAtt = _attachments.get(type, j).get();
            thisAtt->enabled(drawPolicy.drawMask().isEnabled(type, j) && thisAtt->used());
            colourBuffers.push_back(thisAtt->enabled() ? static_cast<GLenum>(thisAtt->binding()): GL_NONE);
        }
        glDrawBuffers(to_uint(bufferCount), colourBuffers.data());
        
        bool drawToDepth = drawPolicy.drawMask().isEnabled(RTAttachment::Type::Depth, 0);
        if (drawToDepth != _zWriteEnabled) {
            _zWriteEnabled = drawToDepth;
            glDepthMask(_zWriteEnabled ? GL_TRUE : GL_FALSE);
        }

        checkStatus();
        _previousMask = drawPolicy.drawMask();
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

    resetAttachments();
    resolve();

    if (Config::ENABLE_GPU_VALIDATION) {
        glFramebuffer::_bufferBound = false;
    }
}

void glFramebuffer::resetAttachments() {
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
    if (hasColour() && drawPolicy.isEnabledState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS)) {
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

    if (hasDepth() && drawPolicy.isEnabledState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER)) {
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
        glFramebufferTextureLayer(GL_FRAMEBUFFER,
                                  static_cast<GLenum>(att->binding()),
                                  att->asTexture()->getHandle(),
                                  0,
                                  layer);
        att->flagDirty();
        att->asTexture()->refreshMipMaps();
    }

    checkStatus();
}

void glFramebuffer::setMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel, RTAttachment::Type type, U8 index) {
    RTAttachment* glAtt = _attachments.get(type, index).get();

    if (glAtt->used()) {
        glAtt->asTexture()->setMipMapRange(mipMinLevel, mipMaxLevel);
        setMipLevel(writeLevel, type, index);
    }

}

void glFramebuffer::setMipLevel(U16 writeLevel, RTAttachment::Type type, U8 index) {
    RTAttachment* att = _attachments.get(type, index).get();

    if (att->used()) {
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
                             static_cast<GLenum>(att->binding()),
                             att->asTexture()->getHandle(),
                             writeLevel);
        checkStatus();
    }
}

void glFramebuffer::resetMipLevel(RTAttachment::Type type, U8 index) {
    const vec2<U16>& mips = _attachments.get(type, index)->mipMapLevel();
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