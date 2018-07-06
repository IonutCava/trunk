#include "stdafx.h"

#include "config.h"

#include "Headers/glFramebuffer.h"

#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Textures/Headers/glSamplerObject.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

bool glFramebuffer::_bufferBound = false;
bool glFramebuffer::_zWriteEnabled = true;

IMPLEMENT_CUSTOM_ALLOCATOR(glFramebuffer, 0, 0)

glFramebuffer::glFramebuffer(GFXDevice& context, const RenderTargetDescriptor& descriptor)
    : RenderTarget(context, descriptor),
      glObject(glObjectType::TYPE_FRAMEBUFFER, context),
      _resolveBuffer(nullptr),
      _resolved(false),
      _isLayeredDepth(false),
      _activeDepthBuffer(false),
      _hasMultisampledColourAttachments(false),
      _framebufferHandle(0)
{
    glCreateFramebuffers(1, &_framebufferHandle);
    assert(_framebufferHandle != 0 && "glFramebuffer error: Tried to bind an invalid framebuffer!");

    _resolved = false;
    _isLayeredDepth = false;

    if (Config::ENABLE_GPU_VALIDATION) {
        // label this FB to be able to tell that it's internally created and nor from a 3rd party lib
        glObjectLabel(GL_FRAMEBUFFER,
                      _framebufferHandle,
                      -1,
                      Util::StringFormat("DVD_FB_%d", _framebufferHandle).c_str());
    }

    // Everything disabled so that the initial "begin" will override this
    _previousPolicy.drawMask().disableAll();
    _previousPolicy.stateMask(0);

    create();
}

glFramebuffer::~glFramebuffer()
{
    if (_framebufferHandle > 0) {
        glDeleteFramebuffers(1, &_framebufferHandle);
    }

    MemoryManager::DELETE(_resolveBuffer);
}

void glFramebuffer::initAttachment(RTAttachmentType type, U8 index) {
    // Avoid invalid dimensions
    assert(getWidth() != 0 && getHeight() != 0 && "glFramebuffer error: Invalid frame buffer dimensions!");

    // Process only valid attachments
    const RTAttachment_ptr& attachment = _attachmentPool->get(type, index);
    if (!attachment->used()) {
        return;
    }

    const Texture_ptr& tex = attachment->texture();

    // Do we need to resize the attachment?
    bool shouldResize = tex->getWidth() != getWidth() || tex->getHeight() != getHeight();
    if (shouldResize) {
        tex->resize(NULL, vec2<U16>(getWidth(), getHeight()));
    }

    // Find the appropriate binding point
    GLenum attachmentEnum;
    if (type == RTAttachmentType::Depth) {
        attachmentEnum = GL_DEPTH_ATTACHMENT;

        TextureType texType = tex->getTextureType();
        _isLayeredDepth = (texType == TextureType::TEXTURE_2D_ARRAY ||
                           texType == TextureType::TEXTURE_2D_ARRAY_MS ||
                           texType == TextureType::TEXTURE_CUBE_MAP ||
                           texType == TextureType::TEXTURE_CUBE_ARRAY ||
                           texType == TextureType::TEXTURE_3D);
    } else {
        attachmentEnum = GLenum((U32)GL_COLOR_ATTACHMENT0 + index);
        if (tex->getDescriptor().isMultisampledTexture()) {
            _hasMultisampledColourAttachments = true;
        }
    }

    attachment->binding(to_U32(attachmentEnum));
    attachment->clearChanged();
}

void glFramebuffer::toggleAttachment(const RTAttachment_ptr& attachment, AttachmentState state) {
    GLenum binding = static_cast<GLenum>(attachment->binding());

    BindingState bState{ state,
                         static_cast<GLint>(attachment->mipWriteLevel()),
                         static_cast<GLint>(attachment->writeLayer()) };

    if (bState != getAttachmentState(binding)) {
        GLuint handle = 0;
        if (attachment->used() && bState._attState != AttachmentState::STATE_DISABLED) {
            handle = attachment->texture()->getHandle();
        } 

        if (bState._writeLayer > 0) {
            glNamedFramebufferTextureLayer(_framebufferHandle, binding, handle, bState._writeLevel, bState._writeLayer);
        } else {
            glNamedFramebufferTexture(_framebufferHandle, binding, handle, bState._writeLevel);
        }

        setAttachmentState(binding, bState);
    }
}

bool glFramebuffer::create() {
    // For every attachment, be it a colour or depth attachment ...
    I32 attachmentCountTotal = 0;
    for (U8 i = 0; i < to_base(RTAttachmentType::COUNT); ++i) {
        for (U8 j = 0; j < _attachmentPool->attachmentCount(static_cast<RTAttachmentType>(i)); ++j) {
            initAttachment(static_cast<RTAttachmentType>(i), j);
            assert(GL_API::s_maxFBOAttachments > ++attachmentCountTotal);
        }
    }
    ACKNOWLEDGE_UNUSED(attachmentCountTotal);

    // If this is a multisampled FBO, make sure we have a resolve buffer
    if (_hasMultisampledColourAttachments && !_resolveBuffer) {
        vectorImpl<RTAttachmentDescriptor> attachments;
        for (U8 i = 0; i < to_base(RTAttachmentType::COUNT); ++i) {
            for (U8 j = 0; j < _attachmentPool->attachmentCount(static_cast<RTAttachmentType>(i)); ++j) {
                const RTAttachment_ptr& att = _attachmentPool->get(static_cast<RTAttachmentType>(i), j);
                if (att->used()) {
                    RTAttachmentDescriptor descriptor = {};
                    descriptor._texDescriptor = att->texture()->getDescriptor();
                    descriptor._clearColour = att->clearColour();
                    descriptor._index = j;
                    descriptor._type = static_cast<RTAttachmentType>(i);
                    descriptor._texDescriptor = att->texture()->getDescriptor();

                    attachments.emplace_back(descriptor);
                }
            }
        }

        RenderTargetDescriptor desc = {};
        desc._name = getName() + "_resolve";
        desc._resolution = vec2<U16>(getWidth(), getHeight());
        desc._attachmentCount = to_U8(attachments.size());
        desc._attachments = attachments.data();

        _resolveBuffer = MemoryManager_NEW glFramebuffer(context(), desc);
    }

    setDefaultState(RenderTarget::defaultPolicy());

    if (_resolveBuffer) {
        _resolveBuffer->create();
    }

    return true;
}

bool glFramebuffer::resize(U16 width, U16 height) {
    if (Config::Profile::USE_2x2_TEXTURES) {
        width = height = 2u;
    }

    if (getWidth() == width && getHeight() == height) {
        return false;
    }

    _descriptor._resolution.set(width, height);

    if (_resolveBuffer) {
        _resolveBuffer->resize(width, height);
    }

    return create();
}

void glFramebuffer::resolve() {
    if (_resolveBuffer && !_resolved) {
        toggleAttachments(RenderTarget::defaultPolicy());
        _resolveBuffer->blitFrom(this, hasColour(), hasDepth());
    }

    _resolved = true;
}

void glFramebuffer::blitFrom(RenderTarget* inputFB,
                             bool blitColour,
                             bool blitDepth )
{
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
        vectorImpl<RTAttachment_ptr> outputAttachments;
        vectorImpl<RTAttachment_ptr> inputAttachments;
        _attachmentPool->get(RTAttachmentType::Colour, outputAttachments);
        input->_attachmentPool->get(RTAttachmentType::Colour, inputAttachments);
        
        U8 colourCount = to_U8(std::min(outputAttachments.size(),inputAttachments.size()));

        for (U8 i = 0; i < colourCount; ++i) {
            glDrawBuffer(static_cast<GLenum>(outputAttachments[i]->binding()));
            glReadBuffer(static_cast<GLenum>(inputAttachments[i]->binding()));
            glBlitFramebuffer(0,
                              0,
                              input->getWidth(),
                              input->getHeight(),
                              0,
                              0,
                              this->getWidth(),
                              this->getHeight(),
                              GL_COLOR_BUFFER_BIT,
                              GL_NEAREST);
            
            _context.registerDrawCall();
        }
    }

    if (blitDepth && hasDepth()) {
        glBlitFramebuffer(0,
                          0,
                          input->getWidth(),
                          input->getHeight(),
                          0,
                          0,
                          this->getWidth(),
                          this->getHeight(),
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
        const RTAttachment_ptr& thisAtt = _attachmentPool->get(RTAttachmentType::Colour, index);
        if (thisAtt->used()) {
            const RTAttachment_ptr& inputAtt = input->_attachmentPool->get(RTAttachmentType::Colour, index);
            glDrawBuffer(static_cast<GLenum>(thisAtt->binding()));
            glReadBuffer(static_cast<GLenum>(inputAtt->binding()));
            glBlitFramebuffer(0,
                                0,
                                input->getWidth(),
                                input->getHeight(),
                                0,
                                0,
                                this->getWidth(),
                                this->getHeight(),
                                GL_COLOR_BUFFER_BIT,
                                GL_NEAREST);
            _context.registerDrawCall();
        }
    }

    if (blitDepth && hasDepth()) {
        glBlitFramebuffer(0,
                          0,
                          input->getWidth(),
                          input->getHeight(),
                          0,
                          0,
                          this->getWidth(),
                          this->getHeight(),
                          GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
        _context.registerDrawCall();
    }

    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, previousFB);
}

const RTAttachment& glFramebuffer::getAttachment(RTAttachmentType type, U8 index) const {
    if (_resolveBuffer) {
        return _resolveBuffer->getAttachment(type, index);
    }

    return RenderTarget::getAttachment(type, index);
}

void glFramebuffer::bind(U8 unit, RTAttachmentType type, U8 index) {
    const RTAttachment& attachment = getAttachment(type, index);
    if (attachment.used()) {
        attachment.texture()->bind(unit);
    }
}

void glFramebuffer::setBlendState(const RTDrawDescriptor& drawPolicy, const vectorImpl<RTAttachment_ptr>& activeAttachments) {
    const RTDrawMask& mask = drawPolicy.drawMask();

    for (U8 i = 0; i < activeAttachments.size(); ++i) {
        if (mask.isEnabled(RTAttachmentType::Colour, i)) {
            const RTAttachment_ptr& colourAtt = activeAttachments[i];

            const RTBlendState& blend = drawPolicy.blendState(i);

            // Set blending per attachment if specified. Overrides general blend state
            GL_API::setBlending(static_cast<GLuint>(colourAtt->binding() - to_U32(GL_COLOR_ATTACHMENT0)),
                                blend._blendEnable,
                                blend._blendProperties,
                                blend._blendColour);
        }
    }
}

void glFramebuffer::prepareBuffers(const RTDrawDescriptor& drawPolicy, const vectorImpl<RTAttachment_ptr>& activeAttachments) {
    const RTDrawMask& mask = drawPolicy.drawMask();

    if (_previousPolicy.drawMask() != mask) {
        // handle colour buffers first
        vectorImpl<GLenum> activeColourBuffers(activeAttachments.size());

        if (!activeColourBuffers.empty()) {
            for (U8 j = 0; j < activeColourBuffers.size(); ++j) {
                const RTAttachment_ptr& colourAtt = activeAttachments[j];
                activeColourBuffers[j] =  mask.isEnabled(RTAttachmentType::Colour, j) ? static_cast<GLenum>(colourAtt->binding()) : GL_NONE;
            }

            glNamedFramebufferDrawBuffers(_framebufferHandle,
                                          static_cast<GLsizei>(activeColourBuffers.size()),
                                          activeColourBuffers.data());
        }

        const RTAttachment_ptr& depthAtt = _attachmentPool->get(RTAttachmentType::Depth, 0);
        _activeDepthBuffer = depthAtt && depthAtt->used();
     }
    
    if (mask.isEnabled(RTAttachmentType::Depth, 0) != _zWriteEnabled) {
        _zWriteEnabled = !_zWriteEnabled;
        glDepthMask(_zWriteEnabled ? GL_TRUE : GL_FALSE);
    }
}

void glFramebuffer::toggleAttachments(const RTDrawDescriptor& drawPolicy) {
    for (U8 i = 0; i < to_base(RTAttachmentType::COUNT); ++i) {
        vectorImpl<RTAttachment_ptr> attachments;

        /// Get the attachments in use for each type
        _attachmentPool->get(static_cast<RTAttachmentType>(i), attachments);

        /// Reset attachments if they changed (e.g. after layered rendering);
        for (const RTAttachment_ptr& attachment : attachments) {
            /// We also draw to mip and layer 0 unless specified otherwise in the drawPolicy
            attachment->writeLayer(0);
            attachment->mipWriteLevel(0);

            /// All active attachments are enabled by default
            toggleAttachment(attachment, AttachmentState::STATE_ENABLED);
        }
    }
}

void glFramebuffer::setDefaultState(const RTDrawDescriptor& drawPolicy) {
    toggleAttachments(drawPolicy);

    vectorImpl<RTAttachment_ptr> colourAttachments;
    _attachmentPool->get(RTAttachmentType::Colour, colourAttachments);

    /// Setup draw buffers
    prepareBuffers(drawPolicy, colourAttachments);

    /// Set the depth range
    GL_API::setDepthRange(_descriptor._depthRange.min, _descriptor._depthRange.max);

    /// Set the blend states
    setBlendState(drawPolicy, colourAttachments);

    /// Clear the draw buffers
    clear(drawPolicy, colourAttachments);

    /// Check that everything is valid
    checkStatus();
}

void glFramebuffer::begin(const RTDrawDescriptor& drawPolicy) {
    /// Push debug state
    if (Config::ENABLE_GPU_VALIDATION) {
        assert(!glFramebuffer::_bufferBound && "glFramebuffer error: Begin() called without a call to the previous bound buffer's End()");
        GL_API::pushDebugMessage(_context, ("FBO Begin: " + getName()).c_str(), _framebufferHandle);
        glFramebuffer::_bufferBound = true;
    }

    /// Activate FBO
    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, _framebufferHandle);

    /// Set the viewport
    if (drawPolicy.isEnabledState(RTDrawDescriptor::State::CHANGE_VIEWPORT)) {
         _context.setViewport(0, 0, to_I32(getWidth()), to_I32(getHeight()));
    }

    setDefaultState(drawPolicy);

    /// Mark the resolve buffer as dirty
    _resolved = false;

    /// Memorize the current draw policy to speed up later calls
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

    resolve();

    if (Config::ENABLE_GPU_VALIDATION) {
        glFramebuffer::_bufferBound = false;
        GL_API::popDebugMessage(_context);
    }
}

void glFramebuffer::clear(const RTDrawDescriptor& drawPolicy, const vectorImpl<RTAttachment_ptr>& activeAttachments) const {
    if (drawPolicy.isEnabledState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS) && hasColour()) {
        for (const RTAttachment_ptr& att : activeAttachments) {
            U32 binding = att->binding();
            if (static_cast<GLenum>(binding) != GL_NONE) {
                GLint buffer = static_cast<GLint>(binding - static_cast<GLint>(GL_COLOR_ATTACHMENT0));

                GFXDataFormat dataType = att->texture()->getDescriptor().dataType();
                if (dataType == GFXDataFormat::FLOAT_16 || dataType == GFXDataFormat::FLOAT_32) {
                    glClearNamedFramebufferfv(_framebufferHandle, GL_COLOR, buffer, att->clearColour()._v);
                } else if (dataType == GFXDataFormat::SIGNED_BYTE || dataType == GFXDataFormat::SIGNED_SHORT || dataType == GFXDataFormat::SIGNED_INT) {
                    glClearNamedFramebufferiv(_framebufferHandle, GL_COLOR, buffer, Util::ToIntColour(att->clearColour())._v);
                } else {
                    glClearNamedFramebufferuiv(_framebufferHandle, GL_COLOR, buffer, Util::ToUIntColour(att->clearColour())._v);
                }
                att->texture()->refreshMipMaps();
                _context.registerDrawCall();
            }
        }
    }

    if (drawPolicy.isEnabledState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER) && hasDepth()) {
        glClearNamedFramebufferfv(_framebufferHandle, GL_DEPTH, 0, &_descriptor._depthValue);
        _attachmentPool->get(RTAttachmentType::Depth, 0)->texture()->refreshMipMaps();
        _context.registerDrawCall();
    }
}

void glFramebuffer::drawToLayer(RTAttachmentType type,
                                U8 index,
                                U16 layer,
                                bool includeDepth) {

    const RTAttachment_ptr& att = _attachmentPool->get(type, index);

    GLenum textureType = GLUtil::glTextureTypeTable[to_U32(att->texture()->getTextureType())];
    // only for array textures (it's better to simply ignore the command if the format isn't supported (debugging reasons)
    if (textureType != GL_TEXTURE_2D_ARRAY &&
        textureType != GL_TEXTURE_CUBE_MAP_ARRAY &&
        textureType != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        return;
    }

    bool useDepthLayer =  (hasDepth()  && includeDepth) ||
                          (hasDepth()  && type == RTAttachmentType::Depth);
    bool useColourLayer = (hasColour() && type == RTAttachmentType::Colour);

    if (useDepthLayer && _isLayeredDepth) {
        const RTAttachment_ptr& attDepth = _attachmentPool->get(RTAttachmentType::Depth, 0);
        const BindingState& state = getAttachmentState(static_cast<GLenum>(attDepth->binding()));
        attDepth->writeLayer(layer);
        toggleAttachment(attDepth, state._attState);
    }

    if (useColourLayer) {
        const BindingState& state = getAttachmentState(static_cast<GLenum>(att->binding()));
        att->writeLayer(layer);
        toggleAttachment(att, state._attState);
    }

    checkStatus();
}

void glFramebuffer::setMipLevel(U16 writeLevel) {
    vectorImpl<RTAttachment_ptr> attachments;

    // This is needed because certain drivers need all attachments to use the same mip level
    // This is also VERY SLOW so it might be worth optimising it per-driver version / IHV
    for (U8 i = 0; i < to_base(RTAttachmentType::COUNT); ++i) {
        _attachmentPool->get(static_cast<RTAttachmentType>(i), attachments);

        for (const RTAttachment_ptr& attachment : attachments) {
            const Texture_ptr& texture = attachment->texture();
            if (texture->getMaxMipLevel() > writeLevel && !texture->getDescriptor().isMultisampledTexture())
            {
                const BindingState& state = getAttachmentState(static_cast<GLenum>(attachment->binding()));
                attachment->mipWriteLevel(writeLevel);
                toggleAttachment(attachment, state._attState);
            } else {
                //nVidia still has issues if attachments are not all at the same mip level -Ionut
                if (GFXDevice::getGPUVendor() == GPUVendor::NVIDIA) {
                    toggleAttachment(attachment, AttachmentState::STATE_DISABLED);
                }
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
    return _attachmentPool->attachmentCount(RTAttachmentType::Colour) > 0;
}

void glFramebuffer::setAttachmentState(GLenum binding, BindingState state) {
    _attachmentState[binding] = state;
}

glFramebuffer::BindingState glFramebuffer::getAttachmentState(GLenum binding) const {
    hashMapImpl<GLenum, BindingState>::const_iterator it = _attachmentState.find(binding);
    if (it != std::cend(_attachmentState)) {
        return it->second;
    }

    return { AttachmentState::COUNT, -1, -1 };
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