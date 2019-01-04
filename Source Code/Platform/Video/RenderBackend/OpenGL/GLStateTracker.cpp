#include "stdafx.h"

#include "Headers/GLStateTracker.h"

#include "Headers/GLWrapper.h"
#include "Headers/glLockManager.h"

#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {
    
namespace {

    GLint getBufferTargetIndex(GLenum target) {
        GLint index = -1;
        // Select the appropriate index in the array based on the buffer target
        switch (target) {
            case GL_TEXTURE_BUFFER: {
                index = 0;
            }break;
            case GL_UNIFORM_BUFFER: {
                index = 1;
            }break;
            case GL_SHADER_STORAGE_BUFFER: {
                index = 2;
            }break;
            case GL_PIXEL_UNPACK_BUFFER: {
                index = 3;
            }break;
            case GL_DRAW_INDIRECT_BUFFER: {
                index = 4;
            }break;
            case GL_ARRAY_BUFFER: {
                index = 5;
            }break;
            default:
            case GL_ELEMENT_ARRAY_BUFFER: {
                // Make sure the target is available. Assert if it isn't as this
                // means that a non-supported feature is used somewhere
                DIVIDE_ASSERT(IS_IN_RANGE_INCLUSIVE(index, 0, 5),
                              "GLStates error: attempted to bind an invalid buffer target!");
                return -1;
            }
        };
        return index;
    }
}; //namespace 

void GLStateTracker::init(GLStateTracker* base) {
    if (_init) {
        return;
    } 
    if (base == nullptr) {
        _opengl46Supported = GLUtil::getIntegerv(GL_MINOR_VERSION) == 6;
        _vaoBufferData.init(GL_API::s_maxAttribBindings);
        _samplerBoundMap.fill(0u);
        for (std::array<U32, to_base(TextureType::COUNT)>& it : _textureBoundMap) {
            it.fill(0u);
        }

        _blendProperties.resize(GL_API::s_maxFBOAttachments, BlendingProperties());
        _blendEnabled.resize(GL_API::s_maxFBOAttachments, GL_FALSE);
      
    } else {
        *this = *base;
    }
    _currentCullMode = GL_BACK;
    _patchVertexCount = GLUtil::getIntegerv(GL_PATCH_VERTICES);
    _init = true;
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

size_t GLStateTracker::setStateBlock(size_t stateBlockHash) {
    // If the new state hash is different from the previous one
    if (stateBlockHash != _currentStateBlockHash) {
        // Remember the previous state hash
        _previousStateBlockHash = _currentStateBlockHash;
        // Update the current state hash
        _currentStateBlockHash = stateBlockHash;

        bool currentStateValid = false;
        const RenderStateBlock& currentState = RenderStateBlock::get(_currentStateBlockHash, currentStateValid);
        if (_previousStateBlockHash != 0) {
            bool previousStateValid = false;
            const RenderStateBlock& previousState = RenderStateBlock::get(_previousStateBlockHash, previousStateValid);

            DIVIDE_ASSERT(currentStateValid && previousStateValid &&
                          currentState != previousState,
                          "GL_API error: Invalid state blocks detected on activation!");

            // Activate the new render state block in an rendering API dependent way
            activateStateBlock(currentState, previousState);
        } else {
            DIVIDE_ASSERT(currentStateValid, "GL_API error: Invalid state blocks detected on activation!");
            activateStateBlock(currentState);
        }
    }

    // Return the previous state hash
    return _previousStateBlockHash;
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

void GLStateTracker::setPatchVertexCount(U32 count) {
    if (_patchVertexCount != count) {
        _patchVertexCount = count;
        glPatchParameteri(GL_PATCH_VERTICES, _patchVertexCount);
        
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
            GLuint targetHandle = samplerHandles ? samplerHandles[0] : 0u;
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
                memset(&_samplerBoundMap[unitOffset], 0, sizeof(GLuint) * samplerCount);
            }
            return true;
        } 
    }

    return false;
}

bool GLStateTracker::bindTextures(GLushort unitOffset,
                                  GLuint textureCount,
                                  TextureType* textureTypes,
                                  GLuint* textureHandles,
                                  GLuint* samplerHandles) {

    bool bound = false;
    if (textureCount > 0 &&
        unitOffset + textureCount < static_cast<GLuint>(GL_API::s_maxTextureUnits))
    {
        if (textureHandles != nullptr) {

            bool mipMapsQueued = false;
            {
                SharedLock r_lock(GL_API::s_mipmapQueueSetLock);
                mipMapsQueued = !GL_API::s_mipmapQueueSync.empty();
            }

            if (mipMapsQueued) {
                for (GLuint i = 0; i < textureCount; ++i) {
                    GLuint crtHandle = textureHandles[i];
                    if (crtHandle > 0) {
                        GLsync entry = nullptr;
                        {
                            UniqueLockShared w_lock(GL_API::s_mipmapQueueSetLock);
                            auto it = GL_API::s_mipmapQueueSync.find(crtHandle);
                            if (it == std::cend(GL_API::s_mipmapQueueSync) || it->second == nullptr) {
                                continue;
                            }
                            entry = it->second;
                            GL_API::s_mipmapQueueSync.erase(it);
                        }

                        // entry shouldn't be null by this point;
                        assert(entry != nullptr);
                        // do this here so that we don't hold the mutex lock for too long
                        glLockManager::wait(&entry, false /*this should work, right?*/);
                        glDeleteSync(entry);
                    }
                }
            }
        }

        bindSamplers(unitOffset, textureCount, samplerHandles);

        if (textureCount == 1) {
            GLuint targetHandle = textureHandles ? textureHandles[0] : 0u;
            TextureType type = textureTypes ? textureTypes[0] : TextureType::TEXTURE_2D;

            GLuint& crtHandle = _textureBoundMap[unitOffset][to_base(type)];

            if (crtHandle != targetHandle) {
                glBindTextureUnit(unitOffset, targetHandle);
                crtHandle = targetHandle;
                bound = true;
            }
        } else {
            glBindTextures(unitOffset, textureCount, textureHandles);
            if (textureHandles != nullptr) {
                memcpy(&_textureBoundMap[unitOffset], textureHandles, sizeof(GLuint) * textureCount);
            } else {
                memset(&_textureBoundMap[unitOffset], 0, sizeof(GLuint) * textureCount);
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
bool GLStateTracker::bindActiveBuffer(GLuint vaoID, GLuint location, GLuint bufferID, GLintptr offset, GLsizei stride) {
    const VAOBindings::BufferBindingParams& bindings = _vaoBufferData.bindingParams(vaoID, location);

    VAOBindings::BufferBindingParams currentParams(bufferID, offset, stride);
    if (bindings != currentParams) {
        // Bind the specified buffer handle to the desired buffer target
        glVertexArrayVertexBuffer(vaoID, location, bufferID, offset, stride);
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
    if (ID == GLUtil::_invalidObjectID) {
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
    };

    // Remember the new binding state for future reference
    _activeFBID[to_U32(usage)] = ID;

    return true;
}

bool GLStateTracker::setActiveVAO(GLuint ID) {
    GLuint temp = 0;
    return setActiveVAO(ID, temp);
}

/// Switch the currently active vertex array object
bool GLStateTracker::setActiveVAO(GLuint ID, GLuint& previousID) {
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
    GLuint& crtBinding = target == GL_ELEMENT_ARRAY_BUFFER 
                                 ? _activeVAOIB[_activeVAOID]
                                 : _activeBufferID[getBufferTargetIndex(target)];
    previousID = crtBinding;

    // Prevent double bind
    if (previousID != ID) {
        // Remember the new binding for future reference
        crtBinding = ID;
        // Bind the specified buffer handle to the desired buffer target
        glBindBuffer(target, ID);
        return true;
    }

    return false;
}

bool GLStateTracker::setActiveBuffer(GLenum target, GLuint ID) {
    GLuint temp = 0;
    return setActiveBuffer(target, ID, temp);
}

/// Change the currently active shader program. Passing null will unbind shaders
/// (will use program 0)
bool GLStateTracker::setActiveProgram(GLuint programHandle) {
    // Check if we are binding a new program or unbinding all shaders
    // Prevent double bind
    if (_activeShaderProgram != programHandle) {
        // Remember the new binding for future reference
        _activeShaderProgram = programHandle;
        // Bind the new program
        glUseProgram(programHandle);
        return true;
    }

    return false;
}

void GLStateTracker::setDepthRange(F32 nearVal, F32 farVal) {
    CLAMP(nearVal, 0.0f, 1.0f);
    CLAMP(farVal, 0.0f, 1.0f);
    if (!COMPARE(nearVal, _depthNearVal) && !COMPARE(farVal, _depthFarVal)) {
        glDepthRange(nearVal, farVal);
        _depthNearVal = nearVal;
        _depthFarVal = farVal;
    }
}

void GLStateTracker::setBlendColour(const UColour& blendColour, bool force) {
    if (_blendColour != blendColour || force) {
        FColour floatColour = Util::ToFloatColour(blendColour);
        glBlendColor(static_cast<GLfloat>(floatColour.r),
                     static_cast<GLfloat>(floatColour.g),
                     static_cast<GLfloat>(floatColour.b),
                     static_cast<GLfloat>(floatColour.a));

        _blendColour.set(blendColour);
    }
}

void GLStateTracker::setBlending(const BlendingProperties& blendingProperties, bool force) {
    bool enable = blendingProperties.blendEnabled();

    if ((_blendEnabledGlobal == GL_TRUE) != enable || force) {
        enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        _blendEnabledGlobal = (enable ? GL_TRUE : GL_FALSE);
        std::fill(std::begin(_blendEnabled), std::end(_blendEnabled), _blendEnabledGlobal);
    }

    if (enable || force) {
        if (_blendPropertiesGlobal != blendingProperties || force)
        {
            if (blendingProperties._blendOpAlpha != BlendOperation::COUNT) {
                if (blendingProperties._blendSrc != _blendPropertiesGlobal._blendSrc ||
                    blendingProperties._blendDest != _blendPropertiesGlobal._blendDest ||
                    blendingProperties._blendSrcAlpha != _blendPropertiesGlobal._blendSrcAlpha ||
                    blendingProperties._blendDestAlpha != _blendPropertiesGlobal._blendDestAlpha || force) {
                        glBlendFuncSeparate(GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                                            GLUtil::glBlendTable[to_base(blendingProperties._blendDest)],
                                            GLUtil::glBlendTable[to_base(blendingProperties._blendSrcAlpha)],
                                            GLUtil::glBlendTable[to_base(blendingProperties._blendDestAlpha)]);
                }
                
                if (blendingProperties._blendOp != _blendPropertiesGlobal._blendOp ||
                    blendingProperties._blendOpAlpha != _blendPropertiesGlobal._blendOpAlpha || force) {
                        glBlendEquationSeparate(GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                                    ? to_base(blendingProperties._blendOp)
                                                                                                    : to_base(BlendOperation::ADD)],
                                                GLUtil::glBlendOpTable[to_base(blendingProperties._blendOpAlpha)]);
                }
            } else {
                if (blendingProperties._blendSrc != _blendPropertiesGlobal._blendSrc ||
                    blendingProperties._blendDest != _blendPropertiesGlobal._blendDest || force) {
                        glBlendFunc(GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                                    GLUtil::glBlendTable[to_base(blendingProperties._blendDest)]);
                }
                if (blendingProperties._blendOp != _blendPropertiesGlobal._blendOp || force) {
                    glBlendEquation(GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                        ? to_base(blendingProperties._blendOp)
                                                                                        : to_base(BlendOperation::ADD)]);
                }

            }

            _blendPropertiesGlobal = blendingProperties;

            std::fill(std::begin(_blendProperties), std::end(_blendProperties), _blendPropertiesGlobal);
        }
    }
}

void GLStateTracker::setBlending(GLuint drawBufferIdx,const BlendingProperties& blendingProperties, bool force) {
    bool enable = blendingProperties.blendEnabled();

    assert(drawBufferIdx < (GLuint)(GL_API::s_maxFBOAttachments));

    if ((_blendEnabled[drawBufferIdx] == GL_TRUE) != enable || force) {
        enable ? glEnablei(GL_BLEND, drawBufferIdx) : glDisablei(GL_BLEND, drawBufferIdx);
        _blendEnabled[drawBufferIdx] = (enable ? GL_TRUE : GL_FALSE);
        if (!enable) {
            _blendEnabledGlobal = GL_FALSE;
        }
    }

    if (enable || force) {
        if (_blendProperties[drawBufferIdx] != blendingProperties || force) {
            if (blendingProperties._blendOpAlpha != BlendOperation::COUNT) {
                if (blendingProperties._blendSrc != _blendProperties[drawBufferIdx]._blendSrc ||
                    blendingProperties._blendDest != _blendProperties[drawBufferIdx]._blendDest ||
                    blendingProperties._blendSrcAlpha != _blendProperties[drawBufferIdx]._blendSrcAlpha ||
                    blendingProperties._blendDestAlpha != _blendProperties[drawBufferIdx]._blendDestAlpha || force) {
                        glBlendFuncSeparatei(drawBufferIdx,
                                             GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                                             GLUtil::glBlendTable[to_base(blendingProperties._blendDest)],
                                             GLUtil::glBlendTable[to_base(blendingProperties._blendSrcAlpha)],
                                             GLUtil::glBlendTable[to_base(blendingProperties._blendDestAlpha)]);
                }

                if (blendingProperties._blendOp != _blendProperties[drawBufferIdx]._blendOp ||
                    blendingProperties._blendOpAlpha != _blendProperties[drawBufferIdx]._blendOpAlpha || force) {
                    glBlendEquationSeparatei(drawBufferIdx, 
                                             GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                                 ? to_base(blendingProperties._blendOp)
                                                                                                 : to_base(BlendOperation::ADD)],
                                             GLUtil::glBlendOpTable[to_base(blendingProperties._blendOpAlpha)]);
                }
            } else {
                if (blendingProperties._blendSrc != _blendProperties[drawBufferIdx]._blendSrc ||
                    blendingProperties._blendDest != _blendProperties[drawBufferIdx]._blendDest || force) {
                        glBlendFunci(drawBufferIdx,
                                     GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                                     GLUtil::glBlendTable[to_base(blendingProperties._blendDest)]);
                }

                if (blendingProperties._blendOp != _blendProperties[drawBufferIdx]._blendOp || force) {
                    glBlendEquationi(drawBufferIdx,
                                     GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                         ? to_base(blendingProperties._blendOp)
                                                                                         : to_base(BlendOperation::ADD)]);
                }
            }

            _blendProperties[drawBufferIdx] = blendingProperties;
            _blendPropertiesGlobal.reset();
        }
    }
}

/// Change the current viewport area. Redundancy check is performed in GFXDevice class
bool GLStateTracker::setViewport(I32 x, I32 y, I32 width, I32 height) {
    if (width > 0 && height > 0 && Rect<I32>(x, y, width, height) != _activeViewport) {
        // Debugging and profiling the application may require setting a 1x1 viewport to exclude fill rate bottlenecks
        if (Config::Profile::USE_1x1_VIEWPORT) {
            glViewport(x, y, 1, 1);
        } else {
            glViewport(x, y, width, height);
        }
        _activeViewport.set(x, y, width, height);
        
        return true;
    }

    return false;
}

bool GLStateTracker::setClearColour(const FColour& colour) {
    if (colour != _activeClearColour) {
        glClearColor(colour.r, colour.g, colour.b, colour.a);
        _activeClearColour.set(colour);
        return true;
    }

    return false;
}

bool GLStateTracker::setScissor(I32 x, I32 y, I32 width, I32 height) {
    if (Rect<I32>(x, y, width, height) != _activeScissor) {
        glScissor(x, y, width, height);
        _activeScissor.set(x, y, width, height);
        return true;
    }

    return false;
}

U32 GLStateTracker::getBoundTextureHandle(U8 slot, TextureType type) {
    return _textureBoundMap[slot][to_base(type)];
}

void GLStateTracker::getActiveViewport(GLint* vp) {
    if (vp != nullptr) {
        vp = (GLint*)_activeViewport._v;
    }
}

/// A state block should contain all rendering state changes needed for the next draw call.
/// Some may be redundant, so we check each one individually
void GLStateTracker::activateStateBlock(const RenderStateBlock& newBlock,
                                        const RenderStateBlock& oldBlock) {
    auto toggle = [](bool flag, GLenum state) {
        flag ? glEnable(state) : glDisable(state);
    };

  
    if (oldBlock.cullEnabled() != newBlock.cullEnabled()) {
        toggle(newBlock.cullEnabled(), GL_CULL_FACE);
    }
    if (oldBlock.stencilEnable() != newBlock.stencilEnable()) {
        toggle(newBlock.stencilEnable(), GL_STENCIL_TEST);
    }
    if (oldBlock.depthTestEnabled() != newBlock.depthTestEnabled()) {
        toggle(newBlock.depthTestEnabled(), GL_DEPTH_TEST);
    }
    if (oldBlock.scissorTestEnable() != newBlock.scissorTestEnable()) {
        toggle(newBlock.scissorTestEnable(), GL_SCISSOR_TEST);
    }
    // Check culling mode (back (CW) / front (CCW) by default)
    if (oldBlock.cullMode() != newBlock.cullMode()) {
        if (newBlock.cullMode() != CullMode::NONE) {
            GLenum targetMode = GLUtil::glCullModeTable[to_U32(newBlock.cullMode())];
            if (_currentCullMode != targetMode) {
                glCullFace(targetMode);
                _currentCullMode = targetMode;
            }
        }
    }
    // Check rasterization mode
    if (oldBlock.fillMode() != newBlock.fillMode()) {
        glPolygonMode(GL_FRONT_AND_BACK,
                      GLUtil::glFillModeTable[to_U32(newBlock.fillMode())]);
    }
    // Check the depth function
    if (oldBlock.zFunc() != newBlock.zFunc()) {
        glDepthFunc(GLUtil::glCompareFuncTable[to_U32(newBlock.zFunc())]);
    }

    // Check if we need to change the stencil mask
    if (oldBlock.stencilWriteMask() != newBlock.stencilWriteMask()) {
        glStencilMask(newBlock.stencilWriteMask());
    }
    // Stencil function is dependent on 3 state parameters set together
    if (oldBlock.stencilFunc() != newBlock.stencilFunc() ||
        oldBlock.stencilRef()  != newBlock.stencilRef() ||
        oldBlock.stencilMask() != newBlock.stencilMask()) {
        glStencilFunc(GLUtil::glCompareFuncTable[to_U32(newBlock.stencilFunc())],
                      newBlock.stencilRef(),
                      newBlock.stencilMask());
    }
    // Stencil operation is also dependent  on 3 state parameters set together
    if (oldBlock.stencilFailOp()  != newBlock.stencilFailOp() ||
        oldBlock.stencilZFailOp() != newBlock.stencilZFailOp() ||
        oldBlock.stencilPassOp()  != newBlock.stencilPassOp()) {
        glStencilOp(GLUtil::glStencilOpTable[to_U32(newBlock.stencilFailOp())],
                    GLUtil::glStencilOpTable[to_U32(newBlock.stencilZFailOp())],
                    GLUtil::glStencilOpTable[to_U32(newBlock.stencilPassOp())]);
    }
    // Check and set polygon offset
    if (!COMPARE(oldBlock.zBias(), newBlock.zBias())) {
        if (IS_ZERO(newBlock.zBias())) {
            glDisable(GL_POLYGON_OFFSET_FILL);
        } else {
            glEnable(GL_POLYGON_OFFSET_FILL);
            if (!COMPARE(oldBlock.zBias(), newBlock.zBias()) || !COMPARE(oldBlock.zUnits(), newBlock.zUnits())) {
                glPolygonOffset(newBlock.zBias(), newBlock.zUnits());
            }
        }
    }

    // Check and set colour mask
    if (oldBlock.colourWrite().i != newBlock.colourWrite().i) {
        P32 cWrite = newBlock.colourWrite();
        glColorMask(cWrite.b[0] == 1 ? GL_TRUE : GL_FALSE,   // R
                    cWrite.b[1] == 1 ? GL_TRUE : GL_FALSE,   // G
                    cWrite.b[2] == 1 ? GL_TRUE : GL_FALSE,   // B
                    cWrite.b[3] == 1 ? GL_TRUE : GL_FALSE);  // A
    }
}

void GLStateTracker::activateStateBlock(const RenderStateBlock& newBlock) {
    auto toggle = [](bool flag, GLenum state) {
        flag ? glEnable(state) : glDisable(state);
    };

    toggle(newBlock.cullEnabled(), GL_CULL_FACE);
    toggle(newBlock.stencilEnable(), GL_STENCIL_TEST);
    toggle(newBlock.depthTestEnabled(), GL_DEPTH_TEST);

    if (newBlock.cullMode() != CullMode::NONE) {
        GLenum targetMode = GLUtil::glCullModeTable[to_U32(newBlock.cullMode())];
        if (_currentCullMode != targetMode) {
            glCullFace(targetMode);
            _currentCullMode = targetMode;
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GLUtil::glFillModeTable[to_U32(newBlock.fillMode())]);
    glDepthFunc(GLUtil::glCompareFuncTable[to_U32(newBlock.zFunc())]);
    glStencilMask(newBlock.stencilWriteMask());
    glStencilFunc(GLUtil::glCompareFuncTable[to_U32(newBlock.stencilFunc())],
                  newBlock.stencilRef(),
                  newBlock.stencilMask());
    glStencilOp(GLUtil::glStencilOpTable[to_U32(newBlock.stencilFailOp())],
                GLUtil::glStencilOpTable[to_U32(newBlock.stencilZFailOp())],
                GLUtil::glStencilOpTable[to_U32(newBlock.stencilPassOp())]);

    if (IS_ZERO(newBlock.zBias())) {
        glDisable(GL_POLYGON_OFFSET_FILL);
    } else {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(newBlock.zBias(), newBlock.zUnits());
    }

    P32 cWrite = newBlock.colourWrite();
    glColorMask(cWrite.b[0] == 1 ? GL_TRUE : GL_FALSE,   // R
                cWrite.b[1] == 1 ? GL_TRUE : GL_FALSE,   // G
                cWrite.b[2] == 1 ? GL_TRUE : GL_FALSE,   // B
                cWrite.b[3] == 1 ? GL_TRUE : GL_FALSE);  // A
    
}


}; //namespace Divide