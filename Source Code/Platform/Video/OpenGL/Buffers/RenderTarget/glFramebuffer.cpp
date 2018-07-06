#include "stdafx.h"

#include "config.h"

#include "Headers/glFramebuffer.h"

#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Textures/Headers/glSamplerObject.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
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
bool glFramebuffer::_zWriteEnabled = true;

IMPLEMENT_CUSTOM_ALLOCATOR(glFramebuffer, 0, 0)
glFramebuffer::glFramebuffer(GFXDevice& context, const stringImpl& name)
    : RenderTarget(context, name),
      _resolveBuffer(nullptr),
      _resolved(false),
      _isLayeredDepth(false),
      _activeDepthBuffer(false),
      _framebufferHandle(0)
{
    // Everything disabled so that the initial "begin" will override this
    _previousPolicy.drawMask().disableAll();
    _previousPolicy.stateMask(0);
}

glFramebuffer::~glFramebuffer()
{
    if (_framebufferHandle > 0) {
        glDeleteFramebuffers(1, &_framebufferHandle);
    }

    MemoryManager::DELETE(_resolveBuffer);
}

void glFramebuffer::copy(const RenderTarget& other) {
    RenderTarget::copy(other);
    create(other.getWidth(), other.getHeight());
}

void glFramebuffer::updateDescriptor(RTAttachment::Type type, U8 index) {
    const RTAttachment_ptr& attachment = _attachmentPool->get(type, index);

    if (!attachment->used() && !attachment->changed()) {
        return;
    }

    TextureDescriptor& texDescriptor = attachment->descriptor();

    bool multisampled = _resolveBuffer != nullptr;
    if ((type == RTAttachment::Type::Colour && index == 0) ||
         type == RTAttachment::Type::Depth) {
        multisampled = texDescriptor._type == TextureType::TEXTURE_2D_MS ||
                       texDescriptor._type == TextureType::TEXTURE_2D_ARRAY_MS;
        multisampled = multisampled && GL_API::s_msaaSamples > 0;
    }

    if (multisampled) {
        texDescriptor.getSampler().toggleMipMaps(false);
        if (!_resolveBuffer) {
             _resolveBuffer = MemoryManager_NEW glFramebuffer(_context, _name + "_resolve");
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

void glFramebuffer::initAttachment(const RTAttachment_ptr& attachment, RTAttachment::Type type, U8 index, U8 copyCount) {
    TextureDescriptor& texDescriptor = attachment->descriptor();

    assert(_width != 0 && _height != 0 && "glFramebuffer error: Invalid frame buffer dimensions!");

    vec2<U16> mipLevel(0,
                       texDescriptor.getSampler().generateMipMaps()
                            ? 1 + Texture::computeMipCount(_width, _height)
                            : 1);

    // check if we have a valid attachment
    if (attachment->used()) {
        const Texture_ptr& tex = attachment->asTexture();
        // Do we need to resize the attachment?
        if (tex->getWidth() != _width || tex->getHeight() != _height) {
            tex->resize(NULL, vec2<U16>(_width, _height), mipLevel);
        }
    } else {
        ResourceDescriptor textureAttachment(Util::StringFormat("FBO_%s_Att_%s_%d_%d_%d",
                                             _name.c_str(),
                                             getAttachmentName(type),
                                             index,
                                             copyCount,
                                             getGUID()));
        textureAttachment.setThreadedLoading(false);
        textureAttachment.setPropertyDescriptor(texDescriptor.getSampler());
        textureAttachment.setEnumValue(to_U32(texDescriptor._type));
        textureAttachment.setID(texDescriptor._layerCount);
        Texture_ptr tex = CreateResource<Texture>(_context.parent().resourceCache(), textureAttachment);
        assert(tex);
        Texture::TextureLoadInfo info;
        info._type = texDescriptor._type;
        tex->lockAutomaticMipMapGeneration(!texDescriptor.automaticMipMapGeneration());
        tex->loadData(info, texDescriptor, NULL, vec2<U16>(_width, _height), mipLevel);
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

    attachment->binding(to_U32(attachmentEnum));
    attachment->clearChanged();
}

void glFramebuffer::initAttachment(RTAttachment::Type type, U8 index ) {
    const RTAttachment_ptr& attachment = _attachmentPool->get(type, index);

    if (!attachment->used() && !attachment->changed()) {
        return;
    }

    initAttachment(attachment, type, index, 0);
}

void glFramebuffer::toggleAttachment(const RTAttachment_ptr& attachment, AttachmentState state) {
    GLenum binding = static_cast<GLenum>(attachment->binding());

    if (state != getAttachmentState(binding)) {
        GLuint handle = 0;
        GLint level = 0;
        GLint layer = 0;
        if (attachment->used() && state != AttachmentState::STATE_DISABLED) {
            handle = attachment->asTexture()->getHandle();
            level = attachment->mipWriteLevel();
            layer = attachment->writeLayer();
        } 

        if (state == AttachmentState::STATE_LAYERED) {
            glNamedFramebufferTextureLayer(_framebufferHandle, binding, handle, level, layer);
        } else {
            glNamedFramebufferTexture(_framebufferHandle, binding, handle, level);
        }

        setAttachmentState(binding, state);
    }
}

bool glFramebuffer::create(U16 width, U16 height) {
    for (U8 i = 0; i < to_base(RTAttachment::Type::COUNT); ++i) {
        RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
        for (U8 j = 0; j < _attachmentPool->attachmentCount(type); ++j) {
            updateDescriptor(type, j);
        }
    }
    
    if (_resolveBuffer) {
        for (U8 i = 0; i < to_base(RTAttachment::Type::COUNT); ++i) {
            RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
            for (U8 j = 0; j < _attachmentPool->attachmentCount(type); ++j) {
                const RTAttachment_ptr& att = _attachmentPool->get(type, j);
                const RTAttachment_ptr& resAtt = _resolveBuffer->_attachmentPool->get(type, j);
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

    // For every attachment, be it a colour or depth attachment ...
    I32 attachmentCountTotal = 0;
    for (U8 i = 0; i < to_base(RTAttachment::Type::COUNT); ++i) {
        for (U8 j = 0; j < _attachmentPool->attachmentCount(static_cast<RTAttachment::Type>(i)); ++j) {
            initAttachment(static_cast<RTAttachment::Type>(i), j);
            assert(GL_API::s_maxFBOAttachments > ++attachmentCountTotal);
        }
    }
    ACKNOWLEDGE_UNUSED(attachmentCountTotal);

    resetAttachments();
    prepareBuffers(RenderTarget::defaultPolicy());
    clear(RenderTarget::defaultPolicy());

    return checkStatus();
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
        vectorImpl<RTAttachment_ptr> inputAttachments;
        _attachmentPool->get(RTAttachment::Type::Colour, _activeAttachmentsCache);
        input->_attachmentPool->get(RTAttachment::Type::Colour, inputAttachments);
        
        U8 colourCount = std::min(to_U8(_activeAttachmentsCache.size()), to_U8(inputAttachments.size()));
        for (U8 i = 0; i < colourCount; ++i) {
            glDrawBuffer(static_cast<GLenum>(_activeAttachmentsCache[i]->binding()));
            glReadBuffer(static_cast<GLenum>(inputAttachments[i]->binding()));
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
        const RTAttachment_ptr& thisAtt = _attachmentPool->get(RTAttachment::Type::Colour, index);
        if (thisAtt->used()) {
            const RTAttachment_ptr& inputAtt = input->_attachmentPool->get(RTAttachment::Type::Colour, index);
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
        return _resolveBuffer->getAttachment(type, index, flushStateOnRequest);
    }

    return RenderTarget::getAttachment(type, index, flushStateOnRequest);
}

void glFramebuffer::bind(U8 unit, RTAttachment::Type type, U8 index, bool flushStateOnRequest) {
    const RTAttachment& attachment = getAttachment(type, index, flushStateOnRequest);
    if (attachment.used()) {
        attachment.asTexture()->bind(unit, false);
    }
}

void glFramebuffer::prepareBuffers(const RTDrawDescriptor& drawPolicy) {
    const RTDrawMask& mask = drawPolicy.drawMask();

    if (_previousPolicy.drawMask() != mask) {
        _attachmentPool->get(RTAttachment::Type::Colour, _activeAttachmentsCache);
        // handle colour buffers first
        _activeColourBuffers.resize(_activeAttachmentsCache.size());

        if (!_activeColourBuffers.empty()) {
            for (U8 j = 0; j < _activeColourBuffers.size(); ++j) {
                _activeColourBuffers[j] =
                    mask.isEnabled(RTAttachment::Type::Colour, j) ? static_cast<GLenum>(_activeAttachmentsCache[j]->binding())
                                                                  : GL_NONE;
            }

            glNamedFramebufferDrawBuffers(_framebufferHandle,
                                          static_cast<GLsizei>(_activeColourBuffers.size()),
                                          _activeColourBuffers.data());
        }

        const RTAttachment_ptr& depthAtt = _attachmentPool->get(RTAttachment::Type::Depth, 0);
        _activeDepthBuffer = depthAtt && depthAtt->used();
     }
    
    if (mask.isEnabled(RTAttachment::Type::Depth, 0) != _zWriteEnabled) {
        _zWriteEnabled = !_zWriteEnabled;
        glDepthMask(_zWriteEnabled ? GL_TRUE : GL_FALSE);
    }

    checkStatus();
}

void glFramebuffer::resetAttachments() {
    // Reset attachments if they changed (e.g. after layered rendering);
    for (U8 i = 0; i < to_base(RTAttachment::Type::COUNT); ++i) {
        _attachmentPool->get(static_cast<RTAttachment::Type>(i), _activeAttachmentsCache);
        for (const RTAttachment_ptr& attachment : _activeAttachmentsCache) {
            attachment->writeLayer(0);
            attachment->mipWriteLevel(0);
            toggleAttachment(attachment, AttachmentState::STATE_ENABLED);
        }
    }
}

void glFramebuffer::begin(const RTDrawDescriptor& drawPolicy) {
    assert(_framebufferHandle != 0 && "glFramebuffer error: Tried to bind an invalid framebuffer!");

    if (Config::ENABLE_GPU_VALIDATION) {
        assert(!glFramebuffer::_bufferBound && "glFramebuffer error: Begin() called without a call to the previous bound buffer's End()");
        _context.pushDebugMessage(("FBO Begin: " + _name).c_str(), 4);
        glFramebuffer::_bufferBound = true;
    }

    if (drawPolicy.isEnabledState(RTDrawDescriptor::State::CHANGE_VIEWPORT)) {
         _context.setViewport(0, 0, _width, _height);
    }

    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, _framebufferHandle);

    prepareBuffers(drawPolicy);
    clear(drawPolicy);

    _resolved = _resolveBuffer != nullptr;

    _previousPolicy = drawPolicy;
}

void glFramebuffer::end() {
    if (Config::ENABLE_GPU_VALIDATION) {
        assert(glFramebuffer::_bufferBound && "glFramebuffer error: End() called without a previous call to Begin()");
    }

    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, 0);
    if (_previousPolicy.isEnabledState(RTDrawDescriptor::State::CHANGE_VIEWPORT)) {
        _context.restoreViewport();
    }

    resetAttachments();
    resolve();

    if (Config::ENABLE_GPU_VALIDATION) {
        glFramebuffer::_bufferBound = false;
        _context.popDebugMessage();
    }
}

void glFramebuffer::clear(const RTDrawDescriptor& drawPolicy) const {
    if (drawPolicy.isEnabledState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS) && hasColour()) {
        for (U8 i = 0; i < to_U8(_activeColourBuffers.size()); ++i) {
            if (_activeColourBuffers[i] != GL_NONE) {
                const RTAttachment_ptr& att = _attachmentPool->get(RTAttachment::Type::Colour, i);
                GFXDataFormat dataType = att->descriptor().dataType();
                if (dataType == GFXDataFormat::FLOAT_16 || dataType == GFXDataFormat::FLOAT_32) {
                    glClearNamedFramebufferfv(_framebufferHandle, GL_COLOR, i, att->clearColour()._v);
                } else if (dataType == GFXDataFormat::SIGNED_BYTE || dataType == GFXDataFormat::SIGNED_SHORT || dataType == GFXDataFormat::SIGNED_INT) {
                    glClearNamedFramebufferiv(_framebufferHandle, GL_COLOR, i, Util::ToIntColour(att->clearColour())._v);
                } else {
                    glClearNamedFramebufferuiv(_framebufferHandle, GL_COLOR, i, Util::ToUIntColour(att->clearColour())._v);
                }
                att->asTexture()->refreshMipMaps();
                _context.registerDrawCall();
            }
        }
    }

    if (drawPolicy.isEnabledState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER) && hasDepth()) {
        glClearNamedFramebufferfv(_framebufferHandle, GL_DEPTH, 0, &_depthValue);
        _attachmentPool->get(RTAttachment::Type::Depth, 0)->asTexture()->refreshMipMaps();
        _context.registerDrawCall();
    }
}

void glFramebuffer::drawToLayer(RTAttachment::Type type,
                                U8 index,
                                U16 layer,
                                bool includeDepth) {

    const RTAttachment_ptr& att = _attachmentPool->get(type, index);

    GLenum textureType = GLUtil::glTextureTypeTable[to_U32(att->descriptor().type())];
    // only for array textures (it's better to simply ignore the command if the format isn't supported (debugging reasons)
    if (textureType != GL_TEXTURE_2D_ARRAY &&
        textureType != GL_TEXTURE_CUBE_MAP_ARRAY &&
        textureType != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        return;
    }

    bool useDepthLayer =
        (hasDepth() && includeDepth) ||
        (hasDepth() && type == RTAttachment::Type::Depth);
    bool useColourLayer =
        (hasColour() && type == RTAttachment::Type::Colour);

    if (useDepthLayer && _isLayeredDepth) {
        const RTAttachment_ptr& attDepth = _attachmentPool->get(RTAttachment::Type::Depth, 0);
        attDepth->writeLayer(layer);
        toggleAttachment(attDepth, AttachmentState::STATE_LAYERED);
    }

    if (useColourLayer) {
        att->writeLayer(layer);
        toggleAttachment(att, AttachmentState::STATE_LAYERED);
    }
    
    checkStatus();
}

void glFramebuffer::setMipLevel(U16 writeLevel) {
    // This is needed because certain drivers need all attachments to use the same mip level
    // This is also VERY SLOW so it might be worth optimising it per-driver version / IHV
    for (U8 i = 0; i < to_base(RTAttachment::Type::COUNT); ++i) {
        _attachmentPool->get(static_cast<RTAttachment::Type>(i), _activeAttachmentsCache);
        for (const RTAttachment_ptr& attachment : _activeAttachmentsCache) {
            const Texture_ptr& texture = attachment->asTexture();
            if (texture->getMaxMipLevel() > writeLevel) {
                if (attachment->writeLayer() > 0) {
                    glNamedFramebufferTextureLayer(_framebufferHandle,
                                                   static_cast<GLenum>(attachment->binding()),
                                                   texture->getHandle(),
                                                   writeLevel,
                                                   attachment->writeLayer());
                } else {
                    glNamedFramebufferTexture(_framebufferHandle,
                                              static_cast<GLenum>(attachment->binding()),
                                              texture->getHandle(),
                                              writeLevel);
                }
                attachment->mipWriteLevel(writeLevel);
            } else {
                toggleAttachment(attachment, AttachmentState::STATE_DISABLED);
            }
        }
    }

    checkStatus();
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
            GLUtil::glImageFormatTable[to_U32(imageFormat)],
            GLUtil::glDataFormat[to_U32(dataType)], outData);
    }
}

bool glFramebuffer::hasDepth() const {
    return _activeDepthBuffer;
}

bool glFramebuffer::hasColour() const {
    return !_activeColourBuffers.empty();
}

void glFramebuffer::setAttachmentState(GLenum binding, AttachmentState state) {
    _attachmentState[binding] = state;
}

glFramebuffer::AttachmentState glFramebuffer::getAttachmentState(GLenum binding) const {
    hashMapImpl<GLenum, AttachmentState>::const_iterator it = _attachmentState.find(binding);
    if (it != std::cend(_attachmentState)) {
        return it->second;
    }

    return AttachmentState::COUNT;
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