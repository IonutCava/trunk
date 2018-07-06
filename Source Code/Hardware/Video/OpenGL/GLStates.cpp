//OpenGL state management: Lights, matrices, viewport, bound objects etc
#include "Headers/GLWrapper.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Hardware/Video/OpenGL/Shaders/Headers/glUniformBufferObject.h"
#include <gtc/type_ptr.hpp>
#include <gtc/matrix_transform.hpp>

GLuint GL_API::_activeVAOId = 0;
GLuint GL_API::_activeFBId = 0;
GLuint GL_API::_activeVBId = 0;
GLuint GL_API::_activeTBId = 0;
GLuint GL_API::_activeTextureUnit = 0;

bool GL_API::_viewportForced = false;
bool GL_API::_viewportUpdateGL = false;
bool GL_API::_lastRestartIndexSmall = true;
bool GL_API::_primitiveRestartEnabled = false;

Unordered_map<GLuint, vec4<GLfloat> > GL_API::_prevClearColor;

void GL_API::clearStates(const bool skipShader,const bool skipTextures,const bool skipBuffers, const bool forceAll){
    if(!skipShader || forceAll) {
        setActiveProgram(nullptr,forceAll);
    }

    if(!skipTextures || forceAll){
        FOR_EACH(glTexture::textureBoundMapDef::value_type& it, glTexture::textureBoundMap){
            if(it.second.second != GL_NONE){
                setActiveTextureUnit(it.first);
                glSamplerObject::Unbind(it.first);
                glBindTexture(it.second.second, 0);
                glTexture::textureBoundMap[it.first] = std::make_pair(0, GL_NONE);
            }
        }
        setActiveTextureUnit(0,forceAll);
    }

    if(!skipBuffers || forceAll){
        setActiveVAO(0,forceAll);
        setActiveVB(0,forceAll);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glUniformBufferObject::unbind();
    }

    GL_API::clearColor(DefaultColors::DIVIDE_BLUE());
    
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

///Update OpenGL light state
void GL_API::setLight(Light* const light, bool shadowPass){
    assert(light != nullptr);
    if (shadowPass){
        _uniformBufferObjects[Shadow_UBO]->ChangeSubData(light->getSlot() * sizeof(LightShadowProperties), sizeof(LightShadowProperties), (GLvoid*)&light->getShadowProperties());
    }else{
        LightProperties temp = light->getProperties();
        mat4<F32> viewMat;
        getMatrix(VIEW_MATRIX, viewMat);
        if (light->getLightType() == LIGHT_TYPE_DIRECTIONAL){
            temp._position.set(vec3<F32>(viewMat * temp._position), temp._position.w);
        }else if (light->getLightType() == LIGHT_TYPE_SPOT){
            temp._direction.set(vec3<F32>(viewMat * temp._direction), temp._direction.w);
        }
        _uniformBufferObjects[Lights_UBO]->ChangeSubData(light->getSlot() * sizeof(LightProperties), sizeof(LightProperties), (GLvoid*)&temp);
    }
}

void GL_API::updateProjectionMatrix(){
    assert(Divide::GLUtil::_contextAvailable);
    const size_t mat4Size = 16 * sizeof(GLfloat);

    GLfloat matrixDataProjection[3 * 16];

    _ViewProjectionCacheMatrix.set(glm::value_ptr(Divide::GLUtil::_projectionMatrix.top() * Divide::GLUtil::_viewMatrix.top()));

    memcpy(matrixDataProjection, glm::value_ptr(Divide::GLUtil::_projectionMatrix.top()), mat4Size);
    memcpy(matrixDataProjection + 16, glm::value_ptr(Divide::GLUtil::_viewMatrix.top()), mat4Size);
    memcpy(matrixDataProjection + 32, _ViewProjectionCacheMatrix.mat, mat4Size);

    _uniformBufferObjects[Matrices_UBO]->ChangeSubData(0, 3 * mat4Size, matrixDataProjection);

    GFX_DEVICE.setProjectionDirty(true);
}

void GL_API::updateViewMatrix(){
    assert(Divide::GLUtil::_contextAvailable);
    const size_t mat4Size = 16 * sizeof(GLfloat);

    GLfloat matrixDataView[2 * 16];

    _ViewProjectionCacheMatrix.set(glm::value_ptr(Divide::GLUtil::_projectionMatrix.top() * Divide::GLUtil::_viewMatrix.top()));

    memcpy(matrixDataView, glm::value_ptr(Divide::GLUtil::_viewMatrix.top()), mat4Size);
    memcpy(matrixDataView + 16, _ViewProjectionCacheMatrix.mat, mat4Size);

    _uniformBufferObjects[Matrices_UBO]->ChangeSubData(mat4Size, 2 * mat4Size, matrixDataView);

    GFX_DEVICE.setViewDirty(true);
}

void GL_API::updateViewport(const vec4<GLint>& viewport){
    assert(Divide::GLUtil::_contextAvailable);
    const size_t vec4Size = 4 * sizeof(GLfloat);
    const size_t mat4Size = 16 * sizeof(GLfloat);

    _uniformBufferObjects[Matrices_UBO]->ChangeSubData(3 * mat4Size, vec4Size, &viewport[0]);
}

void GL_API::getMatrix(const MATRIX_MODE& mode, mat4<GLfloat>& mat) {
    if (mode == VIEW_PROJECTION_MATRIX)          mat.set(_ViewProjectionCacheMatrix);
    else if (mode == VIEW_MATRIX)                mat.set(glm::value_ptr(Divide::GLUtil::_viewMatrix.top()));
    else if (mode == VIEW_INV_MATRIX)            mat.set(glm::value_ptr(glm::inverse(Divide::GLUtil::_viewMatrix.top())));
    else if (mode == PROJECTION_MATRIX)          mat.set(glm::value_ptr(Divide::GLUtil::_projectionMatrix.top()));
    else if (mode == PROJECTION_INV_MATRIX)      mat.set(glm::value_ptr(glm::inverse(Divide::GLUtil::_projectionMatrix.top())));
    else if (mode == TEXTURE_MATRIX)             mat.set(glm::value_ptr(Divide::GLUtil::_textureMatrix.top()));
    else if(mode == VIEW_PROJECTION_INV_MATRIX) _ViewProjectionCacheMatrix.inverse(mat);
    else assert(mode == -1);
}

void GL_API::lockMatrices(const MATRIX_MODE& setCurrentMatrix, bool lockView, bool lockProjection){
    if(lockProjection){
        Divide::GLUtil::_matrixMode(PROJECTION_MATRIX);
        Divide::GLUtil::_pushMatrix();
    }

    if(lockView){
        Divide::GLUtil::_matrixMode(VIEW_MATRIX);
        Divide::GLUtil::_pushMatrix();
    }

    //This is such a cheap operation that no other checks are needed (even if we double set the VIEW_MATRIX)
    Divide::GLUtil::_matrixMode(setCurrentMatrix);
}

void GL_API::releaseMatrices( const MATRIX_MODE& setCurrentMatrix, bool releaseView, bool releaseProjection){
    if(releaseProjection){
        Divide::GLUtil::_matrixMode(PROJECTION_MATRIX);
        Divide::GLUtil::_popMatrix();
    }

    if(releaseView){
        Divide::GLUtil::_matrixMode(VIEW_MATRIX);
        Divide::GLUtil::_popMatrix();
    }

    //This is such a cheap operation that no other checks are needed (even if we double set the VIEW_MATRIX)
    Divide::GLUtil::_matrixMode(setCurrentMatrix);
}

void GL_API::updateClipPlanes(){
    GLuint clipPlaneIndex = 0;
    bool   clipPlaneActive = false;

    const PlaneList& list = GFX_DEVICE.getClippingPlanes();
    
    for (const Plane<F32>& clipPlane : list){
        clipPlaneIndex  = clipPlane.getIndex();
        clipPlaneActive = clipPlane.active();

        if (_activeClipPlanes[clipPlaneIndex] != clipPlaneActive){
            _activeClipPlanes[clipPlaneIndex]  = clipPlaneActive;
            clipPlaneActive ? glEnable(GL_CLIP_DISTANCE0 + clipPlaneIndex) : glDisable(GL_CLIP_DISTANCE0 + clipPlaneIndex);
        }
    }
}

bool GL_API::setActiveTextureUnit(GLuint unit,const bool force){
    if(_activeTextureUnit == unit && !force)
        return false; //< prevent double bind

    _activeTextureUnit = unit;
    glActiveTexture(GL_TEXTURE0 + unit);

    return true;
}

bool GL_API::setActiveFB(GLuint id, const bool read, const bool write, const bool force){
    if (_activeFBId == id && !force)
        return false; //<prevent double bind

    _activeFBId = id;
    if (read && write)      glBindFramebuffer(GL_FRAMEBUFFER, id);
    else if(read && !write) glBindFramebuffer(GL_READ_FRAMEBUFFER, id);
    else                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);

    return true;
}

bool GL_API::setActiveVAO(GLuint id, const bool force){
    if(_activeVAOId == id && !force)
        return false; //<prevent double bind
        
    _activeVAOId = id;
    glBindVertexArray(id);

    return true;
}

bool GL_API::setActiveTB(GLuint id, const bool force){
    if (_activeTBId == id && !force)
        return false; //<prevent double bind

    _activeTBId = id;
    glBindBuffer(GL_TEXTURE_BUFFER, id);

    return true;
}

bool GL_API::setActiveVB(GLuint id,const bool force){
    if(_activeVBId == id && !force)
        return false; //<prevent double bind
        
    _activeVBId = id;
    glBindBuffer(GL_ARRAY_BUFFER, id);

    return true;
}

bool GL_API::setActiveProgram(glShaderProgram* const program,const bool force){
    GLuint newProgramId = (program != nullptr) ? program->getId() : 0;
    GLuint oldProgramId = (GFXDevice::_activeShaderProgram != nullptr) ? GFXDevice::_activeShaderProgram->getId() : 0;
    if(oldProgramId == newProgramId && !force)
        return false; //<prevent double bind

    if (GFXDevice::_activeShaderProgram != nullptr) GFXDevice::_activeShaderProgram->unbind(false);

    GFXDevice::_activeShaderProgram = program;

    glUseProgram(newProgramId);
    return true;
}

void GL_API::clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLuint renderTarget, bool force){
    vec4<F32>& prevClearColor = _prevClearColor[renderTarget];
    if (prevClearColor != vec4<F32>(r,g,b,a)){
        prevClearColor.set(r, g, b, a);
        glClearColor(r,g,b,a);
    }
}

void GL_API::restoreViewport(){
    if(!_viewportUpdateGL)  return;

    if(!_viewportForced) GFX_DEVICE._viewport.pop(); //push / pop only if new viewport (not-forced)

    const vec4<GLint>& prevViewport = GFX_DEVICE._viewport.top();
    glViewport(prevViewport.x, prevViewport.y, prevViewport.z, prevViewport.w);
    GL_API::getInstance().updateViewport(prevViewport);
}

vec4<GLint> GL_API::setViewport(const vec4<GLint>& viewport, bool force){
    _viewportUpdateGL = !viewport.compare(GFX_DEVICE._viewport.top());

    if(_viewportUpdateGL) {

        _viewportForced = force;
        if (!_viewportForced) GFX_DEVICE._viewport.push(viewport); //push / pop only if new viewport (not-forced)
        else                  GFX_DEVICE._viewport.top() = viewport;
        
        glViewport(viewport.x,viewport.y,viewport.z,viewport.w);
        GL_API::getInstance().updateViewport(viewport);
    }
    
    return viewport;
}

GLfloat* GL_API::lookAt(const mat4<GLfloat>& viewMatrix) {
    return Divide::GLUtil::_lookAt(viewMatrix.mat); 
}

//Setting ortho projection:
GLfloat* GL_API::setProjection(const vec4<GLfloat>& rect, const vec2<GLfloat>& planes) {
    return Divide::GLUtil::_ortho(rect.x, rect.y, rect.z, rect.w, planes.x, planes.y);
}

//Setting perspective projection:
GLfloat* GL_API::setProjection(GLfloat FoV, GLfloat aspectRatio, const vec2<GLfloat>& planes) {
    return Divide::GLUtil::_perspective(FoV, aspectRatio, planes.x, planes.y);
}

//Setting anaglyph frustum for specified eye
void GL_API::setAnaglyphFrustum(GLfloat camIOD, const vec2<F32>& zPlanes, F32 aspectRatio, F32 verticalFoV, bool rightFrustum) {
    Divide::GLUtil::_anaglyph(camIOD, (GLdouble)zPlanes.x, (GLdouble)zPlanes.y, aspectRatio, verticalFoV, rightFrustum);
}

#ifndef SHOULD_TOGGLE
    #define SHOULD_TOGGLE(state) (!oldBlock || oldBlock->getDescriptor().state != newDescriptor.state)
    #define SHOULD_TOGGLE_2(state1, state2)  (!oldBlock ||SHOULD_TOGGLE(state1) || SHOULD_TOGGLE(state2))
    #define SHOULD_TOGGLE_3(state1, state2, state3) (!oldBlock ||SHOULD_TOGGLE_2(state1, state2) || SHOULD_TOGGLE(state3))
    #define SHOULD_TOGGLE_4(state1, state2, state3, state4) (!oldBlock || SHOULD_TOGGLE_3(state1, state2, state3) || SHOULD_TOGGLE(state4))
    #define TOGGLE_NO_CHECK(state, enumValue) newDescriptor.state ? glEnable(enumValue) : glDisable(enumValue)
    #define TOGGLE_WITH_CHECK(state, enumValue) if(SHOULD_TOGGLE(state)) TOGGLE_NO_CHECK(state, enumValue);
#endif

void GL_API::activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock){
    // ------------------- PASS INDEPENDENT STATES -------------------------------------- //
    const RenderStateBlockDescriptor& newDescriptor = newBlock.getDescriptor();

    TOGGLE_WITH_CHECK(_blendEnable,   GL_BLEND);
    TOGGLE_WITH_CHECK(_cullMode,      GL_CULL_FACE);
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
        glDepthMask(newDescriptor._zWriteEnable);
    
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