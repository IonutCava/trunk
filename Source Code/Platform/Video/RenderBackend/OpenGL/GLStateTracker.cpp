#include "stdafx.h"

#include "Headers/GLStateTracker.h"

#include "Headers/glLockManager.h"
#include "Headers/GLWrapper.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {
    
namespace {

    inline GLint getBufferTargetIndex(GLenum target) {
        // Select the appropriate index in the array based on the buffer target
        switch (target) {
            case GL_TEXTURE_BUFFER: return 0;
            case GL_UNIFORM_BUFFER: return 1;
            case GL_SHADER_STORAGE_BUFFER: return 2;
            case GL_PIXEL_UNPACK_BUFFER: return 3;
            case GL_DRAW_INDIRECT_BUFFER: return 4;
            case GL_ARRAY_BUFFER: return 5;
            //case GL_ELEMENT_ARRAY_BUFFER: return -1;
            default: break;
        };

        DIVIDE_UNEXPECTED_CALL();
        return -1;
    }
}; //namespace 

void GLStateTracker::init() noexcept 
{
    _opengl46Supported = GLUtil::getGLValue<GLint>(GL_MINOR_VERSION) == 6;
    _vaoBufferData.init(GL_API::s_maxAttribBindings);
    _samplerBoundMap.fill(0u);
    for (std::array<U32, to_base(TextureType::COUNT)>& it : _textureBoundMap) {
        it.fill(0u);
    }

    _blendProperties.resize(GL_API::s_maxFBOAttachments, BlendingProperties());
    _blendEnabled.resize(GL_API::s_maxFBOAttachments, GL_FALSE);
}

void GLStateTracker::setStateBlock(size_t stateBlockHash) {
    if (stateBlockHash == 0) {
        stateBlockHash = RenderStateBlock::defaultHash();
    }

    // If the new state hash is different from the previous one
    if (stateBlockHash != _activeState.getHash()) {
        bool currentStateValid = false;
        const RenderStateBlock& currentState = RenderStateBlock::get(stateBlockHash, currentStateValid);

        DIVIDE_ASSERT(currentStateValid, "GL_API error: Invalid state blocks detected on activation!");

        // Activate the new render state block in an rendering API dependent way
        activateStateBlock(currentState);
    }
}

/// Pixel pack alignment is usually changed by textures, PBOs, etc
bool GLStateTracker::setPixelPackAlignment(GLint packAlignment,
                                           GLint rowLength,
                                           GLint skipRows,
                                           GLint skipPixels) {
    // Keep track if we actually affect any OpenGL state
    bool changed = false;
    if (_activePackUnpackAlignments[0] != packAlignment) {
        glPixelStorei(GL_PACK_ALIGNMENT, packAlignment);
        _activePackUnpackAlignments[0] = packAlignment;
        changed = true;
    }

    if (_activePackUnpackRowLength[0] != rowLength) {
        glPixelStorei(GL_PACK_ROW_LENGTH, rowLength);
        _activePackUnpackRowLength[0] = rowLength;
        changed = true;
    }

    if (_activePackUnpackSkipRows[0] != skipRows) {
        glPixelStorei(GL_PACK_SKIP_ROWS, skipRows);
        _activePackUnpackSkipRows[0] = skipRows;
        changed = true;
    }

    if (_activePackUnpackSkipPixels[0] != skipPixels) {
        glPixelStorei(GL_PACK_SKIP_PIXELS, skipPixels);
        _activePackUnpackSkipPixels[0] = skipPixels;
        changed = true;
    }

    // We managed to change at least one entry
    return changed;
}

/// Pixel unpack alignment is usually changed by textures, PBOs, etc
bool GLStateTracker::setPixelUnpackAlignment(GLint unpackAlignment,
                                             GLint rowLength,
                                             GLint skipRows,
                                             GLint skipPixels) {
    // Keep track if we actually affect any OpenGL state
    bool changed = false;
    if (_activePackUnpackAlignments[1] != unpackAlignment) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
        _activePackUnpackAlignments[1] = unpackAlignment;
        changed = true;
    }

    if (rowLength != -1 && _activePackUnpackRowLength[1] != rowLength) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
        _activePackUnpackRowLength[1] = rowLength;
        changed = true;
    }

    if (skipRows != -1 && _activePackUnpackSkipRows[1] != skipRows) {
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
        _activePackUnpackSkipRows[1] = skipRows;
        changed = true;
    }

    if (skipPixels != -1 && _activePackUnpackSkipPixels[1] != skipPixels) {
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
        _activePackUnpackSkipPixels[1] = skipPixels;
        changed = true;
    }

    // We managed to change at least one entry
    return changed;
}

/// Enable or disable primitive restart and ensure that the correct index size
/// is used
void GLStateTracker::togglePrimitiveRestart(bool state) {
    // Toggle primitive restart on or off
    if (_primitiveRestartEnabled != state) {
        _primitiveRestartEnabled = state;
        state ? glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX)
              : glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    }
}

/// Enable or disable primitive rasterization
void GLStateTracker::toggleRasterization(bool state) {
    // Toggle primitive restart on or off
    if (_rasterizationEnabled != state) {
        _rasterizationEnabled = state;
        state ? glDisable(GL_RASTERIZER_DISCARD)
              : glEnable(GL_RASTERIZER_DISCARD);
    }
}

bool GLStateTracker::bindSamplers(GLushort unitOffset,
                                  GLuint samplerCount,
                                  GLuint* samplerHandles) {
    if (samplerCount > 0 &&
        unitOffset + samplerCount < static_cast<GLuint>(GL_API::s_maxTextureUnits))
    {
        if (samplerCount == 1) {
            GLuint& handle = _samplerBoundMap[unitOffset];
            const GLuint targetHandle = samplerHandles ? samplerHandles[0] : 0u;
            if (handle != targetHandle) {
                glBindSampler(unitOffset, targetHandle);
                handle = targetHandle;
                return true;
            }
        } else {
            glBindSamplers(unitOffset, samplerCount, samplerHandles);
            if (samplerHandles != nullptr) {
                memcpy(&_samplerBoundMap[unitOffset], samplerHandles, sizeof(GLuint) * samplerCount);
            } else {
                memset(_samplerBoundMap.data(), 0u, sizeof(GLuint) * samplerCount);
            }
            return true;
        } 
    }

    return false;
}

bool GLStateTracker::bindTextures(const GLushort unitOffset,
                                  const GLuint textureCount,
                                  TextureType* textureTypes,
                                  GLuint* textureHandles,
                                  GLuint* samplerHandles) {

    bool bound = false;
    if (textureCount > 0 &&
        unitOffset + textureCount < static_cast<GLuint>(GL_API::s_maxTextureUnits))
    {
        if (textureHandles != nullptr) {
            UniqueLock<SharedMutex> w_lock(GL_API::s_mipmapQueueSetLock);
            for (GLuint i = 0; i < textureCount; ++i) {
                const GLuint crtHandle = textureHandles[i];
                if (crtHandle > 0) {
                    const auto it = GL_API::s_mipmapQueue.find(crtHandle);
                    if (it == std::cend(GL_API::s_mipmapQueue)) {
                        continue;
                    }
                    glGenerateTextureMipmap(crtHandle);
                    GL_API::s_mipmapQueue.erase(crtHandle);
                }
            }
        }

        bindSamplers(unitOffset, textureCount, samplerHandles);

        if (textureCount == 1) {
            const GLuint targetHandle = textureHandles ? textureHandles[0] : 0u;
            const TextureType type = textureTypes ? textureTypes[0] : TextureType::TEXTURE_2D;

            GLuint& crtHandle = _textureBoundMap[unitOffset][to_base(type)];

            if (crtHandle != targetHandle) {
                glBindTextureUnit(unitOffset, targetHandle);
                crtHandle = targetHandle;
                bound = true;
            }
        } else {
            const TextureType type = textureTypes ? textureTypes[0] : TextureType::COUNT;

            glBindTextures(unitOffset, textureCount, textureHandles);
            if (textureHandles != nullptr) {
                assert(type != TextureType::COUNT);
                memcpy(_textureBoundMap[unitOffset].data(), textureHandles, sizeof(GLuint) * textureCount);
            } else {
                for (auto& map : _textureBoundMap) {
                    memset(map.data(), 0u, sizeof(GLuint) * textureCount);
                }
            }

            bound = true;
        }
    }

    return bound;
}

/// Bind a texture specified by a GL handle and GL type to the specified unit
/// using the sampler object defined by hash value
bool GLStateTracker::bindTexture(GLushort unit,
                                 TextureType type,
                                 GLuint handle,
                                 GLuint samplerHandle) {
    // Fail if we specified an invalid unit. Assert instead of returning false
    // because this might be related to a bad algorithm
    DIVIDE_ASSERT(unit < static_cast<GLuint>(GL_API::s_maxTextureUnits),
                  "GLStates error: invalid texture unit specified as a texture binding slot!");
    return bindTextures(unit, 1, &type, &handle, &samplerHandle);
}

bool GLStateTracker::bindTextureImage(GLushort unit, TextureType type, GLuint handle, GLint level,
                              bool layered, GLint layer, GLenum access,
                              GLenum format) {
    static ImageBindSettings tempSettings;
    tempSettings = {handle, level, layered ? GL_TRUE : GL_FALSE, layer, access, format};

    ImageBindSettings& settings = _imageBoundMap[unit];
    if (settings != tempSettings) {
        glBindImageTexture(unit, handle, level, layered ? GL_TRUE : GL_FALSE, layer, access, format);
        settings = tempSettings;
        return true;
    }

    return false;
}

/// Single place to change buffer objects for every target available
bool GLStateTracker::bindActiveBuffer(GLuint vaoID, GLuint location, GLuint bufferID, GLuint instanceDivisor, size_t offset, size_t stride) {
    if (_vaoBufferData.instanceDivisor(vaoID, location) != instanceDivisor) {
        glVertexArrayBindingDivisor(vaoID, location, instanceDivisor);
        _vaoBufferData.instanceDivisor(vaoID, location, instanceDivisor);
    }

    const VAOBindings::BufferBindingParams& bindings = _vaoBufferData.bindingParams(vaoID, location);
    const VAOBindings::BufferBindingParams currentParams(bufferID, offset, stride);

    if (bindings != currentParams) {
        // Bind the specified buffer handle to the desired buffer target
        glVertexArrayVertexBuffer(vaoID, location, bufferID, (GLintptr)offset, (GLsizei)stride);
        // Remember the new binding for future reference
        _vaoBufferData.bindingParams(vaoID, location, currentParams);
        return true;
    }

    return false;
}

bool GLStateTracker::setActiveFB(RenderTarget::RenderTargetUsage usage, GLuint ID) {
    GLuint temp = 0;
    return setActiveFB(usage, ID, temp);
}

/// Switch the current framebuffer by binding it as either a R/W buffer, read
/// buffer or write buffer
bool GLStateTracker::setActiveFB(RenderTarget::RenderTargetUsage usage, GLuint ID, GLuint& previousID) {
    // We may query the active framebuffer handle and get an invalid handle in
    // return and then try to bind the queried handle
    // This is, for example, in save/restore FB scenarios. An invalid handle
    // will just reset the buffer binding
    if (ID == GLUtil::k_invalidObjectID) {
        ID = 0;
    }
    previousID = _activeFBID[to_U32(usage)];
    // Prevent double bind
    if (_activeFBID[to_U32(usage)] == ID) {
        if (usage == RenderTarget::RenderTargetUsage::RT_READ_WRITE) {
            if (_activeFBID[to_base(RenderTarget::RenderTargetUsage::RT_READ_ONLY)] == ID &&
                _activeFBID[to_base(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY)] == ID) {
                return false;
            }
        } else {
            return false;
        }
    }
    // Bind the requested buffer to the appropriate target
    switch (usage) {
        case RenderTarget::RenderTargetUsage::RT_READ_WRITE: {
            // According to documentation this is equivalent to independent
            // calls to
            // bindFramebuffer(read, ID) and bindFramebuffer(write, ID)
            glBindFramebuffer(GL_FRAMEBUFFER, ID);
            // This also overrides the read and write bindings
            _activeFBID[to_base(RenderTarget::RenderTargetUsage::RT_READ_ONLY)] = ID;
            _activeFBID[to_base(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY)] = ID;
        } break;
        case RenderTarget::RenderTargetUsage::RT_READ_ONLY: {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, ID);
        } break;
        case RenderTarget::RenderTargetUsage::RT_WRITE_ONLY: {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ID);
        } break;
        default: DIVIDE_UNEXPECTED_CALL(); break;
    };

    // Remember the new binding state for future reference
    _activeFBID[to_U32(usage)] = ID;

    return true;
}

bool GLStateTracker::setActiveVAO(const GLuint ID) noexcept {
    GLuint temp = 0;
    return setActiveVAO(ID, temp);
}

/// Switch the currently active vertex array object
bool GLStateTracker::setActiveVAO(const GLuint ID, GLuint& previousID) noexcept {
    previousID = _activeVAOID;
    // Prevent double bind
    if (_activeVAOID != ID) {
        // Remember the new binding for future reference
        _activeVAOID = ID;
        //setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        // Activate the specified VAO
        glBindVertexArray(ID);
        return true;
    }

    return false;
}


/// Single place to change buffer objects for every target available
bool GLStateTracker::setActiveBuffer(GLenum target, GLuint ID, GLuint& previousID) {
    GLuint& crtBinding = target != GL_ELEMENT_ARRAY_BUFFER 
                                 ? _activeBufferID[getBufferTargetIndex(target)]
                                 : _activeVAOIB[_activeVAOID];
    previousID = crtBinding;

    // Prevent double bind (hope that this is the most common case. Should be.)
    if (previousID == ID) {
        return false;
    }

    // Remember the new binding for future reference
    crtBinding = ID;
    // Bind the specified buffer handle to the desired buffer target
    glBindBuffer(target, ID);
    return true;
}

bool GLStateTracker::setActiveBuffer(GLenum target, GLuint ID) {
    GLuint temp = 0u;
    return setActiveBuffer(target, ID, temp);
}

/// Change the currently active shader program. Passing null will unbind shaders (will use program 0)
bool GLStateTracker::setActiveProgram(GLuint programHandle) {
    // Check if we are binding a new program or unbinding all shaders
    // Prevent double bind
    if (_activeShaderProgram != programHandle) {
        setActiveShaderPipeline(0u);

        // Remember the new binding for future reference
        _activeShaderProgram = programHandle;
        // Bind the new program
        glUseProgram(programHandle);
        return true;
    }

    return false;
}

/// Change the currently active shader pipeline. Passing null will unbind shaders (will use pipeline 0)
bool GLStateTracker::setActiveShaderPipeline(GLuint pipelineHandle) {
    // Check if we are binding a new program or unbinding all shaders
    // Prevent double bind
    if (_activeShaderPipeline != pipelineHandle) {
        setActiveProgram(0u);

        // Remember the new binding for future reference
        _activeShaderPipeline = pipelineHandle;
        // Bind the new pipeline
        glBindProgramPipeline(pipelineHandle);
        return true;
    }

    return false;
}

void GLStateTracker::setDepthRange(F32 nearVal, F32 farVal) {
    CLAMP_01(nearVal);
    CLAMP_01(farVal);
    if (!COMPARE(nearVal, _depthNearVal) && !COMPARE(farVal, _depthFarVal)) {
        glDepthRange(nearVal, farVal);
        _depthNearVal = nearVal;
        _depthFarVal = farVal;
    }
}

void GLStateTracker::setClippingPlaneState(const bool lowerLeftOrigin, const bool negativeOneToOneDepth) {
    if (lowerLeftOrigin != _lowerLeftOrigin || negativeOneToOneDepth != _negativeOneToOneDepth) {

        glClipControl(
            (lowerLeftOrigin ? GL_LOWER_LEFT : GL_UPPER_LEFT),
            (negativeOneToOneDepth ? GL_NEGATIVE_ONE_TO_ONE : GL_ZERO_TO_ONE)
        );

        _lowerLeftOrigin = lowerLeftOrigin;
        _negativeOneToOneDepth = negativeOneToOneDepth;
    }
}

void GLStateTracker::setBlendColour(const UColour4& blendColour) {
    if (_blendColour != blendColour) {
        _blendColour.set(blendColour);

        const FColour4 floatColour = Util::ToFloatColour(blendColour);
        glBlendColor(floatColour.r, floatColour.g, floatColour.b, floatColour.a);
    }
}

void GLStateTracker::setBlending(const BlendingProperties& blendingProperties) {
    OPTICK_EVENT();

    const bool enable = blendingProperties.blendEnabled();

    if ((_blendEnabledGlobal == GL_TRUE) != enable) {
        enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        _blendEnabledGlobal = (enable ? GL_TRUE : GL_FALSE);
        std::fill(std::begin(_blendEnabled), std::end(_blendEnabled), _blendEnabledGlobal);
    }

    if (enable && _blendPropertiesGlobal != blendingProperties) {
        if (blendingProperties._blendOpAlpha != BlendOperation::COUNT) {
            if (blendingProperties._blendSrc != _blendPropertiesGlobal._blendSrc ||
                blendingProperties._blendDest != _blendPropertiesGlobal._blendDest ||
                blendingProperties._blendSrcAlpha != _blendPropertiesGlobal._blendSrcAlpha ||
                blendingProperties._blendDestAlpha != _blendPropertiesGlobal._blendDestAlpha)
            {
                    glBlendFuncSeparate(GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                                        GLUtil::glBlendTable[to_base(blendingProperties._blendDest)],
                                        GLUtil::glBlendTable[to_base(blendingProperties._blendSrcAlpha)],
                                        GLUtil::glBlendTable[to_base(blendingProperties._blendDestAlpha)]);
            }
                
            if (blendingProperties._blendOp != _blendPropertiesGlobal._blendOp ||
                blendingProperties._blendOpAlpha != _blendPropertiesGlobal._blendOpAlpha)
            {
                    glBlendEquationSeparate(GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                                ? to_base(blendingProperties._blendOp)
                                                                                                : to_base(BlendOperation::ADD)],
                                            GLUtil::glBlendOpTable[to_base(blendingProperties._blendOpAlpha)]);
            }
        } else {
            if (blendingProperties._blendSrc != _blendPropertiesGlobal._blendSrc ||
                blendingProperties._blendDest != _blendPropertiesGlobal._blendDest)
            {
                    glBlendFunc(GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                                GLUtil::glBlendTable[to_base(blendingProperties._blendDest)]);
            }
            if (blendingProperties._blendOp != _blendPropertiesGlobal._blendOp)
            {
                glBlendEquation(GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                    ? to_base(blendingProperties._blendOp)
                                                                                    : to_base(BlendOperation::ADD)]);
            }

        }

        _blendPropertiesGlobal = blendingProperties;

        std::fill(std::begin(_blendProperties), std::end(_blendProperties), _blendPropertiesGlobal);
    }
}

void GLStateTracker::setBlending(const GLuint drawBufferIdx,const BlendingProperties& blendingProperties) {
    const bool enable = blendingProperties.blendEnabled();

    assert(drawBufferIdx < static_cast<GLuint>(GL_API::s_maxFBOAttachments));

    if ((_blendEnabled[drawBufferIdx] == GL_TRUE) != enable) {
        enable ? glEnablei(GL_BLEND, drawBufferIdx) : glDisablei(GL_BLEND, drawBufferIdx);
        _blendEnabled[drawBufferIdx] = (enable ? GL_TRUE : GL_FALSE);
        if (!enable) {
            _blendEnabledGlobal = GL_FALSE;
        }
    }

    BlendingProperties& crtProperties = _blendProperties[drawBufferIdx];

    if (enable && crtProperties != blendingProperties) {
        if (blendingProperties._blendOpAlpha != BlendOperation::COUNT) {
            if (blendingProperties._blendSrc != crtProperties._blendSrc ||
                blendingProperties._blendDest != crtProperties._blendDest ||
                blendingProperties._blendSrcAlpha != crtProperties._blendSrcAlpha ||
                blendingProperties._blendDestAlpha != crtProperties._blendDestAlpha)
            {
                    glBlendFuncSeparatei(drawBufferIdx,
                                         GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                                         GLUtil::glBlendTable[to_base(blendingProperties._blendDest)],
                                         GLUtil::glBlendTable[to_base(blendingProperties._blendSrcAlpha)],
                                         GLUtil::glBlendTable[to_base(blendingProperties._blendDestAlpha)]);

                    _blendPropertiesGlobal._blendSrc = blendingProperties._blendSrc;
                    _blendPropertiesGlobal._blendDest = blendingProperties._blendDest;
                    _blendPropertiesGlobal._blendSrcAlpha = blendingProperties._blendSrcAlpha;
                    _blendPropertiesGlobal._blendDestAlpha = blendingProperties._blendDestAlpha;
            }

            if (blendingProperties._blendOp != crtProperties._blendOp ||
                blendingProperties._blendOpAlpha != crtProperties._blendOpAlpha)
            {
                glBlendEquationSeparatei(drawBufferIdx, 
                                            GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                                ? to_base(blendingProperties._blendOp)
                                                                                                : to_base(BlendOperation::ADD)],
                                            GLUtil::glBlendOpTable[to_base(blendingProperties._blendOpAlpha)]);

                _blendPropertiesGlobal._blendOp = blendingProperties._blendOp;
                _blendPropertiesGlobal._blendOpAlpha = blendingProperties._blendOpAlpha;
            }
        } else {
            if (blendingProperties._blendSrc != crtProperties._blendSrc ||
                blendingProperties._blendDest != crtProperties._blendDest)
            {
                    glBlendFunci(drawBufferIdx,
                                    GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                                    GLUtil::glBlendTable[to_base(blendingProperties._blendDest)]);

                    _blendPropertiesGlobal._blendSrc = blendingProperties._blendSrc;
                    _blendPropertiesGlobal._blendDest = blendingProperties._blendDest;
            }

            if (blendingProperties._blendOp != crtProperties._blendOp)
            {
                glBlendEquationi(drawBufferIdx,
                                    GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                        ? to_base(blendingProperties._blendOp)
                                                                                        : to_base(BlendOperation::ADD)]);
                _blendPropertiesGlobal._blendOp = blendingProperties._blendOp;
            }
        }

        crtProperties = blendingProperties;
    }
}

/// Change the current viewport area. Redundancy check is performed in GFXDevice class
bool GLStateTracker::setViewport(const Rect<I32>& viewport) {
    if (viewport.z > 0 && viewport.w > 0 && viewport != _activeViewport) {
        glViewport(viewport.x, viewport.y, viewport.z, viewport.w);
        _activeViewport.set(viewport);
        return true;
    }

    return false;
}

bool GLStateTracker::setClearColour(const FColour4& colour) {
    if (colour != _activeClearColour) {
        glClearColor(colour.r, colour.g, colour.b, colour.a);
        _activeClearColour.set(colour);
        return true;
    }

    return false;
}

bool GLStateTracker::setScissor(const Rect<I32>& rect) {
    if (rect != _activeScissor) {
        glScissor(rect.x, rect.y, rect.z, rect.w);
        _activeScissor.set(rect);
        return true;
    }

    return false;
}

U32 GLStateTracker::getBoundTextureHandle(const U8 slot, const TextureType type) {
    return _textureBoundMap[slot][to_base(type)];
}

void GLStateTracker::getActiveViewport(GLint* vp) {
    if (vp != nullptr) {
        *vp = *_activeViewport._v;
    }
}

/// A state block should contain all rendering state changes needed for the next draw call.
/// Some may be redundant, so we check each one individually
void GLStateTracker::activateStateBlock(const RenderStateBlock& newBlock) {

    if (_activeState.cullEnabled() != newBlock.cullEnabled()) {
        newBlock.cullEnabled() ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    }

    if (_activeState.stencilEnable() != newBlock.stencilEnable()) {
        newBlock.stencilEnable() ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
    }

    if (_activeState.depthTestEnabled() != newBlock.depthTestEnabled()) {
        newBlock.depthTestEnabled() ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    }

    if (_activeState.scissorTestEnabled() != newBlock.scissorTestEnabled()) {
        newBlock.scissorTestEnabled() ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
    }

    // Check culling mode (back (CW) / front (CCW) by default)
    if (_activeState.cullMode() != newBlock.cullMode()) {
        if (newBlock.cullMode() != CullMode::NONE) {
            const GLenum targetMode = GLUtil::glCullModeTable[to_U32(newBlock.cullMode())];
            if (_currentCullMode != targetMode) {
                glCullFace(targetMode);
                _currentCullMode = targetMode;
            }
        }
    }

    if (_activeState.frontFaceCCW() != newBlock.frontFaceCCW()) {
        _currentFrontFace = newBlock.frontFaceCCW() ? GL_CCW : GL_CW;
        glFrontFace(_currentFrontFace);
    }

    // Check rasterization mode
    if (_activeState.fillMode() != newBlock.fillMode()) {
        glPolygonMode(GL_FRONT_AND_BACK, GLUtil::glFillModeTable[to_U32(newBlock.fillMode())]);
    }

    if (_activeState.tessControlPoints() != newBlock.tessControlPoints()) {
        glPatchParameteri(GL_PATCH_VERTICES, newBlock.tessControlPoints());
    }

    // Check the depth function
    if (_activeState.zFunc() != newBlock.zFunc()) {
        glDepthFunc(GLUtil::glCompareFuncTable[to_U32(newBlock.zFunc())]);
    }

    // Check if we need to change the stencil mask
    if (_activeState.stencilWriteMask() != newBlock.stencilWriteMask()) {
        glStencilMask(newBlock.stencilWriteMask());
    }
    // Stencil function is dependent on 3 state parameters set together
    if (_activeState.stencilFunc() != newBlock.stencilFunc() ||
        _activeState.stencilRef()  != newBlock.stencilRef() ||
        _activeState.stencilMask() != newBlock.stencilMask()) {
        glStencilFunc(GLUtil::glCompareFuncTable[to_U32(newBlock.stencilFunc())],
                      newBlock.stencilRef(),
                      newBlock.stencilMask());
    }
    // Stencil operation is also dependent  on 3 state parameters set together
    if (_activeState.stencilFailOp()  != newBlock.stencilFailOp() ||
        _activeState.stencilZFailOp() != newBlock.stencilZFailOp() ||
        _activeState.stencilPassOp()  != newBlock.stencilPassOp()) {
        glStencilOp(GLUtil::glStencilOpTable[to_U32(newBlock.stencilFailOp())],
                    GLUtil::glStencilOpTable[to_U32(newBlock.stencilZFailOp())],
                    GLUtil::glStencilOpTable[to_U32(newBlock.stencilPassOp())]);
    }
    // Check and set polygon offset
    if (!COMPARE(_activeState.zBias(), newBlock.zBias())) {
        if (IS_ZERO(newBlock.zBias())) {
            glDisable(GL_POLYGON_OFFSET_FILL);
        } else {
            glEnable(GL_POLYGON_OFFSET_FILL);
            if (!COMPARE(_activeState.zBias(), newBlock.zBias()) || !COMPARE(_activeState.zUnits(), newBlock.zUnits())) {
                glPolygonOffset(newBlock.zBias(), newBlock.zUnits());
            }
        }
    }

    // Check and set colour mask
    if (_activeState.colourWrite().i != newBlock.colourWrite().i) {
        const P32 cWrite = newBlock.colourWrite();
        glColorMask(cWrite.b[0] == 1 ? GL_TRUE : GL_FALSE,   // R
                    cWrite.b[1] == 1 ? GL_TRUE : GL_FALSE,   // G
                    cWrite.b[2] == 1 ? GL_TRUE : GL_FALSE,   // B
                    cWrite.b[3] == 1 ? GL_TRUE : GL_FALSE);  // A
    }

    _activeState.from(newBlock);
}

}; //namespace Divide