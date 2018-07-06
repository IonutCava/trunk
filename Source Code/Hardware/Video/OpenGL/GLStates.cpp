//OpenGL state management: Lights, matrices, viewport, bound objects etc
#include "Headers/GLWrapper.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Hardware/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"

GLuint GL_API::_activeVAOId = GL_INVALID_INDEX;
GLuint GL_API::_activeFBId  = GL_INVALID_INDEX;
GLuint GL_API::_activeBufferId[] = {GL_INVALID_INDEX, GL_INVALID_INDEX, GL_INVALID_INDEX, GL_INVALID_INDEX, GL_INVALID_INDEX, GL_INVALID_INDEX, GL_INVALID_INDEX};
GLuint GL_API::_activeTextureUnit       = GL_INVALID_INDEX;
GLuint GL_API::_activeTransformFeedback = GL_INVALID_INDEX;
GLint  GL_API::_activePackUnpackAlignments[] = {1, 1};
GLint  GL_API::_activePackUnpackRowLength[]  = {0, 0};
GLint  GL_API::_activePackUnpackSkipPixels[] = {0, 0};
GLint  GL_API::_activePackUnpackSkipRows[]   = {0, 0};

bool GL_API::_lastRestartIndexSmall   = true;
bool GL_API::_primitiveRestartEnabled = false;

GL_API::textureBoundMapDef GL_API::_textureBoundMap;
GL_API::samplerBoundMapDef GL_API::_samplerBoundMap;
GL_API::samplerObjectMap   GL_API::_samplerMap;

Unordered_map<GLuint, vec4<GLfloat> > GL_API::_prevClearColor;

void GL_API::clearStates(const bool skipShader,const bool skipTextures,const bool skipBuffers){
    if(!skipShader)
        setActiveProgram(nullptr);
    
    if(!skipTextures){
        FOR_EACH(textureBoundMapDef::value_type& it, _textureBoundMap)
            GL_API::bindTexture(it.first, 0, it.second.second);

        setActiveTextureUnit(0);
    }

    if(!skipBuffers){
        setPixelPackUnpackAlignment();
        setActiveVAO(0);
        setActiveFB(0, true, true);
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

bool GL_API::setPixelPackAlignment(GLint packAlignment, GLint rowLength, GLint skipRows, GLint skipPixels) {
    bool changed = false;
    if(_activePackUnpackAlignments[0] != packAlignment){
        glPixelStorei(GL_PACK_ALIGNMENT, packAlignment);
        _activePackUnpackAlignments[0] = packAlignment;
        changed = true;
    }
    if(_activePackUnpackRowLength[0] != rowLength){
        glPixelStorei(GL_PACK_ROW_LENGTH, rowLength);
        _activePackUnpackRowLength[0] = rowLength;
        changed = true;
    }
    if(_activePackUnpackSkipRows[0] != skipRows){
        glPixelStorei(GL_PACK_SKIP_ROWS, skipRows);
        _activePackUnpackSkipRows[0] = skipRows;
        changed = true;
    }
    if(_activePackUnpackSkipPixels[0] != skipPixels){
        glPixelStorei(GL_PACK_SKIP_PIXELS, skipPixels);
        _activePackUnpackSkipPixels[0] = skipPixels;
        changed = true;
    }

    return changed;
}

bool GL_API::setPixelUnpackAlignment(GLint unpackAlignment, GLint rowLength, GLint skipRows, GLint skipPixels) {
   bool changed = false;
    if(_activePackUnpackAlignments[1] != unpackAlignment){
        glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
        _activePackUnpackAlignments[1] = unpackAlignment;
        changed = true;
    }
    if(_activePackUnpackRowLength[1] != rowLength){
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
        _activePackUnpackRowLength[1] = rowLength;
        changed = true;
    }
    if(_activePackUnpackSkipRows[1] != skipRows){
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
        _activePackUnpackSkipRows[1] = skipRows;
        changed = true;
    }
    if(_activePackUnpackSkipPixels[1] != skipPixels){
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
        _activePackUnpackSkipPixels[1] = skipPixels;
        changed = true;
    }

    return changed;
}

void GL_API::togglePrimitiveRestart(bool state, bool smallIndices){
    if (_lastRestartIndexSmall != smallIndices && state){
        _lastRestartIndexSmall = smallIndices;
        glPrimitiveRestartIndex(smallIndices ? Config::PRIMITIVE_RESTART_INDEX_S : Config::PRIMITIVE_RESTART_INDEX_L);
    }
    if (_primitiveRestartEnabled != state){
        _primitiveRestartEnabled = state;
        state ? glEnable(GL_PRIMITIVE_RESTART) : glDisable(GL_PRIMITIVE_RESTART);
    }
}

void GL_API::updateClipPlanes(){
    bool clipPlaneActive = false;

    const PlaneList& list = GFX_DEVICE.getClippingPlanes();
    
    for (U8 i = 0; i < Config::MAX_CLIP_PLANES; ++i){
        clipPlaneActive = list[i].active();

        if (_activeClipPlanes[i] != clipPlaneActive){
            _activeClipPlanes[i]  = clipPlaneActive;
            clipPlaneActive ? glEnable(GL_CLIP_DISTANCE0 + i) : glDisable(GL_CLIP_DISTANCE0 + i);
        }
    }
}

bool GL_API::setActiveTextureUnit(GLuint unit){
    if(_activeTextureUnit == unit)
        return false; //< prevent double bind

    _activeTextureUnit = unit;
    glActiveTexture(GL_TEXTURE0 + unit);

    return true;
}

bool GL_API::bindSampler(GLuint unit, size_t samplerHash){
    if(_samplerBoundMap[unit] != samplerHash){
        glBindSampler(unit, getSamplerHandle(samplerHash));
        _samplerBoundMap[unit] = samplerHash;
        return true;
    }
    return false;
}
  
bool GL_API::bindTexture(GLuint unit, GLuint handle, GLenum type, size_t samplerHash){
    assert(unit < GFX_DEVICE.getMaxTextureSlots());

    GL_API::bindSampler(unit, samplerHash);

    std::pair<GLuint, GLenum>& currentMapping = _textureBoundMap[unit];
    if (currentMapping.first != handle || currentMapping.second != type){
        GL_API::setActiveTextureUnit(unit);
        glBindTexture(type, handle);
        currentMapping.first = handle;
        currentMapping.second = type;
        return true;
    }
    return false;
}

bool GL_API::setActiveFB(GLuint id, const bool read, const bool write){
    if (_activeFBId == id)
        return false; //<prevent double bind

    _activeFBId = id;
    if (read && write)      glBindFramebuffer(GL_FRAMEBUFFER, id);
    else if(read && !write) glBindFramebuffer(GL_READ_FRAMEBUFFER, id);
    else                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);

    return true;
}

bool GL_API::setActiveVAO(GLuint id){
    if(_activeVAOId == id)
        return false; //<prevent double bind
        
    _activeVAOId = id;
    glBindVertexArray(id);

    return true;
}

bool GL_API::setActiveTransformFeedback(GLuint id){
    if(_activeTransformFeedback == id)
        return false;

    _activeTransformFeedback = id;
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, id);
    return true;
}

bool GL_API::setActiveBuffer(GLenum target, GLuint id){
    GLint index = -1;
    switch (target){
        case GL_ARRAY_BUFFER          : index = 0; break;
        case GL_TEXTURE_BUFFER        : index = 1; break;
        case GL_UNIFORM_BUFFER        : index = 2; break;
        case GL_SHADER_STORAGE_BUFFER : index = 3; break;
        case GL_ELEMENT_ARRAY_BUFFER  : index = 4; break;
        case GL_PIXEL_UNPACK_BUFFER   : index = 5; break;
        case GL_DRAW_INDIRECT_BUFFER  : index = 6; break;
    };

    DIVIDE_ASSERT(index != -1, "GLStates error: attempted to bind an invalid buffer target!");

    if (_activeBufferId[index] == id)
        return false; //<prevent double bind

    _activeBufferId[index] = id;
    glBindBuffer(target, id);

    return true;
}

bool GL_API::setActiveProgram(glShaderProgram* const program){
    GLuint newProgramId = (program != nullptr) ? program->getId() : 0;
    GLuint oldProgramId = (GFX_DEVICE.activeShaderProgram() != nullptr) ? GFX_DEVICE.activeShaderProgram()->getId() : 0;
    if(oldProgramId == newProgramId)
        return false; //<prevent double bind

    if (GFX_DEVICE.activeShaderProgram() != nullptr)
        GFX_DEVICE.activeShaderProgram()->unbind(false);

    GFX_DEVICE.activeShaderProgram(program);

    glUseProgram(newProgramId);
    return true;
}

void GL_API::clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLuint renderTarget){
    vec4<F32>& prevClearColor = _prevClearColor[renderTarget];
    if (prevClearColor != vec4<F32>(r,g,b,a)){
        prevClearColor.set(r, g, b, a);
        glClearColor(r,g,b,a);
    }
}

void GL_API::changeViewport(const vec4<I32>& newViewport) const {
    if (Config::Profile::USE_1x1_VIEWPORT)
        glViewport(newViewport.x, newViewport.y, 1, 1);
    else
        glViewport(newViewport.x, newViewport.y, newViewport.z, newViewport.w);

}

#ifndef SHOULD_TOGGLE
    #define SHOULD_TOGGLE(state) (!oldBlock || oldBlock->getDescriptor().state != newDescriptor.state)
    #define SHOULD_TOGGLE_2(state1, state2)  (!oldBlock ||SHOULD_TOGGLE(state1) || SHOULD_TOGGLE(state2))
    #define SHOULD_TOGGLE_3(state1, state2, state3) (!oldBlock ||SHOULD_TOGGLE_2(state1, state2) || SHOULD_TOGGLE(state3))
    #define SHOULD_TOGGLE_4(state1, state2, state3, state4) (!oldBlock || SHOULD_TOGGLE_3(state1, state2, state3) || SHOULD_TOGGLE(state4))
    #define TOGGLE_NO_CHECK(state, enumValue) newDescriptor.state ? glEnable(enumValue) : glDisable(enumValue)
    #define TOGGLE_WITH_CHECK(state, enumValue) if(SHOULD_TOGGLE(state)) TOGGLE_NO_CHECK(state, enumValue);
#endif

void GL_API::activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock) const {
    // ------------------- PASS INDEPENDENT STATES -------------------------------------- //
    const RenderStateBlockDescriptor& newDescriptor = newBlock.getDescriptor();

    TOGGLE_WITH_CHECK(_blendEnable,   GL_BLEND);
    TOGGLE_WITH_CHECK(_cullEnabled,   GL_CULL_FACE);
    TOGGLE_WITH_CHECK(_stencilEnable, GL_STENCIL_TEST);
    TOGGLE_WITH_CHECK(_zEnable,       GL_DEPTH_TEST);

    if (SHOULD_TOGGLE_2(_blendSrc, _blendDest))
        glBlendFuncSeparate(glBlendTable[newDescriptor._blendSrc], glBlendTable[newDescriptor._blendDest], GL_ONE, GL_ZERO);

    if (SHOULD_TOGGLE(_blendOp))
        glBlendEquation(glBlendOpTable[newDescriptor._blendOp]);
    
    if (SHOULD_TOGGLE(_cullMode))
        glCullFace(glCullModeTable[newDescriptor._cullMode]);
    
    if (SHOULD_TOGGLE(_fillMode))
        glPolygonMode(GL_FRONT_AND_BACK, glFillModeTable[newDescriptor._fillMode]);
    
    if (SHOULD_TOGGLE(_zFunc))
        glDepthFunc(glCompareFuncTable[newDescriptor._zFunc]);
    
    if (SHOULD_TOGGLE(_zWriteEnable))
        glDepthMask(newDescriptor._zWriteEnable ? GL_TRUE : GL_FALSE);
    
    if (SHOULD_TOGGLE(_stencilWriteMask))
        glStencilMask(newDescriptor._stencilWriteMask);
    
    if (SHOULD_TOGGLE_3(_stencilFunc, _stencilRef, _stencilMask))
        glStencilFunc(glCompareFuncTable[newDescriptor._stencilFunc], newDescriptor._stencilRef, newDescriptor._stencilMask);
    
    if (SHOULD_TOGGLE_3(_stencilFailOp, _stencilZFailOp, _stencilPassOp))
        glStencilOp(glStencilOpTable[newDescriptor._stencilFailOp], glStencilOpTable[newDescriptor._stencilZFailOp], glStencilOpTable[newDescriptor._stencilPassOp]);
    
    if (SHOULD_TOGGLE(_zBias)){
        if (IS_ZERO(newDescriptor._zBias)){
            glDisable(GL_POLYGON_OFFSET_FILL);
        }else {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(newDescriptor._zBias, newDescriptor._zUnits);
        }
    }

    if (SHOULD_TOGGLE_4(_colorWrite.b.b0, _colorWrite.b.b1, _colorWrite.b.b2, _colorWrite.b.b3)) {
        glColorMask(newDescriptor._colorWrite.b.b0 == GL_TRUE, // R
                    newDescriptor._colorWrite.b.b1 == GL_TRUE, // G
                    newDescriptor._colorWrite.b.b2 == GL_TRUE, // B
                    newDescriptor._colorWrite.b.b3 == GL_TRUE);// A
    }
}