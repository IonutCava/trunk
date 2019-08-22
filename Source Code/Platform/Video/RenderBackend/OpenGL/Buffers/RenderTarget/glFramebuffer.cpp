#include "stdafx.h"

#include "config.h"

#include "Headers/glFramebuffer.h"

#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"
#include "Platform/Video/RenderBackend/OpenGL/Textures/Headers/glSamplerObject.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
    TextureType GetNonMSType(TextureType type) {
        if (type == TextureType::TEXTURE_2D_MS) {
            return TextureType::TEXTURE_2D;
        }

        if (type == TextureType::TEXTURE_2D_ARRAY_MS) {
            return TextureType::TEXTURE_2D_ARRAY;
        }

        return type;
    }
};


bool glFramebuffer::_zWriteEnabled = true;

glFramebuffer::glFramebuffer(GFXDevice& context, glFramebuffer* parent, const RenderTargetDescriptor& descriptor)
    : RenderTarget(context, descriptor),
      glObject(glObjectType::TYPE_FRAMEBUFFER, context),
      _parent(parent),
      _resolveBuffer(nullptr),
      _isLayeredDepth(false),
      _activeDepthBuffer(false),
      _statusCheckQueued(false),
      _hasMultisampledColourAttachments(false),
      _framebufferHandle(0),
      _prevViewport(-1)
{
    glCreateFramebuffers(1, &_framebufferHandle);
    assert(_framebufferHandle != 0 && "glFramebuffer error: Tried to bind an invalid framebuffer!");

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
}

glFramebuffer::~glFramebuffer()
{
    GL_API::deleteFramebuffers(1, &_framebufferHandle);
    MemoryManager::SAFE_DELETE(_resolveBuffer);
}

RTAttachment* glFramebuffer::getAttachmentInternal(RTAttachmentType type, U8 index) {
    RTAttachment* attachment = _attachmentPool->get(type, index).get();
    if (attachment->isExternal()) {
        glFramebuffer* parent = static_cast<glFramebuffer&>(attachment->parent().parent())._parent;
        if (parent != nullptr) {
            attachment = &parent->getInternalAttachment(attachment->getExternal()->descriptor()._type, attachment->getExternal()->descriptor()._index);
        }
    }

    return attachment;
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
        bool shouldResize = tex->getWidth() != getWidth() || tex->getHeight() != getHeight();
        if (shouldResize) {
            tex->resize(NULL, vec2<U16>(getWidth(), getHeight()));
        }
    } else {
        RTAttachment* attachmentTemp = getAttachmentInternal(type, index);
        attachment->setTexture(attachmentTemp->texture(true));
        attachment->clearChanged();
        tex = attachment->texture(false).get();
    }

    // Find the appropriate binding point
    GLenum attachmentEnum;
    if (type == RTAttachmentType::Depth) {
        attachmentEnum = GL_DEPTH_ATTACHMENT;

        TextureType texType = tex->getData().type();
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

void glFramebuffer::toggleAttachment(const RTAttachment& attachment, AttachmentState state, bool layeredRendering) {

    GLenum binding = static_cast<GLenum>(attachment.binding());

    if (attachment.texture(false)->getNumLayers() == 1) {
        layeredRendering = false;
    }

    BindingState bState{ state,
                         attachment.mipWriteLevel(),
                         attachment.writeLayer(),
                         layeredRendering };

    BindingState oldState = getAttachmentState(binding);
    if (bState != oldState) {
        if (!attachment.used() || bState._attState == AttachmentState::STATE_DISABLED) {
            return;
        }

        GLuint handle = attachment.texture(false)->getData().textureHandle();
        if (layeredRendering) {
            glNamedFramebufferTextureLayer(_framebufferHandle, binding, handle, bState._writeLevel, bState._writeLayer);
        } else {
            glNamedFramebufferTexture(_framebufferHandle, binding, handle, bState._writeLevel);
        }
        queueCheckStatus();
        setAttachmentState(binding, bState);
    }
}

bool glFramebuffer::create() {
    if (!RenderTarget::create()) {
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

    // If this is a multisampled FBO, make sure we have a resolve buffer
    if (_hasMultisampledColourAttachments && _resolveBuffer == nullptr) {
        vector<RTAttachmentDescriptor> attachments;
        for (U8 i = 0; i < to_base(RTAttachmentType::COUNT); ++i) {
            for (U8 j = 0; j < _attachmentPool->attachmentCount(static_cast<RTAttachmentType>(i)); ++j) {
                RTAttachment* att = _attachmentPool->get(static_cast<RTAttachmentType>(i), j).get();
                if (att->used()) {
                    RTAttachmentDescriptor descriptor = {};
                    descriptor._texDescriptor = att->texture(false)->getDescriptor();
                    descriptor._texDescriptor._type = GetNonMSType(descriptor._texDescriptor._type);
                    descriptor._texDescriptor._msaaSamples = 0;
                    descriptor._clearColour = att->clearColour();
                    descriptor._index = j;
                    descriptor._type = static_cast<RTAttachmentType>(i);
                    attachments.emplace_back(descriptor);
                }
            }
        }

        RenderTargetDescriptor desc = {};
        desc._name = name() + "_resolve";
        desc._resolution = vec2<U16>(getWidth(), getHeight());
        desc._attachmentCount = to_U8(attachments.size());
        desc._attachments = attachments.data();

        _resolveBuffer = MemoryManager_NEW glFramebuffer(context(), this, desc);
        if (!_resolveBuffer->create()) {
            return false;
        }
    }

    RTDrawDescriptor defaultDescriptor = RenderTarget::defaultPolicy();
    defaultDescriptor.clearExternalColour(false);
    defaultDescriptor.clearExternalDepth(false);
    setDefaultState(defaultDescriptor);

    return checkStatus();
}

bool glFramebuffer::resize(U16 width, U16 height) {
    if (Config::Profile::USE_2x2_TEXTURES) {
        width = height = 2u;
    }

    if (getWidth() == width && getHeight() == height) {
        return false;
    }

    _descriptor._resolution.set(width, height);

    if (_resolveBuffer != nullptr) {
        _resolveBuffer->resize(width, height);
    }

    return create();
}

void glFramebuffer::resolve(I8 colour, bool allColours, bool depth, bool externalColours) {
    if (_resolveBuffer != nullptr) {
        RTBlitParams params = {};
        params._inputFB = this;
        if (allColours || colour != -1) {
            const RTAttachmentPool::PoolEntry& colourAtt = _attachmentPool->get(RTAttachmentType::Colour);
            for (const RTAttachment_ptr& att : colourAtt) {
                ColourBlitEntry entry = {};
                entry._inputIndex = entry._outputIndex = to_U16(att->binding() - to_U32(GL_COLOR_ATTACHMENT0));
                eastl::set<U16, eastl::greater<U16>>& layers = _attachmentResolvedLayers[static_cast<GLenum>(att->binding())];
                for (U16 layer : layers) {
                    if (att->isExternal() && !externalColours) {
                        continue;
                    }
                    entry._inputLayer = entry._outputLayer = layer;
                    params._blitColours.push_back(entry);
                }
                layers.clear();
                if (!allColours && entry._inputIndex == colour) {
                    break;
                }
            }
        }

        if (depth) {
            eastl::set<U16, eastl::greater<U16>>& layers = _attachmentResolvedLayers[GL_DEPTH_ATTACHMENT];
            for (U16 layer : layers) {
                params._blitDepth.push_back({ layer, layer });
            }
            layers.clear();
        }

        _resolveBuffer->blitFrom(params);
    }
}

void glFramebuffer::fastBlit(GLuint inputFB,
                             const vec2<GLuint>& inputDim,
                             const vec2<GLuint>& outputDim,
                             GLenum colourAttIn,
                             GLenum colourAttOut,
                             ClearBufferMask mask)
{
    if (colourAttIn != GL_NONE) {
        assert(colourAttOut != GL_NONE);
        glNamedFramebufferDrawBuffer(_framebufferHandle, colourAttOut);
        glNamedFramebufferReadBuffer(inputFB, colourAttIn);
    }

    glBlitNamedFramebuffer(inputFB, _framebufferHandle,
                           0, 0,
                           inputDim.w, inputDim.h,
                           0, 0,
                           outputDim.w, outputDim.h,
                           mask,
                           GL_NEAREST);

    _context.registerDrawCall();
}

void glFramebuffer::blitFrom(const RTBlitParams& params)
{
    if (!params._inputFB || (params._blitColours.empty() && params._blitDepth.empty())) {
        return;
    }

    glFramebuffer* input = static_cast<glFramebuffer*>(params._inputFB);
    vec2<GLuint> inputDim(input->getWidth(), input->getHeight());
    vec2<GLuint> outputDim(this->getWidth(), this->getHeight());

    // This seems hacky but it is the most common blit case so it is a really good idea to add it as a fast path. -Ionut
    // If we just blit depth, or just the first colour attachment (layer 0) or both colour 0 (layer 0) + depth
    if (params._blitColours.empty() || (params._blitColours.size() == 1 && params._blitColours.front()._inputLayer == 0)) {
        ClearBufferMask clearMask = GL_NONE_BIT;
        bool setDepthBlitFlag = !params._blitDepth.empty() && hasDepth();
        if (setDepthBlitFlag) {
            clearMask = GL_DEPTH_BUFFER_BIT;
        }

        GLenum colourAttIn = GL_NONE;
        GLenum colourAttOut = GL_NONE;
        if (!params._blitColours.empty() && hasColour()) {
            const ColourBlitEntry& entry = params._blitColours.front();

            clearMask |= GL_COLOR_BUFFER_BIT;
            colourAttIn = GL_COLOR_ATTACHMENT0 + entry._inputIndex;
            colourAttOut = GL_COLOR_ATTACHMENT0 + entry._outputIndex;

            RTAttachment* inAtt = input->_attachmentPool->get(RTAttachmentType::Colour, to_U8(entry._inputIndex)).get();
            RTAttachment* outAtt = this->_attachmentPool->get(RTAttachmentType::Colour, to_U8(entry._outputIndex)).get();

            bool layerChanged = inAtt->writeLayer(entry._inputLayer);
            if (layerChanged || inAtt->numLayers() > 0) {
                const BindingState& inState = input->getAttachmentState(static_cast<GLenum>(inAtt->binding()));
                input->toggleAttachment(*inAtt, inState._attState, inAtt->numLayers() > 0 || entry._inputLayer > 0);
            }

            layerChanged = outAtt->writeLayer(entry._outputLayer);
            if (layerChanged || outAtt->numLayers() > 0) {
                const BindingState& outState = this->getAttachmentState(static_cast<GLenum>(outAtt->binding()));
                this->toggleAttachment(*outAtt, outState._attState, outAtt->numLayers() > 0 || entry._outputLayer > 0);
            }

            queueMipMapRecomputation(*outAtt, vec2<U32>(0, entry._outputLayer));
        }

        fastBlit(input->_framebufferHandle, inputDim, outputDim, colourAttIn, colourAttOut, clearMask);
        
        return;
    }

    std::unordered_set<vec_size_eastl> blitDepthEntry;
    // Multiple attachments, multiple layers, multiple everything ... what a mess ... -Ionut
    if (!params._blitColours.empty() && hasColour()) {

        const RTAttachmentPool::PoolEntry& inputAttachments = input->_attachmentPool->get(RTAttachmentType::Colour);
        const RTAttachmentPool::PoolEntry& outputAttachments = this->_attachmentPool->get(RTAttachmentType::Colour);

        GLuint prevReadAtt = 0;
        GLuint prevWriteAtt = 0;

        for (const ColourBlitEntry& entry : params._blitColours) {
            const RTAttachment_ptr& inAtt = inputAttachments[entry._inputIndex];
            const RTAttachment_ptr& outAtt = outputAttachments[entry._outputIndex];

            GLuint crtReadAtt = inAtt->binding();
            if (prevReadAtt != crtReadAtt) {
                glNamedFramebufferReadBuffer(input->_framebufferHandle, static_cast<GLenum>(crtReadAtt));
                prevReadAtt = crtReadAtt;
            }

            GLuint crtWriteAtt = outAtt->binding();
            if (prevWriteAtt != crtWriteAtt) {
                glNamedFramebufferDrawBuffer(this->_framebufferHandle, static_cast<GLenum>(crtWriteAtt));
                prevWriteAtt = crtWriteAtt;
            }

            bool layerChanged = inAtt->writeLayer(entry._inputLayer);
            const BindingState& inState = input->getAttachmentState(static_cast<GLenum>(crtReadAtt));
            if (layerChanged || inAtt->numLayers() > 0) {
                input->toggleAttachment(*inAtt, inState._attState, inAtt->numLayers() > 0 || entry._inputLayer > 0);
            }

            layerChanged = outAtt->writeLayer(entry._outputLayer);
            const BindingState& outState = this->getAttachmentState(static_cast<GLenum>(crtWriteAtt));
            if (layerChanged || outAtt->numLayers() > 0) {
                this->toggleAttachment(*outAtt, outState._attState, outAtt->numLayers() > 0 || entry._outputLayer > 0);
            }

            // If we change layers, then the depth buffer should match that ... I guess ... this sucks!
            if (input->hasDepth()) {
                const RTAttachment_ptr& inDepthAtt = input->_attachmentPool->get(RTAttachmentType::Depth, 0);
                layerChanged = inDepthAtt->writeLayer(entry._inputLayer);
                const BindingState& inDepthState = input->getAttachmentState(GL_DEPTH_ATTACHMENT);
                if (layerChanged || inDepthAtt->numLayers() > 0) {
                    input->toggleAttachment(*inDepthAtt, inDepthState._attState, inDepthAtt->numLayers() > 0 || entry._inputLayer > 0);
                }
            }

            if (this->hasDepth()) {
                const RTAttachment_ptr& outDepthAtt = this->_attachmentPool->get(RTAttachmentType::Depth, 0);
                layerChanged = outDepthAtt->writeLayer(entry._outputLayer);
                const BindingState& outDepthState = this->getAttachmentState(GL_DEPTH_ATTACHMENT);
                if (layerChanged || outDepthAtt->numLayers() > 0) {
                    this->toggleAttachment(*outDepthAtt, outDepthState._attState, outDepthAtt->numLayers() > 0 || entry._outputLayer > 0);
                }
            }

            // We always change depth layers to satisfy whatever f**ked up completion requirements the OpenGL driver has (looking at you Nvidia)
            bool shouldBlitDepth = false;
            for (vec_size_eastl idx = 0; idx < params._blitDepth.size(); ++idx) {
                const DepthBlitEntry& depthEntry = params._blitDepth[idx];
                // MAYBE, we need to blit this combo of depth layers. 
                if (depthEntry._inputLayer == entry._inputLayer && depthEntry._outputLayer == entry._outputLayer) {
                    // If so, blit it now and skip it later. Hey ... micro-optimisation here :D
                    blitDepthEntry.insert(idx);
                    shouldBlitDepth = true;
                    break;

                }
            }
            
            glBlitNamedFramebuffer(input->_framebufferHandle, this->_framebufferHandle,
                                   0, 0,
                                   inputDim.w, inputDim.h,
                                   0, 0,
                                   outputDim.w, outputDim.h,
                                   shouldBlitDepth ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT 
                                                   : GL_COLOR_BUFFER_BIT,
                                   GL_NEAREST);
            _context.registerDrawCall();
            queueMipMapRecomputation(*outAtt, vec2<U32>(0, entry._outputLayer));
        }
    }

    if (!params._blitDepth.empty() && hasDepth()) {
        for (vec_size_eastl idx = 0; idx < params._blitDepth.size(); ++idx) {
            // Already blitted
            if (blitDepthEntry.find(idx) != std::cend(blitDepthEntry)) {
                continue;
            }

            const DepthBlitEntry& entry = params._blitDepth[idx];
        
            const RTAttachment_ptr& inDepthAtt = input->_attachmentPool->get(RTAttachmentType::Depth, 0);
            const RTAttachment_ptr& outDepthAtt = this->_attachmentPool->get(RTAttachmentType::Depth, 0);

            const BindingState& inState = input->getAttachmentState(GL_DEPTH_ATTACHMENT);
            bool layerChanged = inDepthAtt->writeLayer(entry._inputLayer);
            if (layerChanged || inDepthAtt->numLayers() > 0) {
                input->toggleAttachment(*inDepthAtt, inState._attState, inDepthAtt->numLayers() > 0 || entry._inputLayer > 0);
            }
            

            const BindingState& outState = this->getAttachmentState(GL_DEPTH_ATTACHMENT);
            layerChanged = outDepthAtt->writeLayer(entry._outputLayer);
            if (layerChanged || outDepthAtt->numLayers() > 0) {
                this->toggleAttachment(*outDepthAtt, outState._attState, outDepthAtt->numLayers() > 0 || entry._outputLayer > 0);
            }
            

            glBlitNamedFramebuffer(input->_framebufferHandle, this->_framebufferHandle,
                                   0, 0,
                                   inputDim.w, inputDim.h,
                                   0, 0,
                                   outputDim.w, outputDim.h,
                                   GL_DEPTH_BUFFER_BIT,
                                   GL_NEAREST);
            _context.registerDrawCall();
            queueMipMapRecomputation(*outDepthAtt, vec2<U32>(0, entry._outputLayer));
        }
    }
}

const RTAttachment& glFramebuffer::getAttachment(RTAttachmentType type, U8 index, bool resolved) const {
    if (_resolveBuffer == nullptr || !resolved) {
        return getInternalAttachment(type, index, resolved);
    }

    return _resolveBuffer->getAttachment(type, index, false);
}

RTAttachment& glFramebuffer::getInternalAttachment(RTAttachmentType type, U8 index, bool resolved) {
    return RenderTarget::getAttachment(type, index, resolved);
}

const RTAttachment& glFramebuffer::getInternalAttachment(RTAttachmentType type, U8 index, bool resolved) const {
    return RenderTarget::getAttachment(type, index, resolved);
}

const RTAttachment_ptr& glFramebuffer::getAttachmentPtr(RTAttachmentType type, U8 index, bool resolved) const {
    if (_resolveBuffer == nullptr || !resolved) {
        return RenderTarget::getAttachmentPtr(type, index, resolved);
    }

    return _resolveBuffer->getAttachmentPtr(type, index, resolved);
}

RTAttachment& glFramebuffer::getAttachment(RTAttachmentType type, U8 index, bool resolved) {
    if (_resolveBuffer == nullptr || !resolved) {
        return RenderTarget::getAttachment(type, index, resolved); 
    }

    return _resolveBuffer->getAttachment(type, index, resolved);
}

void glFramebuffer::setBlendState(const RTDrawDescriptor& drawPolicy, const RTAttachmentPool::PoolEntry& activeAttachments) {
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
    const RTDrawMask& mask = drawPolicy.drawMask();

    if (_previousPolicy.drawMask() != mask) {
        // handle colour buffers first
        vector<GLenum> activeColourBuffers(MAX_RT_COLOUR_ATTACHMENTS, GL_NONE);

        U8 count = to_U8(std::min((size_t)MAX_RT_COLOUR_ATTACHMENTS, activeAttachments.size()));
        for (U8 j = 0; j < count; ++j) {
            const RTAttachment_ptr& colourAtt = activeAttachments[j];
            activeColourBuffers[j] =  mask.isEnabled(RTAttachmentType::Colour, j) ? static_cast<GLenum>(colourAtt->binding()) : GL_NONE;
        }

        glNamedFramebufferDrawBuffers(_framebufferHandle,
                                      static_cast<GLsizei>(activeColourBuffers.size()),
                                      activeColourBuffers.data());
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

void glFramebuffer::setDefaultState(const RTDrawDescriptor& drawPolicy) {
    toggleAttachments();

    const RTAttachmentPool::PoolEntry& colourAttachments = _attachmentPool->get(RTAttachmentType::Colour);

    /// Setup draw buffers
    prepareBuffers(drawPolicy, colourAttachments);

    /// Set the depth range
    GL_API::getStateTracker().setDepthRange(_descriptor._depthRange.min, _descriptor._depthRange.max);

    /// Set the blend states
    setBlendState(drawPolicy, colourAttachments);

    /// Clear the draw buffers
    clear(drawPolicy, colourAttachments);

    /// Check that everything is valid
    checkStatus();
}

void glFramebuffer::begin(const RTDrawDescriptor& drawPolicy) {
    /// Push debug state
    GL_API::pushDebugMessage(("FBO Begin: " + name()).c_str(), _framebufferHandle);

    /// Activate FBO
    GL_API::getStateTracker().setActiveFB(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY, _framebufferHandle);

    /// Set the viewport
    if (drawPolicy.isEnabledState(RTDrawDescriptor::State::CHANGE_VIEWPORT)) {
        _prevViewport.set(_context.getViewport());
        _context.setViewport(0, 0, to_I32(getWidth()), to_I32(getHeight()));
    }

    setDefaultState(drawPolicy);

    if (hasDepth() && drawPolicy.drawMask().isEnabled(RTAttachmentType::Depth)) {
        _attachmentResolvedLayers[GL_DEPTH_ATTACHMENT].insert(_attachmentState[GL_DEPTH_ATTACHMENT]._writeLayer);

        const std::unordered_set<U16>& additionalDirtyLayers = drawPolicy.getDirtyLayers(RTAttachmentType::Depth);
        for (U16 layer : additionalDirtyLayers) {
            _attachmentResolvedLayers[GL_DEPTH_ATTACHMENT].insert(layer);
        }
    }

    if (hasColour()) {
        for (U8 i = 0; i < MAX_RT_COLOUR_ATTACHMENTS; ++i) {
            if (drawPolicy.drawMask().isEnabled(RTAttachmentType::Colour, i)) {
                _attachmentResolvedLayers[GL_COLOR_ATTACHMENT0 + i].insert(_attachmentState[GL_COLOR_ATTACHMENT0 + i]._writeLayer);
                const std::unordered_set<U16>& additionalDirtyLayers = drawPolicy.getDirtyLayers(RTAttachmentType::Colour, i);
                for (U16 layer : additionalDirtyLayers) {
                    _attachmentResolvedLayers[GL_COLOR_ATTACHMENT0 + i].insert(layer);
                }
            }
        }
    }
    /// Memorize the current draw policy to speed up later calls
    _previousPolicy = drawPolicy;
}

void glFramebuffer::end(bool resolveMSAAColour, bool resolveMSAAExternalColour, bool resolveMSAADepth) {
    GL_API::getStateTracker().setActiveFB(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY, 0);
    if (_previousPolicy.isEnabledState(RTDrawDescriptor::State::CHANGE_VIEWPORT)) {
        _context.setViewport(_prevViewport);
    }

    queueMipMapRecomputation();

    if (resolveMSAAColour || resolveMSAAExternalColour || resolveMSAADepth) {
        const RTDrawMask& mask = _previousPolicy.drawMask();
        resolve(-1, resolveMSAAColour && mask.isEnabled(RTAttachmentType::Colour), resolveMSAADepth && mask.isEnabled(RTAttachmentType::Depth), resolveMSAAExternalColour);
    }

    GL_API::popDebugMessage();
}

void glFramebuffer::queueMipMapRecomputation() {
    if (hasColour()) {
        const RTAttachmentPool::PoolEntry& colourAttachments = _attachmentPool->get(RTAttachmentType::Colour);
        for (const RTAttachment_ptr& att : colourAttachments) {
            queueMipMapRecomputation(*att, vec2<U32>(0, att->descriptor()._texDescriptor._layerCount));
        }
    }

    if (hasDepth()) {
        const RTAttachment_ptr& attDepth = _attachmentPool->get(RTAttachmentType::Depth, 0);
        queueMipMapRecomputation(*attDepth, vec2<U32>(0, attDepth->descriptor()._texDescriptor._layerCount));
    }
}

void glFramebuffer::queueMipMapRecomputation(const RTAttachment& attachment, const vec2<U32>& layerRange) {
    const Texture_ptr& texture = attachment.texture(false);
    if (attachment.used() && texture->automaticMipMapGeneration() && texture->getCurrentSampler().generateMipMaps()) {
        GL_API::queueComputeMipMap(texture->getData().textureHandle());
    }
}

void glFramebuffer::clear(const RTDrawDescriptor& drawPolicy, const RTAttachmentPool::PoolEntry& activeAttachments) const {
    if (drawPolicy.isEnabledState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS) && hasColour()) {
        for (const RTAttachment_ptr& att : activeAttachments) {
            U32 binding = att->binding();
            if (static_cast<GLenum>(binding) != GL_NONE) {

                if (!drawPolicy.clearExternalColour() && att->isExternal()) {
                    continue;
                }

                GLint buffer = static_cast<GLint>(binding - static_cast<GLint>(GL_COLOR_ATTACHMENT0));
                if (!drawPolicy.clearColour(to_U8(buffer))) {
                    continue;
                }

                switch (att->texture(false)->getDescriptor().dataType()) {
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

    if (drawPolicy.isEnabledState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER) && hasDepth()) {
        if (drawPolicy.clearExternalDepth() && _attachmentPool->get(RTAttachmentType::Depth, 0)->isExternal()) {
            return;
        }

        glClearNamedFramebufferfv(_framebufferHandle, GL_DEPTH, 0, &_descriptor._depthValue);
        _context.registerDrawCall();
    }
}

void glFramebuffer::drawToLayer(const DrawLayerParams& params) {
    if (params._type == RTAttachmentType::COUNT) {
        return;
    }

    const RTAttachment_ptr& att = _attachmentPool->get(params._type, params._index);

    GLenum textureType = GLUtil::glTextureTypeTable[to_U32(att->texture(false)->getData().type())];
    // only for array textures (it's better to simply ignore the command if the format isn't supported (debugging reasons)
    if (textureType != GL_TEXTURE_2D_ARRAY &&
        textureType != GL_TEXTURE_CUBE_MAP_ARRAY &&
        textureType != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        return;
    }

    bool useDepthLayer =  (hasDepth()  && params._includeDepth) ||
                          (hasDepth()  && params._type == RTAttachmentType::Depth);
    bool useColourLayer = (hasColour() && params._type == RTAttachmentType::Colour);

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
    if (writeLevel == std::numeric_limits<U16>::max()) {
        return;
    }

    // This is needed because certain drivers need all attachments to use the same mip level
    // This is also VERY SLOW so it might be worth optimising it per-driver version / IHV
    for (U8 i = 0; i < to_base(RTAttachmentType::COUNT); ++i) {
        const RTAttachmentPool::PoolEntry& attachments = _attachmentPool->get(static_cast<RTAttachmentType>(i));

        for (const RTAttachment_ptr& attachment : attachments) {
            const Texture_ptr& texture = attachment->texture(false);
            if (texture->getMaxMipLevel() > writeLevel && !texture->getDescriptor().isMultisampledTexture())
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
                             bufferPtr outData) {
    if (_resolveBuffer) {
        resolve(-1, true, false, false);
        _resolveBuffer->readData(rect, imageFormat, dataType, outData);
    } else {
        GL_API::getStateTracker().setPixelPackUnpackAlignment();
        GL_API::getStateTracker().setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_ONLY, _framebufferHandle);
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
    _attachmentResolvedLayers[binding].insert(state._writeLayer);
}

glFramebuffer::BindingState glFramebuffer::getAttachmentState(GLenum binding) const {
    hashMap<GLenum, BindingState>::const_iterator it = _attachmentState.find(binding);
    if (it != std::cend(_attachmentState)) {
        return it->second;
    }

    return { AttachmentState::COUNT, 0, 0 };
}

void glFramebuffer::queueCheckStatus() {
    _statusCheckQueued = true;
}

bool glFramebuffer::checkStatus() {
    if (!_statusCheckQueued) {
        return true;
    }

    _statusCheckQueued = false;

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
