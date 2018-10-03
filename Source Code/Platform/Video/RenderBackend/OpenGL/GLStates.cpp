#include "stdafx.h"

#include "config.h"

#include "Headers/GLWrapper.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glLockManager.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"

namespace Divide {

/// The following static variables are used to remember the current OpenGL state
GLuint GL_API::s_UBOffsetAlignment = 0;
GLuint GL_API::s_UBMaxSize = 0;
GLuint GL_API::s_SSBOffsetAlignment = 0;
GLuint GL_API::s_SSBMaxSize = 0;
GLint  GL_API::s_lineWidthLimit = 1;
GLuint GL_API::s_dummyVAO = 0;
GLint  GL_API::s_maxTextureUnits = 0;
GLint  GL_API::s_maxAttribBindings = 0;
GLint  GL_API::s_maxFBOAttachments = 0;
GLuint GL_API::s_activeShaderProgram = 0;
GLuint GL_API::s_anisoLevel = 0;
GLint  GL_API::s_activePackUnpackAlignments[] = {1, 1};
GLint  GL_API::s_activePackUnpackRowLength[] = {0, 0};
GLint  GL_API::s_activePackUnpackSkipPixels[] = {0, 0};
GLint  GL_API::s_activePackUnpackSkipRows[] = {0, 0};
GLuint GL_API::s_activeVAOID = GLUtil::_invalidObjectID;
GLuint GL_API::s_activeTransformFeedback = GLUtil::_invalidObjectID;
GLuint GL_API::s_activeFBID[] = {GLUtil::_invalidObjectID,
                                 GLUtil::_invalidObjectID,
                                 GLUtil::_invalidObjectID};
GLuint GL_API::s_activeBufferID[] = {GLUtil::_invalidObjectID,
                                     GLUtil::_invalidObjectID,
                                     GLUtil::_invalidObjectID,
                                     GLUtil::_invalidObjectID,
                                     GLUtil::_invalidObjectID,
                                     GLUtil::_invalidObjectID};
hashMap<GLuint, GLuint> GL_API::s_activeVAOIB;

VAOBindings GL_API::s_vaoBufferData;
GLfloat GL_API::s_depthNearVal = 0.0f;
Pipeline const* GL_API::s_activePipeline = nullptr;
glFramebuffer* GL_API::s_activeRenderTarget = nullptr;
glPixelBuffer* GL_API::s_activePixelBuffer = nullptr;

UColour GL_API::s_blendColour = UColour(0u);
Rect<I32> GL_API::s_activeViewport = Rect<I32>(-1);
Rect<I32> GL_API::s_activeScissor = Rect<I32>(-1);
FColour GL_API::s_activeClearColour = DefaultColours::DIVIDE_BLUE;
GLfloat GL_API::s_depthFarVal = 1.0f;
bool GL_API::s_primitiveRestartEnabled = false;
bool GL_API::s_rasterizationEnabled = true;
bool GL_API::s_opengl46Supported = false;
U32 GL_API::s_patchVertexCount = 0;
size_t GL_API::s_currentStateBlockHash = 0;
size_t GL_API::s_previousStateBlockHash = 0;
GL_API::textureBoundMapDef GL_API::s_textureBoundMap;
GL_API::imageBoundMapDef GL_API::s_imageBoundMap;
std::mutex GL_API::s_mipmapQueueSetLock;
hashMap<GLuint, GLsync> GL_API::s_mipmapQueueSync;
GL_API::samplerBoundMapDef GL_API::s_samplerBoundMap;
GL_API::samplerObjectMap GL_API::s_samplerMap;
std::mutex GL_API::s_samplerMapLock;
GLUtil::glVAOPool GL_API::s_vaoPool;
glHardwareQueryPool* GL_API::s_hardwareQueryPool = nullptr;

BlendingProperties GL_API::s_blendPropertiesGlobal{
    BlendProperty::ONE,
    BlendProperty::ONE,
    BlendOperation::ADD
};

GLboolean GL_API::s_blendEnabledGlobal;
vector<BlendingProperties> GL_API::s_blendProperties;
vector<GLboolean> GL_API::s_blendEnabled;

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

/// Reset as much of the GL default state as possible within the limitations given
void GL_API::clearStates(const DisplayWindow& mainWindow) {
    GL_API::bindTextures(0, GL_API::s_maxTextureUnits, nullptr, nullptr);
    setPixelPackUnpackAlignment();
    setActiveVAO(0);
    setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, 0);
    setActiveBuffer(GL_TEXTURE_BUFFER, 0);
    setActiveBuffer(GL_UNIFORM_BUFFER, 0);
    setActiveBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    setActiveTransformFeedback(0);

    for (vec_size i = 0; i < GL_API::s_blendEnabled.size(); ++i) {
        setBlending((GLuint)i, BlendingProperties(), true);
    }
    GL_API::setBlendColour(UColour(0u), true);

    const vec2<U16>& drawableSize = _context.getDrawableSize(mainWindow);
    GL_API::setScissor(Rect<I32>(0, 0, drawableSize.width, drawableSize.height));

    s_activePipeline = nullptr;
    s_activeRenderTarget = nullptr;
    s_activePixelBuffer = nullptr;
    s_activeViewport.set(-1);
    s_activeClearColour.set(DefaultColours::DIVIDE_BLUE);

    Attorney::GLAPIShaderProgram::unbind();
}

/// Pixel pack alignment is usually changed by textures, PBOs, etc
bool GL_API::setPixelPackAlignment(GLint packAlignment,
                                   GLint rowLength,
                                   GLint skipRows,
                                   GLint skipPixels) {
    // Keep track if we actually affect any OpenGL state
    bool changed = false;
    if (s_activePackUnpackAlignments[0] != packAlignment) {
        glPixelStorei(GL_PACK_ALIGNMENT, packAlignment);
        s_activePackUnpackAlignments[0] = packAlignment;
        changed = true;
    }

    if (s_activePackUnpackRowLength[0] != rowLength) {
        glPixelStorei(GL_PACK_ROW_LENGTH, rowLength);
        s_activePackUnpackRowLength[0] = rowLength;
        changed = true;
    }

    if (s_activePackUnpackSkipRows[0] != skipRows) {
        glPixelStorei(GL_PACK_SKIP_ROWS, skipRows);
        s_activePackUnpackSkipRows[0] = skipRows;
        changed = true;
    }

    if (s_activePackUnpackSkipPixels[0] != skipPixels) {
        glPixelStorei(GL_PACK_SKIP_PIXELS, skipPixels);
        s_activePackUnpackSkipPixels[0] = skipPixels;
        changed = true;
    }

    // We managed to change at least one entry
    return changed;
}

size_t GL_API::setStateBlockInternal(size_t stateBlockHash) {
    // If the new state hash is different from the previous one
    if (stateBlockHash != s_currentStateBlockHash) {
        // Remember the previous state hash
        s_previousStateBlockHash = s_currentStateBlockHash;
        // Update the current state hash
        s_currentStateBlockHash = stateBlockHash;

        bool currentStateValid = false;
        const RenderStateBlock& currentState = RenderStateBlock::get(s_currentStateBlockHash, currentStateValid);
        if (s_previousStateBlockHash != 0) {
            bool previousStateValid = false;
            const RenderStateBlock& previousState = RenderStateBlock::get(s_previousStateBlockHash, previousStateValid);

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
    return s_previousStateBlockHash;
}
/// Pixel unpack alignment is usually changed by textures, PBOs, etc
bool GL_API::setPixelUnpackAlignment(GLint unpackAlignment,
                                     GLint rowLength,
                                     GLint skipRows,
                                     GLint skipPixels) {
    // Keep track if we actually affect any OpenGL state
    bool changed = false;
    if (s_activePackUnpackAlignments[1] != unpackAlignment) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
        s_activePackUnpackAlignments[1] = unpackAlignment;
        changed = true;
    }

    if (rowLength != -1 && s_activePackUnpackRowLength[1] != rowLength) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
        s_activePackUnpackRowLength[1] = rowLength;
        changed = true;
    }

    if (skipRows != -1 && s_activePackUnpackSkipRows[1] != skipRows) {
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
        s_activePackUnpackSkipRows[1] = skipRows;
        changed = true;
    }

    if (skipPixels != -1 && s_activePackUnpackSkipPixels[1] != skipPixels) {
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
        s_activePackUnpackSkipPixels[1] = skipPixels;
        changed = true;
    }

    // We managed to change at least one entry
    return changed;
}

/// Enable or disable primitive restart and ensure that the correct index size
/// is used
void GL_API::togglePrimitiveRestart(bool state) {
    // Toggle primitive restart on or off
    if (s_primitiveRestartEnabled != state) {
        s_primitiveRestartEnabled = state;
        state ? glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX)
              : glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    }
}

void GL_API::setPatchVertexCount(U32 count) {
    if (s_patchVertexCount != count) {
        s_patchVertexCount = count;
        glPatchParameteri(GL_PATCH_VERTICES, s_patchVertexCount);
    }
}
/// Enable or disable primitive rasterization
void GL_API::toggleRasterization(bool state) {
    // Toggle primitive restart on or off
    if (s_rasterizationEnabled != state) {
        s_rasterizationEnabled = state;
        state ? glDisable(GL_RASTERIZER_DISCARD)
              : glEnable(GL_RASTERIZER_DISCARD);
    }
}

/// Clipping planes are only enabled/disabled if they differ from the current
/// state
void GL_API::updateClipPlanes(const FrustumClipPlanes& list) {
    // For every clip plane that we support (usually 6)
    for (U32 i = 0; i < to_base(ClipPlaneIndex::COUNT); ++i) {
        // Check its state and compare it with OpenGL's current state
        bool& activePlane = _activeClipPlanes[i];
        if (activePlane != list._active[i]) {
            // Update the clip plane if it differs internally
            activePlane = !activePlane;
            activePlane ? glEnable(GLenum((U32)GL_CLIP_DISTANCE0 + i))
                        : glDisable(GLenum((U32)GL_CLIP_DISTANCE0 + i));
        }
    }
}

bool GL_API::deleteBuffers(GLuint count, GLuint* buffers) {
    if (count > 0 && buffers != nullptr) {
        for (GLuint i = 0; i < count; ++i) {
            GLuint crtBuffer = buffers[i];
            for (GLuint& boundBuffer : s_activeBufferID) {
                if (boundBuffer == crtBuffer) {
                    boundBuffer = 0;
                }
            }

            for (auto boundBuffer : s_activeVAOIB) {
                if (boundBuffer.second == crtBuffer) {
                    boundBuffer.second = 0;
                }
            }
        }

        glDeleteBuffers(count, buffers);
        memset(buffers, 0, count * sizeof(GLuint));
        return true;
    }

    return false;
}

bool GL_API::deleteVAOs(GLuint count, GLuint* vaos) {
    if (count > 0 && vaos != nullptr) {
        for (GLuint i = 0; i < count; ++i) {
            if (s_activeVAOID == vaos[i]) {
                s_activeVAOID = 0;
                break;
            }
        }
        glDeleteVertexArrays(count, vaos);
        memset(vaos, 0, count * sizeof(GLuint));
        return true;
    }
    return false;
}

bool GL_API::deleteFramebuffers(GLuint count, GLuint* framebuffers) {
    if (count > 0 && framebuffers != nullptr) {
        for (GLuint i = 0; i < count; ++i) {
            GLuint crtFB = framebuffers[i];
            for (GLuint& activeFB : s_activeFBID) {
                if (activeFB == crtFB) {
                    activeFB = 0;
                }
            }
        }
        glDeleteFramebuffers(count, framebuffers);
        memset(framebuffers, 0, count * sizeof(GLuint));
        return true;
    }
    return false;
}

bool GL_API::deleteShaderPrograms(GLuint count, GLuint* programs) {
    if (count > 0 && programs != nullptr) {
        for (GLuint i = 0; i < count; ++i) {
            if (s_activeShaderProgram == programs[i]) {
                s_activeShaderProgram = 0;
                break;
            }
            glDeleteProgram(programs[i]);
        }
        
        memset(programs, 0, count * sizeof(GLuint));
        return true;
    }
    return false;
}

bool GL_API::deleteTextures(GLuint count, GLuint* textures) {
    if (count > 0 && textures != nullptr) {
        
        for (GLuint i = 0; i < count; ++i) {
            GLuint crtTex = textures[i];
            if (crtTex != 0) {
                for (GLuint& boundTex : s_textureBoundMap) {
                    if (boundTex == crtTex) {
                        boundTex = 0;
                    }
                }
                for (ImageBindSettings& settings : s_imageBoundMap) {
                    if (settings._texture == crtTex) {
                        settings.reset();
                    }
                }
            }
        }
        glDeleteTextures(count, textures);
        memset(textures, 0, count * sizeof(GLuint));
        return true;
    }

    return false;
}

bool GL_API::deleteSamplers(GLuint count, GLuint* samplers) {
    if (count > 0 && samplers != nullptr) {

        for (GLuint i = 0; i < count; ++i) {
            GLuint crtSampler = samplers[i];
            if (crtSampler != 0) {
                for (GLuint& boundSampler : s_samplerBoundMap) {
                    if (boundSampler == crtSampler) {
                        boundSampler = 0;
                    }
                }
            }
        }
        glDeleteSamplers(count, samplers);
        memset(samplers, 0, count * sizeof(GLuint));
        return true;
    }

    return false;
}

bool GL_API::bindSamplers(GLushort unitOffset,
                          GLuint samplerCount,
                          GLuint* samplerHandles) {
    if (samplerCount > 0 &&
        unitOffset + samplerCount < static_cast<GLuint>(GL_API::s_maxTextureUnits))
    {
        if (samplerCount == 1) {
            GLuint& handle = s_samplerBoundMap[unitOffset];
            GLuint targetHandle = samplerHandles ? samplerHandles[0] : 0u;
            if (handle != targetHandle) {
                glBindSampler(unitOffset, targetHandle);
                handle = targetHandle;
                return true;
            }
        } else {
            glBindSamplers(unitOffset, samplerCount, samplerHandles);
            if (samplerHandles != nullptr) {
                memcpy(&s_samplerBoundMap[unitOffset], samplerHandles, sizeof(GLuint) * samplerCount);
            } else {
                memset(&s_samplerBoundMap[unitOffset], 0, sizeof(GLuint) * samplerCount);
            }
            return true;
        } 
    }

    return false;
}

bool GL_API::bindTextures(GLushort unitOffset,
                          GLuint textureCount,
                          GLuint* textureHandles,
                          GLuint* samplerHandles) {

    if (textureHandles != nullptr) {

        std::stack<GLuint> textures;
        for (GLuint i = 0; i < textureCount; ++i) {
            if (textureHandles[i] > 0) {
                textures.push(textureHandles[i]);
            }
        }

        while(!textures.empty()) {
            std::pair<GLuint, GLsync> entry;
            {
                UniqueLock w_lock(s_mipmapQueueSetLock);
                auto it = s_mipmapQueueSync.find(textures.top());
                if (it == std::cend(s_mipmapQueueSync)) {
                    textures.pop();
                    continue;
                }
                entry = std::make_pair(it->first, it->second);
                if (entry.second != nullptr) {
                    s_mipmapQueueSync.erase(it);
                }
            }

            if (entry.second == nullptr) {
                continue;
            }
            glLockManager::wait(&entry.second, false /*this should work, right?*/);
            glDeleteSync(entry.second);
            textures.pop();
        }
    }

    bool bound = false;
    if (textureCount > 0 &&
        unitOffset + textureCount < static_cast<GLuint>(GL_API::s_maxTextureUnits))
    {
        GL_API::bindSamplers(unitOffset, textureCount, samplerHandles);

        if (textureCount == 1) {
            GLuint& crtHandle = s_textureBoundMap[unitOffset];
            GLuint targetHandle = textureHandles ? textureHandles[0] : 0u;

            if (crtHandle != targetHandle) {
                glBindTextureUnit(unitOffset, targetHandle);
                crtHandle = targetHandle;
                bound = true;
            }
        } else {
            glBindTextures(unitOffset, textureCount, textureHandles);
            if (textureHandles != nullptr) {
                memcpy(&s_textureBoundMap[unitOffset], textureHandles, sizeof(GLuint) * textureCount);
            } else {
                memset(&s_textureBoundMap[unitOffset], 0, sizeof(GLuint) * textureCount);
            }

            bound = true;
        }
    }

    return false;
}

/// Bind a texture specified by a GL handle and GL type to the specified unit
/// using the sampler object defined by hash value
bool GL_API::bindTexture(GLushort unit,
                         GLuint handle,
                         GLuint samplerHandle) {
    // Fail if we specified an invalid unit. Assert instead of returning false
    // because this might be related to a bad algorithm
    DIVIDE_ASSERT(unit < static_cast<GLuint>(GL_API::s_maxTextureUnits),
                  "GLStates error: invalid texture unit specified as a texture binding slot!");
    return bindTextures(unit, 1, &handle, &samplerHandle);
}

bool GL_API::bindTextureImage(GLushort unit, GLuint handle, GLint level,
                              bool layered, GLint layer, GLenum access,
                              GLenum format) {
    static ImageBindSettings tempSettings;
    tempSettings = {handle, level, layered ? GL_TRUE : GL_FALSE, layer, access, format};

    ImageBindSettings& settings = s_imageBoundMap[unit];
    if (settings != tempSettings) {
        glBindImageTexture(unit, handle, level, layered ? GL_TRUE : GL_FALSE, layer, access, format);
        settings = tempSettings;
        return true;
    }

    return false;
}

/// Single place to change buffer objects for every target available
bool GL_API::bindActiveBuffer(GLuint vaoID, GLuint location, GLuint bufferID, GLintptr offset, GLsizei stride) {
    const VAOBindings::BufferBindingParams& bindings = s_vaoBufferData.bindingParams(vaoID, location);

    VAOBindings::BufferBindingParams currentParams(bufferID, offset, stride);
    if (bindings != currentParams) {
        // Bind the specified buffer handle to the desired buffer target
        glVertexArrayVertexBuffer(vaoID, location, bufferID, offset, stride);
        // Remember the new binding for future reference
        s_vaoBufferData.bindingParams(vaoID, location, currentParams);
        return true;
    }

    return false;
}

bool GL_API::setActiveFB(RenderTarget::RenderTargetUsage usage, GLuint ID) {
    GLuint temp = 0;
    return setActiveFB(usage, ID, temp);
}

/// Switch the current framebuffer by binding it as either a R/W buffer, read
/// buffer or write buffer
bool GL_API::setActiveFB(RenderTarget::RenderTargetUsage usage, GLuint ID, GLuint& previousID) {
    // We may query the active framebuffer handle and get an invalid handle in
    // return and then try to bind the queried handle
    // This is, for example, in save/restore FB scenarios. An invalid handle
    // will just reset the buffer binding
    if (ID == GLUtil::_invalidObjectID) {
        ID = 0;
    }
    previousID = s_activeFBID[to_U32(usage)];
    // Prevent double bind
    if (s_activeFBID[to_U32(usage)] == ID) {
        if (usage == RenderTarget::RenderTargetUsage::RT_READ_WRITE) {
            if (s_activeFBID[to_base(RenderTarget::RenderTargetUsage::RT_READ_ONLY)] == ID &&
                s_activeFBID[to_base(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY)] == ID) {
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
            s_activeFBID[to_base(RenderTarget::RenderTargetUsage::RT_READ_ONLY)] = ID;
            s_activeFBID[to_base(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY)] = ID;
        } break;
        case RenderTarget::RenderTargetUsage::RT_READ_ONLY: {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, ID);
        } break;
        case RenderTarget::RenderTargetUsage::RT_WRITE_ONLY: {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ID);
        } break;
    };

    // Remember the new binding state for future reference
    s_activeFBID[to_U32(usage)] = ID;

    return true;
}

bool GL_API::setActiveVAO(GLuint ID) {
    GLuint temp = 0;
    return setActiveVAO(ID, temp);
}

/// Switch the currently active vertex array object
bool GL_API::setActiveVAO(GLuint ID, GLuint& previousID) {
    previousID = s_activeVAOID;
    // Prevent double bind
    if (s_activeVAOID != ID) {
        // Remember the new binding for future reference
        s_activeVAOID = ID;
        //setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        // Activate the specified VAO
        glBindVertexArray(ID);
        return true;
    }

    return false;
}

bool GL_API::setActiveTransformFeedback(GLuint ID) {
    GLuint temp = 0;
    return setActiveTransformFeedback(ID, temp);
}

/// Bind the specified transform feedback object
bool GL_API::setActiveTransformFeedback(GLuint ID, GLuint& previousID) {
    previousID = s_activeTransformFeedback;
    // Prevent double bind
    if (s_activeTransformFeedback != ID) {
        // Remember the new binding for future reference
        s_activeTransformFeedback = ID;
        // Activate the specified TFO
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, ID);
        return true;
    }

    return false;
}

/// Single place to change buffer objects for every target available
bool GL_API::setActiveBuffer(GLenum target, GLuint ID, GLuint& previousID) {
    GLuint& crtBinding = target == GL_ELEMENT_ARRAY_BUFFER 
                                 ? s_activeVAOIB[s_activeVAOID]
                                 : s_activeBufferID[getBufferTargetIndex(target)];
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

bool GL_API::setActiveBuffer(GLenum target, GLuint ID) {
    GLuint temp = 0;
    return setActiveBuffer(target, ID, temp);
}

/// Change the currently active shader program. Passing null will unbind shaders
/// (will use program 0)
bool GL_API::setActiveProgram(GLuint programHandle) {
    // Check if we are binding a new program or unbinding all shaders
    // Prevent double bind
    if (GL_API::s_activeShaderProgram != programHandle) {
        // Remember the new binding for future reference
        GL_API::s_activeShaderProgram = programHandle;
        // Bind the new program
        glUseProgram(programHandle);
        return true;
    }

    return false;
}

void GL_API::setDepthRange(F32 nearVal, F32 farVal) {
    CLAMP(nearVal, 0.0f, 1.0f);
    CLAMP(farVal, 0.0f, 1.0f);
    if (!COMPARE(nearVal, s_depthNearVal) && !COMPARE(farVal, s_depthFarVal)) {
        glDepthRange(nearVal, farVal);
        GL_API::s_depthNearVal = nearVal;
        GL_API::s_depthFarVal = farVal;
    }
}

void GL_API::setBlendColour(const UColour& blendColour, bool force) {
    if (GL_API::s_blendColour != blendColour || force) {
        FColour floatColour = Util::ToFloatColour(blendColour);
        glBlendColor(static_cast<GLfloat>(floatColour.r),
                     static_cast<GLfloat>(floatColour.g),
                     static_cast<GLfloat>(floatColour.b),
                     static_cast<GLfloat>(floatColour.a));

        GL_API::s_blendColour.set(blendColour);
    }
}

void GL_API::setBlending(const BlendingProperties& blendingProperties, bool force) {
    bool enable = blendingProperties.blendEnabled();

    if ((GL_API::s_blendEnabledGlobal == GL_TRUE) != enable || force) {
        enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        GL_API::s_blendEnabledGlobal = (enable ? GL_TRUE : GL_FALSE);
        std::fill(std::begin(GL_API::s_blendEnabled), std::end(GL_API::s_blendEnabled), GL_API::s_blendEnabledGlobal);
    }

    if (enable || force) {
        if (GL_API::s_blendPropertiesGlobal != blendingProperties || force)
        {
            if (blendingProperties._blendSrcAlpha != BlendProperty::COUNT) {
                glBlendFuncSeparate(GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                                    GLUtil::glBlendTable[to_base(blendingProperties._blendDest)],
                                    GLUtil::glBlendTable[to_base(blendingProperties._blendSrcAlpha)],
                                    GLUtil::glBlendTable[to_base(blendingProperties._blendDestAlpha)]);

                glBlendEquationSeparate(GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                            ? to_base(blendingProperties._blendOp)
                                                                                            : to_base(BlendOperation::ADD)],
                                        GLUtil::glBlendOpTable[to_base(blendingProperties._blendOpAlpha)]);
            } else {
                glBlendFunc(GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                            GLUtil::glBlendTable[to_base(blendingProperties._blendDest)]);
                    glBlendEquation(GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                        ? to_base(blendingProperties._blendOp)
                                                                                        : to_base(BlendOperation::ADD)]);

            }

            GL_API::s_blendPropertiesGlobal = blendingProperties;

            std::fill(std::begin(GL_API::s_blendProperties), std::end(GL_API::s_blendProperties), GL_API::s_blendPropertiesGlobal);
        }
    }
}

void GL_API::setBlending(GLuint drawBufferIdx,const BlendingProperties& blendingProperties, bool force) {
    bool enable = blendingProperties.blendEnabled();

    assert(drawBufferIdx < (GLuint)(GL_API::s_maxFBOAttachments));

    if ((GL_API::s_blendEnabled[drawBufferIdx] == GL_TRUE) != enable || force) {
        enable ? glEnablei(GL_BLEND, drawBufferIdx) : glDisablei(GL_BLEND, drawBufferIdx);
        GL_API::s_blendEnabled[drawBufferIdx] = (enable ? GL_TRUE : GL_FALSE);
        if (!enable) {
            GL_API::s_blendEnabledGlobal = GL_FALSE;
        }
    }

    if (enable || force) {
        if (GL_API::s_blendProperties[drawBufferIdx] != blendingProperties || force) {
            if (blendingProperties._blendSrcAlpha != BlendProperty::COUNT) {
                glBlendFuncSeparatei(drawBufferIdx,
                                     GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                                     GLUtil::glBlendTable[to_base(blendingProperties._blendDest)],
                                     GLUtil::glBlendTable[to_base(blendingProperties._blendSrcAlpha)],
                                     GLUtil::glBlendTable[to_base(blendingProperties._blendDestAlpha)]);

                glBlendEquationSeparatei(drawBufferIdx, 
                                         GLUtil::glBlendOpTable[to_base(blendingProperties._blendOp)],
                                         GLUtil::glBlendOpTable[to_base(blendingProperties._blendOpAlpha)]);

                glBlendEquationSeparatei(drawBufferIdx, 
                                         GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                             ? to_base(blendingProperties._blendOp)
                                                                                             : to_base(BlendOperation::ADD)],
                                         GLUtil::glBlendOpTable[to_base(blendingProperties._blendOpAlpha)]);
            } else {
                glBlendFunci(drawBufferIdx,
                             GLUtil::glBlendTable[to_base(blendingProperties._blendSrc)],
                             GLUtil::glBlendTable[to_base(blendingProperties._blendDest)]);

                glBlendEquationi(drawBufferIdx,
                                 GLUtil::glBlendOpTable[blendingProperties._blendOp != BlendOperation::COUNT
                                                                                     ? to_base(blendingProperties._blendOp)
                                                                                     : to_base(BlendOperation::ADD)]);
            }

            GL_API::s_blendProperties[drawBufferIdx] = blendingProperties;
            GL_API::s_blendPropertiesGlobal.reset();
        }
    }
}

/// Change the current viewport area. Redundancy check is performed in GFXDevice class
bool GL_API::setViewport(I32 x, I32 y, I32 width, I32 height) {
    if (width > 0 && height > 0 && Rect<I32>(x, y, width, height) != GL_API::s_activeViewport) {
        // Debugging and profiling the application may require setting a 1x1 viewport to exclude fill rate bottlenecks
        if (Config::Profile::USE_1x1_VIEWPORT) {
            glViewport(x, y, 1, 1);
        } else {
            glViewport(x, y, width, height);
        }
        GL_API::s_activeViewport.set(x, y, width, height);
        
        return true;
    }

    return false;
}

bool GL_API::setClearColour(const FColour& colour) {
    if (colour != GL_API::s_activeClearColour) {
        glClearColor(colour.r, colour.g, colour.b, colour.a);
        GL_API::s_activeClearColour.set(colour);
        return true;
    }

    return false;
}

bool GL_API::setScissor(I32 x, I32 y, I32 width, I32 height) {
    if (Rect<I32>(x, y, width, height) != GL_API::s_activeScissor) {
        glScissor(x, y, width, height);
        GL_API::s_activeScissor.set(x, y, width, height);
        return true;
    }

    return false;
}

GLuint GL_API::getBoundTextureHandle(GLuint slot) {
    return s_textureBoundMap[slot];
}

void GL_API::getActiveViewport(GLint* vp) {
    if (vp != nullptr) {
        vp = (GLint*)s_activeViewport._v;
    }
}

/// A state block should contain all rendering state changes needed for the next draw call.
/// Some may be redundant, so we check each one individually
void GL_API::activateStateBlock(const RenderStateBlock& newBlock,
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
    if (oldBlock.zEnable() != newBlock.zEnable()) {
        toggle(newBlock.zEnable(), GL_DEPTH_TEST);
    }
    if (oldBlock.scissorTestEnable() != newBlock.scissorTestEnable()) {
        toggle(newBlock.scissorTestEnable(), GL_SCISSOR_TEST);
    }
    // Check culling mode (back (CW) / front (CCW) by default)
    if (oldBlock.cullMode() != newBlock.cullMode()) {
        if (newBlock.cullMode() != CullMode::NONE) {
            glCullFace(GLUtil::glCullModeTable[to_U32(newBlock.cullMode())]);
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

void GL_API::activateStateBlock(const RenderStateBlock& newBlock) {
    auto toggle = [](bool flag, GLenum state) {
        flag ? glEnable(state) : glDisable(state);
    };

    toggle(newBlock.cullEnabled(), GL_CULL_FACE);
    toggle(newBlock.stencilEnable(), GL_STENCIL_TEST);
    toggle(newBlock.zEnable(), GL_DEPTH_TEST);

    if (newBlock.cullMode() != CullMode::NONE) {
        glCullFace(GLUtil::glCullModeTable[to_U32(newBlock.cullMode())]);
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

};