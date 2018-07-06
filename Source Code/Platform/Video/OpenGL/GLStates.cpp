#include "config.h"

#include "Headers/GLWrapper.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Platform/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"

namespace Divide {

/// The following static variables are used to remember the current OpenGL state
GLuint GL_API::s_dummyVAO = 0;
GLint  GL_API::s_maxTextureUnits = 0;
GLint  GL_API::s_maxAttribBindings = 0;
GLuint GL_API::s_activeShaderProgram = 0;
GLuint GL_API::s_indirectDrawBuffer = 0;
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
                                     GLUtil::_invalidObjectID,
                                     GLUtil::_invalidObjectID};
VAOBindings GL_API::s_vaoBufferData;
bool GL_API::s_primitiveRestartEnabled = false;
bool GL_API::s_rasterizationEnabled = true;
GL_API::textureBoundMapDef GL_API::s_textureBoundMap;
GL_API::imageBoundMapDef GL_API::s_imageBoundMap;
GL_API::samplerBoundMapDef GL_API::s_samplerBoundMap;
GL_API::samplerObjectMap GL_API::s_samplerMap;
SharedLock GL_API::s_samplerMapLock;

/// Reset as much of the GL default state as possible within the limitations given
void GL_API::clearStates() {
    static const vec4<F32> clearColour = DefaultColours::DIVIDE_BLUE();

    for(U16 i = 0; i < to_ushort(GL_API::s_maxTextureUnits); ++i) {
        const std::pair<GLuint, GLenum>& it  = s_textureBoundMap[i];
        if (it.second != GL_ZERO) {
            GL_API::bindTexture(i, 0, it.second);
        }
    }

    setPixelPackUnpackAlignment();
    setActiveVAO(0);
    setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, 0);
    setActiveBuffer(GL_ARRAY_BUFFER, 0);
    setActiveBuffer(GL_TEXTURE_BUFFER, 0);
    setActiveBuffer(GL_UNIFORM_BUFFER, 0);
    setActiveBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    setActiveTransformFeedback(0);
    
    glDisable(GL_SCISSOR_TEST);
    glClearColor(clearColour.r, clearColour.g, clearColour.b, clearColour.a);
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

    if (s_activePackUnpackRowLength[1] != rowLength) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
        s_activePackUnpackRowLength[1] = rowLength;
        changed = true;
    }

    if (s_activePackUnpackSkipRows[1] != skipRows) {
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
        s_activePackUnpackSkipRows[1] = skipRows;
        changed = true;
    }

    if (s_activePackUnpackSkipPixels[1] != skipPixels) {
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
void GL_API::updateClipPlanes() {
    // Get the clip planes from the GFXDevice object
    const PlaneList& list = Attorney::GFXDeviceAPI::getClippingPlanes(_context);
    // For every clip plane that we support (usually 6)
    for (U32 i = 0; i < to_const_uint(Frustum::FrustPlane::COUNT); ++i) {
        // Check its state
        const bool& clipPlaneActive = list[i].active();
        // And compare it with OpenGL's current state
        if (_activeClipPlanes[i] != clipPlaneActive) {
            // Update the clip plane if it differs internally
            _activeClipPlanes[i] = clipPlaneActive;
            clipPlaneActive ? glEnable(GLenum((U32)GL_CLIP_DISTANCE0 + i))
                            : glDisable(GLenum((U32)GL_CLIP_DISTANCE0 + i));
        }
    }
}

bool GL_API::bindSamplers(GLushort unitOffset,
                          GLuint samplerCount,
                          GLuint* samplerHandles) {
    if (samplerCount > 0 &&
        unitOffset + samplerCount < static_cast<GLuint>(GL_API::s_maxTextureUnits))
    {
        glBindSamplers(unitOffset, samplerCount, samplerHandles);

        if (!samplerHandles) {
            for (GLushort i = 0; i < samplerCount; ++i) {
               s_samplerBoundMap[unitOffset + i] = 0;
            }
        } else {
            for (GLushort i = 0; i < samplerCount; ++i) {
                s_samplerBoundMap[unitOffset + i] = samplerHandles[i];
            }
        }

        return true;
    }

    return false;
}

/// Bind the sampler object described by the hash value to the specified unit
bool GL_API::bindSampler(GLushort unit, size_t samplerHash) {

    GLuint samplerHandle = getSamplerHandle(samplerHash);
    return bindSamplers(unit, 1, &samplerHandle);
}

bool GL_API::bindTextures(GLushort unitOffset,
                          GLuint textureCount,
                          GLuint* textureHandles,
                          GLenum* targets,
                          GLuint* samplerHandles) {
    if (textureCount > 0 &&
        unitOffset + textureCount < static_cast<GLuint>(GL_API::s_maxTextureUnits))
    {
        GL_API::bindSamplers(unitOffset, textureCount, samplerHandles);
        glBindTextures(unitOffset, textureCount, textureHandles);

        if (!textureHandles) {
            for (GLushort i = 0; i < textureCount; ++i) {
                s_textureBoundMap[unitOffset + i].first = 0;
            }
        } else {
            for (GLushort i = 0; i < textureCount; ++i) {
                std::pair<GLuint, GLenum>& currentMapping = s_textureBoundMap[unitOffset + i];
                currentMapping.first = textureHandles[i];
                currentMapping.second = targets[i];
            }
        }
        return true;
    }

    return false;
}

// Bind a texture specified by a GL handle and GL type to the specified unit
/// using the sampler object defined by hash value
bool GL_API::bindTexture(GLushort unit,
                         GLuint handle,
                         GLenum target,
                         size_t samplerHash) {
    // Fail if we specified an invalid unit. Assert instead of returning false
    // because this might be related to a bad algorithm
    DIVIDE_ASSERT(unit < static_cast<GLuint>(GL_API::s_maxTextureUnits),
                  "GLStates error: invalid texture unit specified as a texture binding slot!");
    GLuint samplerHandle = getSamplerHandle(samplerHash);
    return bindTextures(unit, 1, &handle, &target, &samplerHandle);
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
    previousID = s_activeFBID[to_uint(usage)];
    // Prevent double bind
    if (s_activeFBID[to_uint(usage)] == ID) {
        if (usage == RenderTarget::RenderTargetUsage::RT_READ_WRITE) {
            if (s_activeFBID[to_const_uint(RenderTarget::RenderTargetUsage::RT_READ_ONLY)] == ID &&
                s_activeFBID[to_const_uint(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY)] == ID) {
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
            s_activeFBID[to_const_uint(RenderTarget::RenderTargetUsage::RT_READ_ONLY)] = ID;
            s_activeFBID[to_const_uint(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY)] = ID;
        } break;
        case RenderTarget::RenderTargetUsage::RT_READ_ONLY: {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, ID);
        } break;
        case RenderTarget::RenderTargetUsage::RT_WRITE_ONLY: {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ID);
        } break;
    };

    // Remember the new binding state for future reference
    s_activeFBID[to_uint(usage)] = ID;
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
        setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
    // We map buffer targets from 0 to n in a static array
    GLint index = -1;
    // Select the appropriate index in the array based on the buffer target
    switch (target) {
        case GL_ARRAY_BUFFER: {
            index = 0;
        }break;
        case GL_TEXTURE_BUFFER: {
            index = 1;
        }break;
        case GL_UNIFORM_BUFFER: {
            index = 2;
        }break;
        case GL_SHADER_STORAGE_BUFFER: {
            index = 3;
        }break;
        case GL_ELEMENT_ARRAY_BUFFER: {
            index = 4;
        }break;
        case GL_PIXEL_UNPACK_BUFFER: {
            index = 5;
        }break;
        case GL_DRAW_INDIRECT_BUFFER: {
            index = 6;
        }break;
        default: {
            // Make sure the target is available. Assert if it isn't as this
            // means that a non-supported feature is used somewhere
            DIVIDE_ASSERT(IS_IN_RANGE_INCLUSIVE(index, 0, 6),
                          "GLStates error: attempted to bind an invalid buffer target!");
            return false;
        }
    };

    // Prevent double bind
    previousID = s_activeBufferID[index];
    if (previousID != ID) {
        // Remember the new binding for future reference
        s_activeBufferID[index] = ID;
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

/// Change the current viewport area. Redundancy check is performed in GFXDevice
/// class
void GL_API::changeViewport(const vec4<I32>& newViewport) const {
    // Debugging and profiling the application may require setting a 1x1
    // viewport to exclude fill rate bottlenecks
    if (Config::Profile::USE_1x1_VIEWPORT) {
        glViewport(newViewport.x, newViewport.y, 1, 1);
    } else {
        glViewport(newViewport.x, newViewport.y, newViewport.z, newViewport.w);
    }
}

/// A state block should contain all rendering state changes needed for the next draw call.
/// Some may be redundant, so we check each one individually
void GL_API::activateStateBlock(const RenderStateBlock& newBlock,
                                const RenderStateBlock& oldBlock) const {
    auto toggle = [](bool flag, GLenum state) {
        flag ? glEnable(state) : glDisable(state);
    };

    // Compare toggle-only states with the previous block
    if (oldBlock.blendEnable() != newBlock.blendEnable()) {
        toggle(newBlock.blendEnable(), GL_BLEND);
    }

    if (oldBlock.cullEnabled() != newBlock.cullEnabled()) {
        toggle(newBlock.cullEnabled(), GL_CULL_FACE);
    }
    if (oldBlock.stencilEnable() != newBlock.stencilEnable()) {
        toggle(newBlock.stencilEnable(), GL_STENCIL_TEST);
    }
    if (oldBlock.zEnable() != newBlock.zEnable()) {
        toggle(newBlock.zEnable(), GL_DEPTH_TEST);
    }
    // Check separate blend functions
    if (oldBlock.blendSrc() != newBlock.blendSrc() ||
        oldBlock.blendDest() != newBlock.blendDest()) {
        glBlendFuncSeparate(GLUtil::glBlendTable[to_uint(newBlock.blendSrc())],
                            GLUtil::glBlendTable[to_uint(newBlock.blendDest())],
                            GL_ONE,
                            GL_ZERO);
    }

    // Check the blend equation
    if (oldBlock.blendOp() != newBlock.blendOp()) {
        glBlendEquation(GLUtil::glBlendOpTable[to_uint(newBlock.blendOp())]);
    }
    // Check culling mode (back (CW) / front (CCW) by default)
    if (oldBlock.cullMode() != newBlock.cullMode()) {
        if (newBlock.cullMode() != CullMode::NONE) {
            glCullFace(GLUtil::glCullModeTable[to_uint(newBlock.cullMode())]);
        }
    }
    // Check rasterization mode
    if (oldBlock.fillMode() != newBlock.fillMode()) {
        glPolygonMode(GL_FRONT_AND_BACK,
                      GLUtil::glFillModeTable[to_uint(newBlock.fillMode())]);
    }
    // Check the depth function
    if (oldBlock.zFunc() != newBlock.zFunc()) {
        glDepthFunc(GLUtil::glCompareFuncTable[to_uint(newBlock.zFunc())]);
    }

    // Check if we need to change the stencil mask
    if (oldBlock.stencilWriteMask() != newBlock.stencilWriteMask()) {
        glStencilMask(newBlock.stencilWriteMask());
    }
    // Stencil function is dependent on 3 state parameters set together
    if (oldBlock.stencilFunc() != newBlock.stencilFunc() ||
        oldBlock.stencilRef()  != newBlock.stencilRef() ||
        oldBlock.stencilMask() != newBlock.stencilMask()) {
        glStencilFunc(GLUtil::glCompareFuncTable[to_uint(newBlock.stencilFunc())],
                      newBlock.stencilRef(),
                      newBlock.stencilMask());
    }
    // Stencil operation is also dependent  on 3 state parameters set together
    if (oldBlock.stencilFailOp() != newBlock.stencilFailOp() ||
        oldBlock.stencilZFailOp() != newBlock.stencilZFailOp() ||
        oldBlock.stencilPassOp() != newBlock.stencilPassOp()) {
        glStencilOp(GLUtil::glStencilOpTable[to_uint(newBlock.stencilFailOp())],
                    GLUtil::glStencilOpTable[to_uint(newBlock.stencilZFailOp())],
                    GLUtil::glStencilOpTable[to_uint(newBlock.stencilPassOp())]);
    }
    // Check and set polygon offset
    if (!COMPARE(oldBlock.zBias(), newBlock.zBias())) {
        if (IS_ZERO(newBlock.zBias())) {
            glDisable(GL_POLYGON_OFFSET_FILL);
        } else {
            glEnable(GL_POLYGON_OFFSET_FILL);
            if (!COMPARE(oldBlock.zUnits(), newBlock.zUnits())) {
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

void GL_API::activateStateBlock(const RenderStateBlock& newBlock) const {
    auto toggle = [](bool flag, GLenum state) {
        flag ? glEnable(state) : glDisable(state);
    };

    toggle(newBlock.blendEnable(), GL_BLEND);
    toggle(newBlock.cullEnabled(), GL_CULL_FACE);
    toggle(newBlock.stencilEnable(), GL_STENCIL_TEST);
    toggle(newBlock.zEnable(), GL_DEPTH_TEST);
    glBlendFuncSeparate(GLUtil::glBlendTable[to_uint(newBlock.blendSrc())],
                        GLUtil::glBlendTable[to_uint(newBlock.blendDest())],
                        GL_ONE,
                        GL_ZERO);
    glBlendEquation(GLUtil::glBlendOpTable[to_uint(newBlock.blendOp())]);
    if (newBlock.cullMode() != CullMode::NONE) {
        glCullFace(GLUtil::glCullModeTable[to_uint(newBlock.cullMode())]);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GLUtil::glFillModeTable[to_uint(newBlock.fillMode())]);
    glDepthFunc(GLUtil::glCompareFuncTable[to_uint(newBlock.zFunc())]);
    glStencilMask(newBlock.stencilWriteMask());
    glStencilFunc(GLUtil::glCompareFuncTable[to_uint(newBlock.stencilFunc())],
                  newBlock.stencilRef(),
                  newBlock.stencilMask());
    glStencilOp(GLUtil::glStencilOpTable[to_uint(newBlock.stencilFailOp())],
                GLUtil::glStencilOpTable[to_uint(newBlock.stencilZFailOp())],
                GLUtil::glStencilOpTable[to_uint(newBlock.stencilPassOp())]);

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