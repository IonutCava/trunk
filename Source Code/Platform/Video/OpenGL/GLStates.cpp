#include "Headers/GLWrapper.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Platform/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"

namespace Divide {

/// The following static variables are used to remember the current OpenGL state
GLint  GL_API::_maxTextureUnits = 0;
GLuint GL_API::_activeShaderProgram = 0;
GLuint GL_API::_indirectDrawBuffer = 0;
GLint  GL_API::_activePackUnpackAlignments[] = {1, 1};
GLint  GL_API::_activePackUnpackRowLength[] = {0, 0};
GLint  GL_API::_activePackUnpackSkipPixels[] = {0, 0};
GLint  GL_API::_activePackUnpackSkipRows[] = {0, 0};
GLuint GL_API::_activeVAOID = GLUtil::_invalidObjectID;
GLuint GL_API::_activeTextureUnit = GLUtil::_invalidObjectID;
GLuint GL_API::_activeTransformFeedback = GLUtil::_invalidObjectID;
GLuint GL_API::_activeFBID[] = {GLUtil::_invalidObjectID,
                                GLUtil::_invalidObjectID,
                                GLUtil::_invalidObjectID};
GLuint GL_API::_activeBufferID[] = {GLUtil::_invalidObjectID,
                                    GLUtil::_invalidObjectID,
                                    GLUtil::_invalidObjectID,
                                    GLUtil::_invalidObjectID,
                                    GLUtil::_invalidObjectID,
                                    GLUtil::_invalidObjectID,
                                    GLUtil::_invalidObjectID};

bool GL_API::_primitiveRestartEnabled = false;
vec4<GLfloat> GL_API::_prevClearColor;
GL_API::textureBoundMapDef GL_API::_textureBoundMap;
GL_API::samplerBoundMapDef GL_API::_samplerBoundMap;
GL_API::samplerObjectMap GL_API::_samplerMap;

/// Reset as much of the GL default state as possible within the limitations given
void GL_API::clearStates(const bool skipTextures,
                         const bool skipBuffers,
                         const bool skipScissor) {

    if (!skipTextures) {
        for (textureBoundMapDef::value_type& it : _textureBoundMap) {
            GL_API::bindTexture(it.first, 0, it.second.second);
        }

        setActiveTextureUnit(0);
    }

    if (!skipBuffers) {
        setPixelPackUnpackAlignment();
        setActiveVAO(0);
        setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, 0);
        setActiveBuffer(GL_ARRAY_BUFFER, 0);
        setActiveBuffer(GL_TEXTURE_BUFFER, 0);
        setActiveBuffer(GL_UNIFORM_BUFFER, 0);
        setActiveBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        setActiveTransformFeedback(0);
    }

    if (!skipScissor) {
        glDisable(GL_SCISSOR_TEST);
    }
    GL_API::clearColor(DefaultColors::DIVIDE_BLUE());
}

/// Pixel pack alignment is usually changed by textures, PBOs, etc
bool GL_API::setPixelPackAlignment(GLint packAlignment,
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
bool GL_API::setPixelUnpackAlignment(GLint unpackAlignment,
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

    if (_activePackUnpackRowLength[1] != rowLength) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
        _activePackUnpackRowLength[1] = rowLength;
        changed = true;
    }

    if (_activePackUnpackSkipRows[1] != skipRows) {
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
        _activePackUnpackSkipRows[1] = skipRows;
        changed = true;
    }

    if (_activePackUnpackSkipPixels[1] != skipPixels) {
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
        _activePackUnpackSkipPixels[1] = skipPixels;
        changed = true;
    }

    // We managed to change at least one entry
    return changed;
}

/// Enable or disable primitive restart and ensure that the correct index size
/// is used
void GL_API::togglePrimitiveRestart(bool state) {
    // Toggle primitive restart on or off
    if (_primitiveRestartEnabled != state) {
        _primitiveRestartEnabled = state;
        state ? glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX)
              : glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    }
}

/// Clipping planes are only enabled/disabled if they differ from the current
/// state
void GL_API::updateClipPlanes() {
    // Get the clip planes from the GFXDevice object
    const PlaneList& list = GFX_DEVICE.getClippingPlanes();
    // For every clip plane that we support (usually 6)
    for (U32 i = 0; i < Config::MAX_CLIP_PLANES; ++i) {
        // Check its state
        const bool& clipPlaneActive = list[i].active();
        // And compare it with OpenGL's current state
        if (_activeClipPlanes[i] != clipPlaneActive) {
            // Update the clip plane if it differs internally
            _activeClipPlanes[i] = clipPlaneActive;
            clipPlaneActive ? glEnable(GL_CLIP_DISTANCE0 + i)
                            : glDisable(GL_CLIP_DISTANCE0 + i);
        }
    }
}

bool GL_API::setActiveTextureUnit(GLuint unit) {
    GLuint temp = 0;
    return setActiveTextureUnit(unit, temp);
}

/// Set the currently active texture unit
bool GL_API::setActiveTextureUnit(GLuint unit, GLuint& previousUnit) {
    previousUnit = _activeTextureUnit;
    // Prevent double bind
    if (_activeTextureUnit == unit) {
        return false;
    }
    // Update and remember internal state
    _activeTextureUnit = unit;
    glActiveTexture(GL_TEXTURE0 + unit);

    return true;
}

bool GL_API::bindSamplers(GLuint unitOffset,
                          GLuint samplerCount,
                          GLuint* samplerHandles) {
    if (samplerCount > 0 &&
        unitOffset + samplerCount <
            static_cast<GLuint>(GL_API::_maxTextureUnits)) {
        GLuint offset = static_cast<GLuint>(GL_TEXTURE0) + unitOffset;
        glBindSamplers(offset, samplerCount, samplerHandles);

        if (!samplerHandles) {
            for (GLuint i = 0; i < samplerCount; ++i) {
                _samplerBoundMap.erase(_samplerBoundMap.find(offset + i));
            }
        } else {
            for (GLuint i = 0; i < samplerCount; ++i) {
                _samplerBoundMap[offset + i] = samplerHandles[i];
            }
        }

        return true;
    }

    return false;
}

/// Bind the sampler object described by the hash value to the specified unit
bool GL_API::bindSampler(GLuint unit, size_t samplerHash) {

    samplerBoundMapDef::iterator it = _samplerBoundMap.find(unit);
    if (it == std::end(_samplerBoundMap)) {
        hashAlg::emplace(_samplerBoundMap, static_cast<GLushort>(unit),
                         samplerHash);
    } else {
        // Prevent double bind
        if (it->second == samplerHash) {
            return false;
        } else {
            // Remember the new binding state for future reference
            it->second = samplerHash;
        }
    }
    // Get the sampler object defined by the hash value and bind it to the
    // specified unit (0 is a valid sampler object)
    glBindSampler(unit, getSamplerHandle(samplerHash));

    return true;
}

bool GL_API::bindTextures(GLuint unitOffset,
                          GLuint textureCount,
                          GLuint* textureHandles,
                          GLenum* targets,
                          GLuint* samplerHandles) {
    if (textureCount > 0 &&
        unitOffset + textureCount <
            static_cast<GLuint>(GL_API::_maxTextureUnits)) {
        GLuint offset = static_cast<GLuint>(GL_TEXTURE0) + unitOffset;
        GL_API::bindSamplers(unitOffset, textureCount, samplerHandles);
        glBindTextures(offset, textureCount, textureHandles);

        if (!textureHandles) {
            for (GLuint i = 0; i < textureCount; ++i) {
                _textureBoundMap.erase(_textureBoundMap.find(offset + i));
            }
        } else {
            for (GLuint i = 0; i < textureCount; ++i) {
                std::pair<GLuint, GLenum>& currentMapping =
                    _textureBoundMap[offset + i];
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
bool GL_API::bindTexture(GLuint unit,
                         GLuint handle,
                         GLenum target,
                         size_t samplerHash) {
    // Fail if we specified an invalid unit. Assert instead of returning false
    // because this might be related to a bad algorithm
    DIVIDE_ASSERT(
        unit < static_cast<GLuint>(GL_API::_maxTextureUnits),
        "GLStates error: invalid texture unit specified as a texture binding "
        "slot!");
    // Bind the sampler object first, as we may just require a sampler update
    // instead of a full texture rebind
    GL_API::bindSampler(unit, samplerHash);
    // Prevent double bind only for the texture
    std::pair<GLuint, GLenum>& currentMapping = _textureBoundMap[unit];
    if (currentMapping.first != handle || currentMapping.second != target) {
        // Remember the new binding state for future reference
        currentMapping.first = handle;
        currentMapping.second = target;
        // Bind the texture to the current unit
        GL_API::setActiveTextureUnit(unit);
        glBindTexture(target, handle); 
        
        return true;
    }

    return false;
}

bool GL_API::setActiveFB(Framebuffer::FramebufferUsage usage, GLuint ID) {
    GLuint temp = 0;
    return setActiveFB(usage, ID, temp);
}
/// Switch the current framebuffer by binding it as either a R/W buffer, read
/// buffer or write buffer
bool GL_API::setActiveFB(Framebuffer::FramebufferUsage usage, GLuint ID, GLuint& previousID) {
    // We may query the active framebuffer handle and get an invalid handle in
    // return and then try to bind the queried handle
    // This is, for example, in save/restore FB scenarios. An invalid handle
    // will just reset the buffer binding
    if (ID == GLUtil::_invalidObjectID) {
        ID = 0;
    }
    previousID = _activeFBID[to_uint(usage)];
    // Prevent double bind
    if (_activeFBID[to_uint(usage)] == ID) {
        if (usage == Framebuffer::FramebufferUsage::FB_READ_WRITE) {
            if (_activeFBID[to_uint(
                    Framebuffer::FramebufferUsage::FB_READ_ONLY)] == ID &&
                _activeFBID[to_uint(
                    Framebuffer::FramebufferUsage::FB_WRITE_ONLY)] == ID) {
                return false;
            }
        } else {
            return false;
        }
    }
    // Bind the requested buffer to the appropriate target
    switch (usage) {
        case Framebuffer::FramebufferUsage::FB_READ_WRITE: {
            // According to documentation this is equivalent to independent
            // calls to
            // bindFramebuffer(read, ID) and bindFramebuffer(write, ID)
            glBindFramebuffer(GL_FRAMEBUFFER, ID);
            // This also overrides the read and write bindings
            _activeFBID[to_uint(Framebuffer::FramebufferUsage::FB_READ_ONLY)] =
                ID;
            _activeFBID[to_uint(Framebuffer::FramebufferUsage::FB_WRITE_ONLY)] =
                ID;
        } break;
        case Framebuffer::FramebufferUsage::FB_READ_ONLY: {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, ID);
        } break;
        case Framebuffer::FramebufferUsage::FB_WRITE_ONLY: {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ID);
        } break;
    };
    // Remember the new binding state for future reference
    _activeFBID[to_uint(usage)] = ID;
    return true;
}

bool GL_API::setActiveVAO(GLuint ID) {
    GLuint temp = 0;
    return setActiveVAO(ID, temp);
}

/// Switch the currently active vertex array object
bool GL_API::setActiveVAO(GLuint ID, GLuint& previousID) {
    previousID = _activeVAOID;
    // Prevent double bind
    if (_activeVAOID == ID) {
        return false;
    }
    // Remember the new binding for future reference
    _activeVAOID = ID;
    // Activate the specified VAO
    glBindVertexArray(ID);

    return true;
}

bool GL_API::setActiveTransformFeedback(GLuint ID) {
    GLuint temp = 0;
    return setActiveTransformFeedback(ID, temp);
}

/// Bind the specified transform feedback object
bool GL_API::setActiveTransformFeedback(GLuint ID, GLuint& previousID) {
    previousID = _activeTransformFeedback;
    // Prevent double bind
    if (_activeTransformFeedback == ID) {
        return false;
    }
    // Remember the new binding for future reference
    _activeTransformFeedback = ID;
    // Activate the specified TFO
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, ID);

    return true;
}

/// Single place to change buffer objects for every target available
bool GL_API::setActiveBuffer(GLenum target, GLuint ID, GLuint& previousID) {
    // We map buffer targets from 0 to n in a static array
    GLint index = -1;
    // Select the appropriate index in the array based on the buffer target
    switch (target) {
        case GL_ARRAY_BUFFER:
            index = 0;
            break;
        case GL_TEXTURE_BUFFER:
            index = 1;
            break;
        case GL_UNIFORM_BUFFER:
            index = 2;
            break;
        case GL_SHADER_STORAGE_BUFFER:
            index = 3;
            break;
        case GL_ELEMENT_ARRAY_BUFFER:
            index = 4;
            break;
        case GL_PIXEL_UNPACK_BUFFER:
            index = 5;
            break;
        case GL_DRAW_INDIRECT_BUFFER:
            index = 6;
            break;
        default:
            // Make sure the target is available. Assert if it isn't as this
            // means that a non-supported feature is used somewhere
            DIVIDE_ASSERT(
                IS_IN_RANGE_INCLUSIVE(index, 0, 6),
                "GLStates error: attempted to bind an invalid buffer target!");
            return false;
    };

    // Prevent double bind
    previousID = _activeBufferID[index];
    if (previousID == ID) {
        return false;
    }
    // Remember the new binding for future reference
    _activeBufferID[index] = ID;
    // Bind the specified buffer handle to the desired buffer target
    glBindBuffer(target, ID);

    return true;
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
    if (GL_API::_activeShaderProgram == programHandle) {
        return false;
    }
    // Remember the new binding for future reference
    GL_API::_activeShaderProgram = programHandle;
    // Bind the new program
    glUseProgram(programHandle);

    return true;
}

/// Change the clear color
void GL_API::clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a, vec4<GLfloat>& prevColor) {
    prevColor.set(_prevClearColor);
    // Prevent setting the clear color to the exact same value again
    if (_prevClearColor != vec4<F32>(r, g, b, a)) {
        // Remember the current clear color for this target for future reference
        _prevClearColor.set(r, g, b, a);
        // Update the clear target
        glClearColor(r, g, b, a);
    }
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

/// A state block should contain all rendering state changes needed for the next
/// draw call.
/// Some may be redundant, so we check each one individually
void GL_API::activateStateBlock(const RenderStateBlock& newBlock,
                                RenderStateBlock* const oldBlock) const {

    auto toggle = [](bool flag, GLenum state) {
        flag ? glEnable(state) : glDisable(state);
    };

    // Compare toggle-only states with the previous block
    if (!oldBlock || oldBlock->blendEnable() != newBlock.blendEnable()) {
        toggle(newBlock.blendEnable(), GL_BLEND);
    }

    if (!oldBlock || oldBlock->cullEnabled() != newBlock.cullEnabled()) {
        toggle(newBlock.cullEnabled(), GL_CULL_FACE);
    }
    if (!oldBlock || oldBlock->stencilEnable() != newBlock.stencilEnable()) {
        toggle(newBlock.stencilEnable(), GL_STENCIL_TEST);
    }
    if (!oldBlock || oldBlock->zEnable() != newBlock.zEnable()) {
        toggle(newBlock.zEnable(), GL_DEPTH_TEST);
    }
    // Check line width
    if (!oldBlock || !FLOAT_COMPARE(oldBlock->lineWidth(), newBlock.lineWidth())) {
        glLineWidth(std::min(newBlock.lineWidth(), static_cast<F32>(_lineWidthLimit)));
    }
    // Check separate blend functions
    if (!oldBlock ||
        oldBlock->blendSrc() != newBlock.blendSrc() ||
        oldBlock->blendDest() != newBlock.blendDest()) {
        glBlendFuncSeparate(GLUtil::glBlendTable[to_uint(newBlock.blendSrc())],
                            GLUtil::glBlendTable[to_uint(newBlock.blendDest())],
                            GL_ONE,
                            GL_ZERO);
    }

    // Check the blend equation
    if (!oldBlock || oldBlock->blendOp() != newBlock.blendOp()) {
        glBlendEquation(GLUtil::glBlendOpTable[to_uint(newBlock.blendOp())]);
    }
    // Check culling mode (back (CW) / front (CCW) by default)
    if (!oldBlock || oldBlock->cullMode() != newBlock.cullMode()) {
        if (newBlock.cullMode() != CullMode::NONE) {
            glCullFace(GLUtil::glCullModeTable[to_uint(newBlock.cullMode())]);
        }
    }
    // Check rasterization mode
    if (!oldBlock || oldBlock->fillMode() != newBlock.fillMode()) {
        glPolygonMode(GL_FRONT_AND_BACK,
                      GLUtil::glFillModeTable[to_uint(newBlock.fillMode())]);
    }
    // Check the depth function
    if (!oldBlock || oldBlock->zFunc() != newBlock.zFunc()) {
        glDepthFunc(GLUtil::glCompareFuncTable[to_uint(newBlock.zFunc())]);
    }
    // Check if we need to toggle the depth mask
    if (!oldBlock || oldBlock->zWriteEnable() != newBlock.zWriteEnable()) {
        glDepthMask(newBlock.zWriteEnable() ? GL_TRUE : GL_FALSE);
    }
    // Check if we need to change the stencil mask
    if (!oldBlock || oldBlock->stencilWriteMask() != newBlock.stencilWriteMask()) {
        glStencilMask(newBlock.stencilWriteMask());
    }
    // Stencil function is dependent on 3 state parameters set together
    if (!oldBlock ||
        oldBlock->stencilFunc() != newBlock.stencilFunc() ||
        oldBlock->stencilRef()  != newBlock.stencilRef() ||
        oldBlock->stencilMask() != newBlock.stencilMask()) {
        glStencilFunc(GLUtil::glCompareFuncTable[to_uint(newBlock.stencilFunc())],
                      newBlock.stencilRef(),
                      newBlock.stencilMask());
    }
    // Stencil operation is also dependent  on 3 state parameters set together
    if (!oldBlock ||
        oldBlock->stencilFailOp() != newBlock.stencilFailOp() ||
        oldBlock->stencilZFailOp() != newBlock.stencilZFailOp() ||
        oldBlock->stencilPassOp() != newBlock.stencilPassOp()) {
        glStencilOp(GLUtil::glStencilOpTable[to_uint(newBlock.stencilFailOp())],
                    GLUtil::glStencilOpTable[to_uint(newBlock.stencilZFailOp())],
                    GLUtil::glStencilOpTable[to_uint(newBlock.stencilPassOp())]);
    }
    // Check and set polygon offset
    if (!oldBlock || !FLOAT_COMPARE(oldBlock->zBias(), newBlock.zBias())) {
        if (IS_ZERO(newBlock.zBias())) {
            glDisable(GL_POLYGON_OFFSET_FILL);
        } else {
            glEnable(GL_POLYGON_OFFSET_FILL);
            if (!oldBlock || !FLOAT_COMPARE(oldBlock->zUnits(), newBlock.zUnits())) {
                glPolygonOffset(newBlock.zBias(), newBlock.zUnits());
            }
        }
    }

    // Check and set color mask
    if (!oldBlock || oldBlock->colorWrite().i != newBlock.colorWrite().i) {
        glColorMask(newBlock.colorWrite().b[0] == 1 ? GL_TRUE : GL_FALSE,   // R
                    newBlock.colorWrite().b[1] == 1 ? GL_TRUE : GL_FALSE,   // G
                    newBlock.colorWrite().b[2] == 1 ? GL_TRUE : GL_FALSE,   // B
                    newBlock.colorWrite().b[3] == 1 ? GL_TRUE : GL_FALSE);  // A
    }
}
};