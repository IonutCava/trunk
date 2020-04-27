#include "stdafx.h"

#include "config.h"

#include "Headers/glFramebuffer.h"

#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"
#include "Platform/Video/RenderBackend/OpenGL/Textures/Headers/glSamplerObject.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
    FORCE_INLINE bool IsValid(const DepthBlitEntry& entry) noexcept {
        return entry._inputLayer != INVALID_DEPTH_LAYER &&
               entry._outputLayer != INVALID_DEPTH_LAYER;
    }
};


bool glFramebuffer::_zWriteEnabled = true;

glFramebuffer::glFramebuffer(GFXDevice& context, const RenderTargetDescriptor& descriptor)
    : RenderTarget(context, descriptor),
      glObject(glObjectType::TYPE_FRAMEBUFFER, context),
      _isLayeredDepth(false),
      _activeDepthBuffer(false),
      _statusCheckQueued(false),
      _framebufferHandle(0),
      _prevViewport(-1),
      _activeReadBuffer(GL_NONE),
      _debugMessage(("Render Target: [ " + name() + " ]"))
{
    glCreateFramebuffers(1, &_framebufferHandle);
    assert(_framebufferHandle != 0 && "glFramebuffer error: Tried to bind an invalid framebuffer!");

    _isLayeredDepth = false;

    if_constexpr(Config::ENABLE_GPU_VALIDATION) {
        // label this FB to be able to tell that it's internally created and nor from a 3rd party lib
        glObjectLabel(GL_FRAMEBUFFER,
                      _framebufferHandle,
                      -1,
                      Util::StringFormat("DVD_FB_%d", _framebufferHandle).c_str());
    }

    // Everything disabled so that the initial "begin" will override this
    _previousPolicy.drawMask().disableAll();
    _activeColourBuffers.fill(GL_NONE);
}

glFramebuffer::~glFramebuffer()
{
    GL_API::deleteFramebuffers(1, &_framebufferHandle);
}

void glFramebuffer::initAttachment(RTAttachmentType type, U8 index) {
    // Avoid invalid dimensions
    assert(getWidth() != 0 && getHeight() != 0 && "glFramebuffer error: Invalid frame buffer dimensions!");

    // Process only valid attachments
    RTAttachment* attachment = _attachmentPool->get(type, index).get();
    if (!attachment->used()) {
        return;
    }   

    Texture* tex = nullptr;
    if (!attachment->isExternal()) {
        tex = attachment->texture().get();
        // Do we need to resize the attachment?
        const bool shouldResize = tex->width() != getWidth() || tex->height() != getHeight();
        if (shouldResize) {
            tex->resize(NULL, vec2<U16>(getWidth(), getHeight()));
        }
        const bool updateSampleCount = tex->descriptor().msaaSamples() != _descriptor._msaaSamples;
        if (updateSampleCount) {
            tex->setSampleCount(_descriptor._msaaSamples);
        }
    } else {
        RTAttachment* attachmentTemp = _attachmentPool->get(type, index).get();
        if (attachmentTemp->isExternal()) {
            RenderTarget& parent = attachmentTemp->parent().parent();
            attachmentTemp = &parent.getAttachment(attachmentTemp->getExternal()->descriptor()._type, attachmentTemp->getExternal()->descriptor()._index);
        }

        attachment->setTexture(attachmentTemp->texture(true));
        attachment->clearChanged();
        tex = attachment->texture(false).get();
    }
    assert(IsValid(tex->data()));
    // Find the appropriate binding point
    GLenum attachmentEnum;
    if (type == RTAttachmentType::Depth) {
        attachmentEnum = GL_DEPTH_ATTACHMENT;
        const TextureType texType = tex->data()._textureType;
        _isLayeredDepth = (texType == TextureType::TEXTURE_2D_ARRAY ||
                           texType == TextureType::TEXTURE_2D_ARRAY_MS ||
                           texType == TextureType::TEXTURE_CUBE_MAP ||
                           texType == TextureType::TEXTURE_CUBE_ARRAY ||
                           texType == TextureType::TEXTURE_3D);
    } else {
        attachmentEnum = GLenum((U32)GL_COLOR_ATTACHMENT0 + index);
    }

    attachment->binding(to_U32(attachmentEnum));
    attachment->clearChanged();
}

void glFramebuffer::toggleAttachment(const RTAttachment& attachment, AttachmentState state, bool layeredRendering) {
    OPTICK_EVENT();

    const Texture_ptr& tex = attachment.texture(false);
    if (layeredRendering && tex->numLayers() == 1) {
        layeredRendering = false;
    }

    if(attachment.used() && state != AttachmentState::STATE_DISABLED) {
        const GLenum binding = static_cast<GLenum>(attachment.binding());
        const BindingState bState{ state,
                                   attachment.mipWriteLevel(),
                                   attachment.writeLayer(),
                                   layeredRendering };
        // Compare with old state
        if (bState != getAttachmentState(binding)) {
            const GLuint handle = tex->data()._textureHandle;
            if (layeredRendering) {
                glNamedFramebufferTextureLayer(_framebufferHandle, binding, handle, bState._writeLevel, bState._writeLayer);
            } else {
                glNamedFramebufferTexture(_framebufferHandle, binding, handle, bState._writeLevel);
            }
            queueCheckStatus();
            setAttachmentState(binding, bState);
        }
    }
}

bool glFramebuffer::create() {
    if (!RenderTarget::create() && _attachmentPool == nullptr) {
        return false;
    }

    // For every attachment, be it a colour or depth attachment ...
    I32 attachmentCountTotal = 0;
    for (U8 i = 0; i < to_base(RTAttachmentType::COUNT); ++i) {
        for (U8 j = 0; j < _attachmentPool->attachmentCount(static_cast<RTAttachmentType>(i)); ++j) {
            initAttachment(static_cast<RTAttachmentType>(i), j);
            assert(GL_API::s_maxFBOAttachments > ++attachmentCountTotal);
        }
    }
    ACKNOWLEDGE_UNUSED(attachmentCountTotal);

    setDefaultState({});

    return checkStatus();
}

namespace BlitHelpers {
    inline RTAttachment* prepareAttachments(glFramebuffer* fbo, RTAttachment* att, U16 layer) {
        const bool layerChanged = att->writeLayer(layer);
        if (layerChanged || att->numLayers() > 0) {
            const glFramebuffer::BindingState& state = fbo->getAttachmentState(static_cast<GLenum>(att->binding()));
            fbo->toggleAttachment(*att, state._attState, att->numLayers() > 0u || layer > 0u);
        }
        return att;
    };

    inline RTAttachment* prepareAttachments(glFramebuffer* fbo, RTAttachment* att, const ColourBlitEntry& entry, bool isInput) {
        return prepareAttachments(fbo, att, isInput ? entry._inputLayer : entry._outputLayer);
    };

    inline RTAttachment* prepareAttachments(glFramebuffer* fbo, RTAttachment* att, const DepthBlitEntry& entry, bool isInput) {
        return prepareAttachments(fbo, att, isInput ? entry._inputLayer : entry._outputLayer);
    };

    inline RTAttachment* prepareAttachments(glFramebuffer* fbo, const ColourBlitEntry& entry, bool isInput) {
        RTAttachment* att = fbo->getAttachmentPtr(RTAttachmentType::Colour, to_U8(entry._inputIndex)).get();
        return prepareAttachments(fbo, att, entry, isInput);
    };

    inline RTAttachment* prepareAttachments(glFramebuffer* fbo, const DepthBlitEntry& entry, bool isInput) {
        RTAttachment* att = fbo->getAttachmentPtr(RTAttachmentType::Depth, 0u).get();
        return prepareAttachments(fbo, att, entry, isInput);
    };
};

void glFramebuffer::blitFrom(const RTBlitParams& params) {
    OPTICK_EVENT();

    if (!params._inputFB || (params._blitColours.empty() && !IsValid(params._blitDepth))) {
        return;
    }

    glFramebuffer* input = static_cast<glFramebuffer*>(params._inputFB);
    glFramebuffer* output = this;
    const vec2<U16>& inputDim = input->_descriptor._resolution;
    const vec2<U16>& outputDim = output->_descriptor._resolution;

    bool blittedDepth = false;
    // Multiple attachments, multiple layers, multiple everything ... what a mess ... -Ionut
    if (!params._blitColours.empty() && hasColour()) {

        const RTAttachmentPool::PoolEntry& inputAttachments = input->_attachmentPool->get(RTAttachmentType::Colour);
        const RTAttachmentPool::PoolEntry& outputAttachments = output->_attachmentPool->get(RTAttachmentType::Colour);

        GLuint prevReadAtt = 0;
        GLuint prevWriteAtt = 0;

        for (const ColourBlitEntry& entry : params._blitColours) {
            const RTAttachment_ptr& inAtt = inputAttachments[entry._inputIndex];
            const RTAttachment_ptr& outAtt = outputAttachments[entry._outputIndex];

            const GLuint crtReadAtt = inAtt->binding();
            const GLenum readBuffer = static_cast<GLenum>(crtReadAtt);
            if (prevReadAtt != readBuffer) {
                if (readBuffer != input->_activeReadBuffer) {
                    input->_activeReadBuffer = readBuffer;
                    glNamedFramebufferReadBuffer(input->_framebufferHandle, readBuffer);
                }
                prevReadAtt = crtReadAtt;
            }

            const GLuint crtWriteAtt = outAtt->binding();
            if (prevWriteAtt != crtWriteAtt) {
                const GLenum colourAttOut = static_cast<GLenum>(crtWriteAtt);
                bool set = _activeColourBuffers[0] != colourAttOut;
                if (!set) {
                    for (size_t i = 1; i < _activeColourBuffers.size(); ++i) {
                        if (_activeColourBuffers[i] != GL_NONE) {
                            set = true;
                            break;
                        }
                    }
                }
                if (set) {
                    output->_activeColourBuffers.fill(GL_NONE);
                    output->_activeColourBuffers[0] = colourAttOut;
                    glNamedFramebufferDrawBuffer(output->_framebufferHandle, colourAttOut);
                }

                
                prevWriteAtt = crtWriteAtt;
            }

            BlitHelpers::prepareAttachments(input, inAtt.get(), entry, true);
            BlitHelpers::prepareAttachments(output, outAtt.get(), entry, false);

            // If we change layers, then the depth buffer should match that ... I guess ... this sucks!
            if (input->hasDepth()) {
                BlitHelpers::prepareAttachments(input, entry, true);
            }

            if (output->hasDepth()) {
                BlitHelpers::prepareAttachments(output, entry, false);
            }

            blittedDepth = (!blittedDepth &&
                            params._blitDepth._inputLayer == entry._inputLayer &&
                            params._blitDepth._outputLayer == entry._outputLayer);
            
            glBlitNamedFramebuffer(input->_framebufferHandle,
                                   output->_framebufferHandle,
                                   0, 0,
                                   inputDim.width, inputDim.height,
                                   0, 0,
                                   outputDim.width, outputDim.height,
                                   blittedDepth ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT 
                                                : GL_COLOR_BUFFER_BIT,
                                   GL_NEAREST);
            _context.registerDrawCall();
            queueMipMapRecomputation(*outAtt, 0u, entry._outputLayer);
        }
    }

    if (!blittedDepth && hasDepth() && IsValid(params._blitDepth)) {
                               BlitHelpers::prepareAttachments(input, params._blitDepth, true);
        RTAttachment* outAtt = BlitHelpers::prepareAttachments(output,  params._blitDepth, false);

        glBlitNamedFramebuffer(input->_framebufferHandle,
                               output->_framebufferHandle,
                               0, 0,
                               inputDim.width, inputDim.height,
                               0, 0,
                               outputDim.width, outputDim.height,
                               GL_DEPTH_BUFFER_BIT,
                               GL_NEAREST);
        _context.registerDrawCall();
        queueMipMapRecomputation(*outAtt, 0u, params._blitDepth._outputLayer);
    }
}

void glFramebuffer::setBlendState(const RTDrawDescriptor& drawPolicy, const RTAttachmentPool::PoolEntry& activeAttachments) {
    OPTICK_EVENT();

    const RTDrawMask& mask = drawPolicy.drawMask();

    for (U8 i = 0; i < activeAttachments.size(); ++i) {
        if (mask.isEnabled(RTAttachmentType::Colour, i)) {
            const RTAttachment_ptr& colourAtt = activeAttachments[i];

            const RTBlendState& blend = drawPolicy.blendState(i);

            // Set blending per attachment if specified. Overrides general blend state
            GL_API::getStateTracker().setBlending(static_cast<GLuint>(colourAtt->binding() - to_U32(GL_COLOR_ATTACHMENT0)), blend._blendProperties);
            GL_API::getStateTracker().setBlendColour(blend._blendColour);
        }
    }
}

void glFramebuffer::prepareBuffers(const RTDrawDescriptor& drawPolicy, const RTAttachmentPool::PoolEntry& activeAttachments) {
    OPTICK_EVENT();

    const RTDrawMask& mask = drawPolicy.drawMask();

    if (_previousPolicy.drawMask() != mask) {
        bool set = false;
        // handle colour buffers first
        const U8 count = to_U8(std::min(to_size(MAX_RT_COLOUR_ATTACHMENTS), activeAttachments.size()));
        for (U8 j = 0; j < MAX_RT_COLOUR_ATTACHMENTS; ++j) {
            GLenum temp = GL_NONE;
            if (j < count) {
                temp = mask.isEnabled(RTAttachmentType::Colour, j) ? static_cast<GLenum>(activeAttachments[j]->binding()) : GL_NONE;
            }
            if (_activeColourBuffers[j] != temp) {
                _activeColourBuffers[j] = temp;
                set = true;
            }
        }

        if (set) {
            glNamedFramebufferDrawBuffers(_framebufferHandle,
                                          MAX_RT_COLOUR_ATTACHMENTS,
                                          _activeColourBuffers.data());
        }

        queueCheckStatus();

        const RTAttachment_ptr& depthAtt = _attachmentPool->get(RTAttachmentType::Depth, 0);
        _activeDepthBuffer = depthAtt && depthAtt->used();
     }
    
    if (mask.isEnabled(RTAttachmentType::Depth) != _zWriteEnabled) {
        _zWriteEnabled = !_zWriteEnabled;
        glDepthMask(_zWriteEnabled ? GL_TRUE : GL_FALSE);
    }
}

void glFramebuffer::toggleAttachments() {
    OPTICK_EVENT();

    for (U8 i = 0; i < to_base(RTAttachmentType::COUNT); ++i) {
        /// Get the attachments in use for each type
        const RTAttachmentPool::PoolEntry& attachments = _attachmentPool->get(static_cast<RTAttachmentType>(i));

        /// Reset attachments if they changed (e.g. after layered rendering);
        for (const RTAttachment_ptr& attachment : attachments) {
            /// We also draw to mip and layer 0 unless specified otherwise in the drawPolicy
            attachment->writeLayer(0);
            attachment->mipWriteLevel(0);

            /// All active attachments are enabled by default
            toggleAttachment(*attachment, AttachmentState::STATE_ENABLED, false);
        }
    }
}

void glFramebuffer::clear(const RTClearDescriptor& descriptor) {
    OPTICK_EVENT();

    if (descriptor.resetToDefault()) {
        toggleAttachments();
    }

    const RTAttachmentPool::PoolEntry& colourAttachments = _attachmentPool->get(RTAttachmentType::Colour);

    if (descriptor.resetToDefault()) {
        prepareBuffers({}, colourAttachments);
    }

    /// Clear the draw buffers
    clear(descriptor, colourAttachments);
}

void glFramebuffer::setDefaultState(const RTDrawDescriptor& drawPolicy) {
    toggleAttachments();

    const RTAttachmentPool::PoolEntry& colourAttachments = _attachmentPool->get(RTAttachmentType::Colour);

    /// Setup draw buffers
    prepareBuffers(drawPolicy, colourAttachments);

    /// Set the depth range
    GL_API::getStateTracker().setDepthRange(_descriptor._depthRange.min, _descriptor._depthRange.max);

    /// Set the blend states
    setBlendState(drawPolicy, colourAttachments);

    /// Check that everything is valid
    checkStatus();
}

void glFramebuffer::begin(const RTDrawDescriptor& drawPolicy) {
    OPTICK_EVENT();

    // Push debug state
    GL_API::pushDebugMessage(_debugMessage.c_str(), _framebufferHandle);

    // Activate FBO
    GL_API::getStateTracker().setActiveFB(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY, _framebufferHandle);

    // Set the viewport
    if (drawPolicy.setViewport()) {
        _prevViewport.set(_context.getViewport());
        _context.setViewport(0, 0, to_I32(getWidth()), to_I32(getHeight()));
    }

    setDefaultState(drawPolicy);

    // Memorize the current draw policy to speed up later calls
    _previousPolicy = drawPolicy;
}

void glFramebuffer::end(bool needsUnbind) {
    OPTICK_EVENT();

    if (needsUnbind) {
        GL_API::getStateTracker().setActiveFB(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY, 0);
        if (_previousPolicy.setViewport()) {
            _context.setViewport(_prevViewport);
        }
    }

    queueMipMapRecomputation();

    GL_API::popDebugMessage();
}

void glFramebuffer::queueMipMapRecomputation() {
    if (hasColour()) {
        const RTAttachmentPool::PoolEntry& colourAttachments = _attachmentPool->get(RTAttachmentType::Colour);
        for (const RTAttachment_ptr& att : colourAttachments) {
            queueMipMapRecomputation(*att, 0u, att->descriptor()._texDescriptor.layerCount());
        }
    }

    if (hasDepth()) {
        const RTAttachment_ptr& attDepth = _attachmentPool->get(RTAttachmentType::Depth, 0);
        queueMipMapRecomputation(*attDepth, 0u, attDepth->descriptor()._texDescriptor.layerCount());
    }
}

void glFramebuffer::queueMipMapRecomputation(const RTAttachment& attachment, U16 startLayer, U16 layerCount) {
    const Texture_ptr& texture = attachment.texture(false);
    if (attachment.used() && texture->automaticMipMapGeneration() && texture->getCurrentSampler().generateMipMaps()) {
        GL_API::queueComputeMipMap(texture->data()._textureHandle);
    }
}

void glFramebuffer::clear(const RTClearDescriptor& drawPolicy, const RTAttachmentPool::PoolEntry& activeAttachments) const {
    OPTICK_EVENT();

    if (drawPolicy.clearColours() && hasColour()) {
        for (const RTAttachment_ptr& att : activeAttachments) {
            const U32 binding = att->binding();
            if (static_cast<GLenum>(binding) != GL_NONE) {

                if (!drawPolicy.clearExternalColour() && att->isExternal()) {
                    continue;
                }

                const GLint buffer = static_cast<GLint>(binding - static_cast<GLint>(GL_COLOR_ATTACHMENT0));

                if (drawPolicy.clearColour(to_U8(buffer))) {
                    switch (att->texture(false)->descriptor().dataType()) {
                        case GFXDataFormat::FLOAT_16:
                        case GFXDataFormat::FLOAT_32 :
                            glClearNamedFramebufferfv(_framebufferHandle, GL_COLOR, buffer, att->clearColour()._v);
                            break;
                    
                        case GFXDataFormat::SIGNED_BYTE:
                        case GFXDataFormat::SIGNED_SHORT:
                        case GFXDataFormat::SIGNED_INT :
                            glClearNamedFramebufferiv(_framebufferHandle, GL_COLOR, buffer, Util::ToIntColour(att->clearColour())._v);
                            break;

                        default:
                            glClearNamedFramebufferuiv(_framebufferHandle, GL_COLOR, buffer, Util::ToUIntColour(att->clearColour())._v);
                            break;
                    }
                    _context.registerDrawCall();
                }
            }
        }
    }

    if (drawPolicy.clearDepth() && hasDepth()) {
        if (drawPolicy.clearExternalDepth() && _attachmentPool->get(RTAttachmentType::Depth, 0)->isExternal()) {
            return;
        }

        glClearNamedFramebufferfv(_framebufferHandle, GL_DEPTH, 0, &_descriptor._depthValue);
        _context.registerDrawCall();
    }
}

void glFramebuffer::drawToLayer(const DrawLayerParams& params) {
    OPTICK_EVENT();

    if (params._type == RTAttachmentType::COUNT) {
        return;
    }

    const RTAttachment_ptr& att = _attachmentPool->get(params._type, params._index);

    const GLenum textureType = GLUtil::glTextureTypeTable[to_U32(att->texture(false)->data()._textureType)];
    // only for array textures (it's better to simply ignore the command if the format isn't supported (debugging reasons)
    if (textureType != GL_TEXTURE_2D_ARRAY &&
        textureType != GL_TEXTURE_CUBE_MAP_ARRAY &&
        textureType != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        return;
    }

    const bool useDepthLayer =  (hasDepth()  && params._includeDepth) ||
                                (hasDepth()  && params._type == RTAttachmentType::Depth);
    const bool useColourLayer = (hasColour() && params._type == RTAttachmentType::Colour);

    if (useDepthLayer && _isLayeredDepth) {
        const RTAttachment_ptr& attDepth = _attachmentPool->get(RTAttachmentType::Depth, 0);
        const BindingState& state = getAttachmentState(static_cast<GLenum>(attDepth->binding()));
        if (attDepth->writeLayer(params._layer)) {
            toggleAttachment(*attDepth, state._attState, true);
        }
    }

    if (useColourLayer) {
        const BindingState& state = getAttachmentState(static_cast<GLenum>(att->binding()));
        if (att->writeLayer(params._layer)) {
            toggleAttachment(*att, state._attState, true);
        }
    }

    if (params._validateLayer) {
        checkStatus();
    } else {
        _statusCheckQueued = false;
    }
}

void glFramebuffer::setMipLevel(U16 writeLevel, bool validate) {
    OPTICK_EVENT();

    if (writeLevel == std::numeric_limits<U16>::max()) {
        return;
    }

    // This is needed because certain drivers need all attachments to use the same mip level
    // This is also VERY SLOW so it might be worth optimising it per-driver version / IHV
    for (U8 i = 0; i < to_base(RTAttachmentType::COUNT); ++i) {
        const RTAttachmentPool::PoolEntry& attachments = _attachmentPool->get(static_cast<RTAttachmentType>(i));

        for (const RTAttachment_ptr& attachment : attachments) {
            const Texture_ptr& texture = attachment->texture(false);
            if (texture->getMipCount() > writeLevel && !texture->descriptor().isMultisampledTexture())
            {
                const BindingState& state = getAttachmentState(static_cast<GLenum>(attachment->binding()));
                attachment->mipWriteLevel(writeLevel);
                toggleAttachment(*attachment, state._attState, state._layeredRendering);
            } else {
                //nVidia still has issues if attachments are not all at the same mip level -Ionut
                if (GFXDevice::getGPUVendor() == GPUVendor::NVIDIA) {
                    toggleAttachment(*attachment, AttachmentState::STATE_DISABLED, false);
                }
            }
        }
    }
    if (validate) {
        checkStatus();
    } else {
        _statusCheckQueued = false;
    }
}

void glFramebuffer::readData(const vec4<U16>& rect,
                             GFXImageFormat imageFormat,
                             GFXDataFormat dataType,
                             bufferPtr outData) const {
    OPTICK_EVENT();

    GL_API::getStateTracker().setPixelPackUnpackAlignment();
    GL_API::getStateTracker().setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_ONLY, _framebufferHandle);
    glReadPixels(
        rect.x, rect.y, rect.z, rect.w,
        GLUtil::glImageFormatTable[to_U32(imageFormat)],
        GLUtil::glDataFormat[to_U32(dataType)], outData);
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
    const hashMap<GLenum, BindingState>::const_iterator it = _attachmentState.find(binding);
    if (it != std::cend(_attachmentState)) {
        return it->second;
    }

    return { AttachmentState::COUNT, 0, 0 };
}

void glFramebuffer::queueCheckStatus() {
    _statusCheckQueued = true;
}

bool glFramebuffer::checkStatus() {
    OPTICK_EVENT();

    if (!_statusCheckQueued) {
        return true;
    }

    _statusCheckQueued = false;
    if_constexpr(Config::ENABLE_GPU_VALIDATION) {
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
            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT: {
                Console::errorfn(Locale::get(_ID("ERROR_RT_DIMENSIONS")));
                return false;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT: {
                Console::errorfn(Locale::get(_ID("ERROR_RT_FORMAT")));
                return false;
            }
            default: {
                Console::errorfn(Locale::get(_ID("ERROR_UNKNOWN")));
                return false;
            }
        };
    }

    return false;
}

};  // namespace Divide
