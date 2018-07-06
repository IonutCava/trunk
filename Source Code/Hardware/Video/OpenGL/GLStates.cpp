#include "Headers/GLWrapper.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Shaders/Headers/ShaderManager.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Hardware/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"

namespace Divide {

/// The following static variables are used to remember the current OpenGL state
GLint  GL_API::_activePackUnpackAlignments[] = {1, 1};
GLint  GL_API::_activePackUnpackRowLength[]  = {0, 0};
GLint  GL_API::_activePackUnpackSkipPixels[] = {0, 0};
GLint  GL_API::_activePackUnpackSkipRows[]   = {0, 0};
GLuint GL_API::_activeVAOId = GLUtil::_invalidObjectID;
GLuint GL_API::_activeTextureUnit = GLUtil::_invalidObjectID;
GLuint GL_API::_activeTransformFeedback = GLUtil::_invalidObjectID;
GLuint GL_API::_activeFBId[] = { GLUtil::_invalidObjectID,
                                 GLUtil::_invalidObjectID,
                                 GLUtil::_invalidObjectID };
GLuint GL_API::_activeBufferId[] = { GLUtil::_invalidObjectID, 
                                     GLUtil::_invalidObjectID, 
                                     GLUtil::_invalidObjectID, 
                                     GLUtil::_invalidObjectID,
                                     GLUtil::_invalidObjectID,
                                     GLUtil::_invalidObjectID,
                                     GLUtil::_invalidObjectID };

bool GL_API::_lastRestartIndexSmall   = true;
bool GL_API::_primitiveRestartEnabled = false;
vec4<GLfloat> GL_API::_prevClearColor;
GL_API::textureBoundMapDef GL_API::_textureBoundMap;
GL_API::samplerBoundMapDef GL_API::_samplerBoundMap;
GL_API::samplerObjectMap   GL_API::_samplerMap;

/// Reset as much of the GL default state as possible within the limitations given
void GL_API::clearStates(const bool skipShader, const bool skipTextures, const bool skipBuffers) {
    if (!skipShader) {
        setActiveProgram(nullptr);
    }

    if (!skipTextures) {
        FOR_EACH(textureBoundMapDef::value_type& it, _textureBoundMap) {
            GL_API::bindTexture(it.first, 0, it.second.second);
        }

        setActiveTextureUnit(0);
    }

    if (!skipBuffers) {
        setPixelPackUnpackAlignment();
        setActiveVAO(0);
        setActiveFB(0, Framebuffer::FB_READ_WRITE);
        setActiveBuffer(GL_ARRAY_BUFFER, 0);
        setActiveBuffer(GL_TEXTURE_BUFFER, 0);
        setActiveBuffer(GL_UNIFORM_BUFFER, 0);
        setActiveBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        setActiveTransformFeedback(0);
    }

    GL_API::clearColor(DefaultColors::DIVIDE_BLUE());
}

/// Pixel pack alignment is usually changed by textures, PBOs, etc
bool GL_API::setPixelPackAlignment(GLint packAlignment, GLint rowLength, GLint skipRows, GLint skipPixels) {
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
bool GL_API::setPixelUnpackAlignment(GLint unpackAlignment, GLint rowLength, GLint skipRows, GLint skipPixels) {
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

/// Enable or disable primitive restart and ensure that the correct index size is used
void GL_API::togglePrimitiveRestart(bool state, bool smallIndices) {
    // Only change in index if we need primitive restart (we mostly don't)
    if (_lastRestartIndexSmall != smallIndices && state) {
        _lastRestartIndexSmall = smallIndices;
        glPrimitiveRestartIndex(smallIndices ? Config::PRIMITIVE_RESTART_INDEX_S : Config::PRIMITIVE_RESTART_INDEX_L);
    }
    // Toggle primitive restart on or off
    if (_primitiveRestartEnabled != state) {
        _primitiveRestartEnabled = state;
        state ? glEnable(GL_PRIMITIVE_RESTART) : glDisable(GL_PRIMITIVE_RESTART);
    }
}

/// Clipping planes are only enabled/disabled if they differ from the current state
void GL_API::updateClipPlanes() {
    // Get the clip planes from the GFXDevice object
    const PlaneList& list = GFX_DEVICE.getClippingPlanes();
    // For every clip plane that we support (usually 6)
    for (U8 i = 0; i < Config::MAX_CLIP_PLANES; ++i) {
        // Check its state
        const bool& clipPlaneActive = list[i].active();
        // And compare it with OpenGL's current state
        if (_activeClipPlanes[i] != clipPlaneActive) {
            // Update the clip plane if it differs internally
            _activeClipPlanes[i]  = clipPlaneActive;
            clipPlaneActive ? glEnable(GL_CLIP_DISTANCE0 + i) : glDisable(GL_CLIP_DISTANCE0 + i);
        }
    }
}

/// Set the currently active texture unit
bool GL_API::setActiveTextureUnit(GLuint unit) {
    // Prevent double bind
    if (_activeTextureUnit == unit) {
        return false; 
    }
    // Update and remember internal state
    _activeTextureUnit = unit;
    glActiveTexture(GL_TEXTURE0 + unit);

    return true;
}

/// Bind the sampler object described by the hash value to the specified unit
bool GL_API::bindSampler(GLuint unit, size_t samplerHash) {
    samplerBoundMapDef::iterator it = _samplerBoundMap.find(unit);
    if (it == _samplerBoundMap.end()) {
        _samplerBoundMap.emplace(unit, samplerHash);
    } else {
        // Prevent double bind
        if (it->second == samplerHash) {
            return false;
        } else {
            // Remember the new binding state for future reference
            it->second = samplerHash;
        }
    }
    // Get the sampler object defined by the hash value and bind it to the specified unit 
    glBindSampler(unit, getSamplerHandle(samplerHash));

    return true;
}

/// Bind a texture specified by a GL handle and GL type to the specified unit using the desired sampler object defined by hash value  
bool GL_API::bindTexture(GLuint unit, GLuint handle, GLenum type, size_t samplerHash) {
    // Fail if we specified an invalid unit. Assert instead of returning false because this might be related to a bad algorithm
    DIVIDE_ASSERT(unit < GFX_DEVICE.getMaxTextureSlots(), "GLStates error: invalid texture unit specified as a texture binding slot!");
    // Bind the sampler object first, as we may just require a sampler update instead of a full texture rebind
    GL_API::bindSampler(unit, samplerHash);
    // Prevent double bind only for the texture
    std::pair<GLuint, GLenum>& currentMapping = _textureBoundMap[unit];
    if (currentMapping.first == handle && currentMapping.second == type) {
        return false;
    }
    // Remember the new binding state for future reference
    currentMapping.first = handle;
    currentMapping.second = type;
    // Switch the texture unit to the one requested
    GL_API::setActiveTextureUnit(unit);
    // Bind the texture to the current unit
    glBindTexture(type, handle);

    return true;
}

/// Switch the current framebuffer by binding it as either a R/W buffer, read buffer or write buffer
bool GL_API::setActiveFB(GLuint id, Framebuffer::FramebufferUsage usage) {
    
    // We may query the active framebuffer handle and get an invalid handle in return and then try to bind the queried handle
    // This is, for example, in save/restore FB scenarios. An invalid handle will just reset the buffer binding
    if (id == GLUtil::_invalidObjectID) {
        id = 0;
    }
    // Prevent double bind
    if (_activeFBId[usage] == id) {
        if (usage == Framebuffer::FB_READ_WRITE) {
            if (_activeFBId[Framebuffer::FB_READ_ONLY] == id && _activeFBId[Framebuffer::FB_WRITE_ONLY] == id ) {
                return false;
            }
        } else {
            return false; 
        }
    }
    // Bind the requested buffer to the appropriate target
    switch(usage) {
        case Framebuffer::FB_READ_WRITE : {
            // According to documentation this is equivalent to independent calls to bindFramebuffer(read, id) and bindFramebuffer(write, id)
            glBindFramebuffer(GL_FRAMEBUFFER, id);
            // This also overrides the read and write bindings
            _activeFBId[Framebuffer::FB_READ_ONLY]  = id;
            _activeFBId[Framebuffer::FB_WRITE_ONLY] = id;
        } break;
        case Framebuffer::FB_READ_ONLY : {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, id);
        } break;
        case Framebuffer::FB_WRITE_ONLY : {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
        } break;
    };
    // Remember the new binding state for future reference
    _activeFBId[usage] = id;
    return true;
}

/// Switch the currently active vertex array object
bool GL_API::setActiveVAO(GLuint id) {
    // Prevent double bind
    if (_activeVAOId == id) {
        return false; 
    }
    // Remember the new binding for future reference
    _activeVAOId = id;
    // Activate the specified VAO
    glBindVertexArray(id);

    return true;
}

/// Bind the specified transform feedback object
bool GL_API::setActiveTransformFeedback(GLuint id) {
    // Prevent double bind
    if (_activeTransformFeedback == id) {
        return false;
    }
    // Remember the new binding for future reference
    _activeTransformFeedback = id;
    // Activate the specified TFO
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, id);

    return true;
}

/// Single place to change buffer objects for every target available
bool GL_API::setActiveBuffer(GLenum target, GLuint id) {
    // We map buffer targets from 0 to n in a static array
    GLint index = -1;
    // Select the appropriate index in the array based on the buffer target
    switch (target) {
        case GL_ARRAY_BUFFER          : index = 0; break;
        case GL_TEXTURE_BUFFER        : index = 1; break;
        case GL_UNIFORM_BUFFER        : index = 2; break;
        case GL_SHADER_STORAGE_BUFFER : index = 3; break;
        case GL_ELEMENT_ARRAY_BUFFER  : index = 4; break;
        case GL_PIXEL_UNPACK_BUFFER   : index = 5; break;
        case GL_DRAW_INDIRECT_BUFFER  : index = 6; break;
    };
    // Make sure the target is available. Assert if it isn't as this means that a non-supported feature is used somewhere
    DIVIDE_ASSERT(index != -1, "GLStates error: attempted to bind an invalid buffer target!");
    // Prevent double bind
    if (_activeBufferId[index] == id) {
        return false; 
    }
    // Remember the new binding for future reference
    _activeBufferId[index] = id;
    // Bind the specified buffer handle to the desired buffer target
    glBindBuffer(target, id);

    return true;
}

/// Change the currently active shader program. Passing null will unbind shaders (will use program 0)
bool GL_API::setActiveProgram(glShaderProgram* const program) {
    // Check if we are binding a new program or unbinding all shaders
    GLuint newProgramId = (program != nullptr) ? program->getId() : 0;
    // Get the previous program's handle to compare against. The active program is stored in the GFXDevice object, so indirects are unavoidable
    ShaderProgram* previousProgram = GFX_DEVICE.activeShaderProgram();
    GLuint oldProgramId = (previousProgram != nullptr) ? previousProgram->getId() : 0;
    // Prevent double bind
    if(oldProgramId == newProgramId) {
        return false; 
    }
    // Unbind the previous program (if we had one)
    if (previousProgram != nullptr) {
        previousProgram->unbind(false);
    }
    // Remember the new binding for future reference
    GFX_DEVICE.activeShaderProgram(program);
    // Bind the new program
    glUseProgram(newProgramId);

    return true;
}

/// Change the clear color
void GL_API::clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    DIVIDE_ASSERT(0 < Config::MAX_RENDER_TARGETS, "GLStates error: invalid render target ID. Increase Config::MAX_RENDER_TARGETS!");
    // Prevent setting the clear color to the exact same value again
    if (_prevClearColor != vec4<F32>(r,g,b,a)) {
        // Remember the current clear color for this target for future reference
        _prevClearColor.set(r, g, b, a);
        // Update the clear target
        glClearColor(r,g,b,a);
    }
}

/// Change the current viewport area. Redundancy check is performed in GFXDevice class
void GL_API::changeViewport(const vec4<I32>& newViewport) const {
    // Debugging and profiling the application may require setting a 1x1 viewport to exclude fill rate bottlenecks
    if (Config::Profile::USE_1x1_VIEWPORT) {
        glViewport(newViewport.x, newViewport.y, 1, 1);
    } else {
        glViewport(newViewport.x, newViewport.y, newViewport.z, newViewport.w);
    }
}

/// The following toggle checks are similar to those used in Torque3D for managing state
#ifndef SHOULD_TOGGLE
    #define SHOULD_TOGGLE(state) (!oldBlock || oldBlock->getDescriptor().state != newDescriptor.state)
    #define SHOULD_TOGGLE_2(state1, state2)  (!oldBlock ||SHOULD_TOGGLE(state1) || SHOULD_TOGGLE(state2))
    #define SHOULD_TOGGLE_3(state1, state2, state3) (!oldBlock ||SHOULD_TOGGLE_2(state1, state2) || SHOULD_TOGGLE(state3))
    #define TOGGLE_NO_CHECK(state, enumValue) newDescriptor.state ? glEnable(enumValue) : glDisable(enumValue)
    #define TOGGLE_WITH_CHECK(state, enumValue) if(SHOULD_TOGGLE(state)) TOGGLE_NO_CHECK(state, enumValue);
#endif

/// A state block should contain all rendering state changes needed for the next draw call. Some may be redundant, so we check each one individually 
void GL_API::activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock) const {
    // Get the new block's descriptor
    const RenderStateBlockDescriptor& newDescriptor = newBlock.getDescriptor();
    // Compare toggle-only states with the previous block
    TOGGLE_WITH_CHECK(_blendEnable,   GL_BLEND);
    TOGGLE_WITH_CHECK(_cullEnabled,   GL_CULL_FACE);
    TOGGLE_WITH_CHECK(_stencilEnable, GL_STENCIL_TEST);
    TOGGLE_WITH_CHECK(_zEnable,       GL_DEPTH_TEST);
    // Check separate blend functions
    if (SHOULD_TOGGLE_2(_blendSrc, _blendDest)) {
        glBlendFuncSeparate(GLUtil::GL_ENUM_TABLE::glBlendTable[newDescriptor._blendSrc], 
                            GLUtil::GL_ENUM_TABLE::glBlendTable[newDescriptor._blendDest], GL_ONE, GL_ZERO);
    }
    // Check the blend equation
    if (SHOULD_TOGGLE(_blendOp)) {
        glBlendEquation(GLUtil::GL_ENUM_TABLE::glBlendOpTable[newDescriptor._blendOp]);
    }
    // Check culling mode (back (CW) / front (CCW) by default) 
    if (SHOULD_TOGGLE(_cullMode)) {
        glCullFace(GLUtil::GL_ENUM_TABLE::glCullModeTable[newDescriptor._cullMode]);
    }
    // Check rasterization mode
    if (SHOULD_TOGGLE(_fillMode)) {
        glPolygonMode(GL_FRONT_AND_BACK, GLUtil::GL_ENUM_TABLE::glFillModeTable[newDescriptor._fillMode]);
    }
    // Check the depth function
    if (SHOULD_TOGGLE(_zFunc)) {
        glDepthFunc(GLUtil::GL_ENUM_TABLE::glCompareFuncTable[newDescriptor._zFunc]);
    }
    // Check if we need to toggle the depth mask
    if (SHOULD_TOGGLE(_zWriteEnable)) {
        glDepthMask(newDescriptor._zWriteEnable ? GL_TRUE : GL_FALSE);
    }
    // Check if we need to change the stencil mask
    if (SHOULD_TOGGLE(_stencilWriteMask)) {
        glStencilMask(newDescriptor._stencilWriteMask);
    }
    // Stencil function is dependent on 3 state parameters set together
    if (SHOULD_TOGGLE_3(_stencilFunc, _stencilRef, _stencilMask)) {
        glStencilFunc(GLUtil::GL_ENUM_TABLE::glCompareFuncTable[newDescriptor._stencilFunc], newDescriptor._stencilRef, newDescriptor._stencilMask);
    }
    // Stencil operation is also dependent  on 3 state parameters set together
    if (SHOULD_TOGGLE_3(_stencilFailOp, _stencilZFailOp, _stencilPassOp)) {
        glStencilOp(GLUtil::GL_ENUM_TABLE::glStencilOpTable[newDescriptor._stencilFailOp], 
                    GLUtil::GL_ENUM_TABLE::glStencilOpTable[newDescriptor._stencilZFailOp], 
                    GLUtil::GL_ENUM_TABLE::glStencilOpTable[newDescriptor._stencilPassOp]);
    }
    // Check and set polygon offset
    if (SHOULD_TOGGLE(_zBias)) {
        if (IS_ZERO(newDescriptor._zBias)) {
            glDisable(GL_POLYGON_OFFSET_FILL);
        } else {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(newDescriptor._zBias, newDescriptor._zUnits);
        }
    }
    // Check and set color mask
    if (SHOULD_TOGGLE(_colorWrite.i)) {
        glColorMask(newDescriptor._colorWrite.b.b0 == GL_TRUE, // R
                    newDescriptor._colorWrite.b.b1 == GL_TRUE, // G
                    newDescriptor._colorWrite.b.b2 == GL_TRUE, // B
                    newDescriptor._colorWrite.b.b3 == GL_TRUE);// A
    }
}

};