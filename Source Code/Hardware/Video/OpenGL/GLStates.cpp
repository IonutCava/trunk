//OpenGL state management: Lights, matrices, viewport, bound objects etc
#include "Headers/GLWrapper.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/Headers/Frustum.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Hardware/Video/OpenGL/Shaders/Headers/glUniformBufferObject.h"
#include <gtc/type_ptr.hpp>
#include <gtc/matrix_transform.hpp>



glShaderProgram* GL_API::_activeShaderProgram = nullptr;
GLuint GL_API::_activeVAOId = 0;
GLuint GL_API::_activeFBId = 0;
GLuint GL_API::_activeVBId = 0;
GLuint GL_API::_activeTextureUnit = 0;
Unordered_map<GLuint, vec4<GLfloat> > GL_API::_prevClearColor;

bool GL_API::_viewportForced = false;
bool GL_API::_viewportUpdateGL = false;

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

void GL_API::toggle2D(bool state){
    if(state == _2DRendering) return;
    static RenderStateBlock* previousStateBlock = nullptr;

    _2DRendering = state;

    if(state){ //2D
        previousStateBlock = SET_STATE_BLOCK(_state2DRendering);
        Divide::GL::_matrixMode(PROJECTION_MATRIX);
        Divide::GL::_pushMatrix(); //1
        Divide::GL::_ortho(0,_cachedResolution.width,0,_cachedResolution.height,-1,1);
        Divide::GL::_matrixMode(VIEW_MATRIX);
        Divide::GL::_pushMatrix(); //2
        Divide::GL::_loadIdentity();
    }else{ //3D
        assert(previousStateBlock != nullptr);

        Divide::GL::_matrixMode(VIEW_MATRIX);
        Divide::GL::_popMatrix(); //2
        Divide::GL::_matrixMode(PROJECTION_MATRIX);
        Divide::GL::_popMatrix(); //1
        SET_STATE_BLOCK(previousStateBlock);
    }
}

///Update OpenGL light state
void GL_API::setLight(Light* const light, bool shadowPass){
    assert(light != nullptr);
    if (shadowPass){
        const LightShadowProperties& crtShadow = light->getShadowProperties();
        GLsizei sizeOfBlock = sizeof(LightShadowProperties);
        _uniformBufferObjects[Shadow_UBO]->ChangeSubData(light->getSlot() * sizeOfBlock, sizeOfBlock, (const GLvoid*)(&crtShadow));
        return;
    }

    LightProperties crtLight = light->getProperties();
    
    F32 lightType = crtLight._position.w;
    
    
    const mat4<F32>& viewMatrix = GFX_DEVICE.getMatrix(VIEW_MATRIX);

    crtLight._position.set(viewMatrix * vec4<F32>(crtLight._position.xyz(), std::min(crtLight._position.w, 1.0f)));

    if (light->getType() == LIGHT_TYPE_DIRECTIONAL){
        crtLight._position.normalize();
    }else if (light->getType() == LIGHT_TYPE_SPOT){
        F32 spotExponent = crtLight._direction.w;
        crtLight._direction.w = 0.0f;
        crtLight._direction.set(viewMatrix * crtLight._direction);
        crtLight._direction.normalize();
        crtLight._direction.w = spotExponent;
    }
    crtLight._position.w = lightType;
   
    GLsizei sizeOfBlock = sizeof(LightProperties);
    //GLsizei padding = 12 * sizeof(F32);
    _uniformBufferObjects[Lights_UBO]->ChangeSubData(light->getSlot() * (sizeOfBlock/* + padding*/),  // offset
                                                     sizeOfBlock,                     // size
                                                     (const GLvoid*)(&crtLight)); // data

   
}

const size_t mat4Size = 16 * sizeof(GLfloat);
GLfloat matrixDataView[2 * 16];
GLfloat matrixDataProjection[3 * 16];

void GL_API::updateProjectionMatrix(){
    if(!Divide::GL::_contextAvailable) return;
    _ViewProjectionCacheMatrix.set(glm::value_ptr(Divide::GL::_projectionMatrix.top() * Divide::GL::_viewMatrix.top()));

    memcpy(matrixDataProjection, glm::value_ptr(Divide::GL::_projectionMatrix.top()), mat4Size);
    memcpy(matrixDataProjection + 16, glm::value_ptr(Divide::GL::_viewMatrix.top()), mat4Size);
    memcpy(matrixDataProjection + 32, _ViewProjectionCacheMatrix.mat, mat4Size);

    _uniformBufferObjects[Matrices_UBO]->ChangeSubData(0, 3 * mat4Size, matrixDataProjection);
    GFX_DEVICE.setProjectionDirty(true);
}

void GL_API::updateViewMatrix(){
    if(!Divide::GL::_contextAvailable) return;
    _ViewProjectionCacheMatrix.set(glm::value_ptr(Divide::GL::_projectionMatrix.top() * Divide::GL::_viewMatrix.top()));

    memcpy(matrixDataView, glm::value_ptr(Divide::GL::_viewMatrix.top()), mat4Size);
    memcpy(matrixDataView + 16, _ViewProjectionCacheMatrix.mat, mat4Size);

    _uniformBufferObjects[Matrices_UBO]->ChangeSubData(mat4Size, 2 * mat4Size, matrixDataView);

    GFX_DEVICE.setViewDirty(true);
}

//Setting _LookAt here for camera's or shadow projection
// -set the current matrix to GL_MODELVIEW
// -reset it to identity

GLfloat* GL_API::lookAt(const vec3<GLfloat>& eye, const vec3<GLfloat>& target, const vec3<GLfloat>& up){
    vec3<GLfloat> viewDirection(eye-target);

    return Divide::GL::_lookAt(glm::value_ptr(glm::lookAt(glm::vec3(eye.x, eye.y, eye.z),
                                                          glm::vec3(target.x, target.y, target.z),
                                                          glm::vec3(up.x, up.y, up.z))),
                               normalize(viewDirection));
}

GLfloat* GL_API::lookAt(const mat4<GLfloat>& viewMatrix, const vec3<GLfloat>& viewDirection) {
    return Divide::GL::_lookAt(viewMatrix.mat, viewDirection);
}

const GLfloat* GL_API::getLookAt(const vec3<GLfloat>& eye, const vec3<GLfloat>& target, const vec3<GLfloat>& up) {
    return glm::value_ptr(glm::lookAt(glm::vec3(eye.x, eye.y, eye.z),
                                      glm::vec3(target.x, target.y, target.z),
                                      glm::vec3(up.x, up.y, up.z)));
}

void GL_API::getMatrix(const MATRIX_MODE& mode, mat4<GLfloat>& mat) {
    if(mode == VIEW_PROJECTION_MATRIX)          mat.set(_ViewProjectionCacheMatrix);
    else if(mode == VIEW_MATRIX)                mat.set(glm::value_ptr(Divide::GL::_viewMatrix.top()));
    else if(mode == VIEW_INV_MATRIX)            mat.set(glm::value_ptr(glm::inverse(Divide::GL::_viewMatrix.top())));
    else if(mode == PROJECTION_MATRIX)          mat.set(glm::value_ptr(Divide::GL::_projectionMatrix.top()));
    else if(mode == PROJECTION_INV_MATRIX)      mat.set(glm::value_ptr(glm::inverse(Divide::GL::_projectionMatrix.top())));
    else if(mode == TEXTURE_MATRIX)             mat.set(glm::value_ptr(Divide::GL::_textureMatrix.top()));
    else if(mode == VIEW_PROJECTION_INV_MATRIX) _ViewProjectionCacheMatrix.inverse(mat);
    else assert(mode == -1);
}

void GL_API::lockMatrices(const MATRIX_MODE& setCurrentMatrix, bool lockView, bool lockProjection){
    if(lockProjection){
        Divide::GL::_matrixMode(PROJECTION_MATRIX);
        Divide::GL::_pushMatrix();
    }

    if(lockView){
        Divide::GL::_matrixMode(VIEW_MATRIX);
        Divide::GL::_pushMatrix();
    }

    Frustum::getInstance().lock();

    //This is such a cheap operation that no other checks are needed (even if we double set the VIEW_MATRIX)
    Divide::GL::_matrixMode(setCurrentMatrix);
}

void GL_API::releaseMatrices( const MATRIX_MODE& setCurrentMatrix, bool releaseView, bool releaseProjection){
    if(releaseProjection){
        Divide::GL::_matrixMode(PROJECTION_MATRIX);
        Divide::GL::_popMatrix();
    }

    if(releaseView){
        Divide::GL::_matrixMode(VIEW_MATRIX);
        Divide::GL::_popMatrix();
    }

    Frustum::getInstance().unlock();

    //This is such a cheap operation that no other checks are needed (even if we double set the VIEW_MATRIX)
    Divide::GL::_matrixMode(setCurrentMatrix);
}

//Setting ortho projection:
// -sets the current matrix to GL_PROJECTION
// -resets it to identity
// -sets an ortho perspective based on the input rect and limits
// -and sets the matrix mode back to GL_MODELVIEW
GLfloat* GL_API::setOrthoProjection(const vec4<GLfloat>& rect, const vec2<GLfloat>& planes){
    Frustum::getInstance().setOrtho(rect, planes);
    Divide::GL::_matrixMode(VIEW_MATRIX);
    return Divide::GL::_ortho(rect.x, rect.y, rect.z, rect.w, planes.x, planes.y);
}

const GLfloat* GL_API::getOrthoProjection(const vec4<GLfloat>& rect, const vec2<GLfloat>& planes){
    return glm::value_ptr(glm::ortho(rect.x, rect.y, rect.z, rect.w, planes.x, planes.y));
}

//Setting perspective projection:
// -sets the current matrix to GL_PROJECTION
// -resets it to identity
// -sets a perspective projection based on the input FoV, aspect ration and limits
// -and sets the matrix mode back to GL_MODELVIEW
GLfloat* GL_API::setPerspectiveProjection(GLfloat FoV,GLfloat aspectRatio, const vec2<GLfloat>& planes){
    Divide::GL::_matrixMode(VIEW_MATRIX);
    Frustum::getInstance().setProjection(aspectRatio, FoV, planes, false);
    return Divide::GL::_perspective(FoV, aspectRatio, planes.x, planes.y);
}

void GL_API::setAnaglyphFrustum(GLfloat camIOD, bool rightFrustum){
    Frustum& frust = Frustum::getInstance();
    const vec2<GLfloat>& zPlanes = frust.getZPlanes();
    Divide::GL::_anaglyph(camIOD, (GLdouble)zPlanes.x, (GLdouble)zPlanes.y, frust.getAspectRatio(), frust.getVerticalFoV(), rightFrustum);
}

void GL_API::updateClipPlanes(){
    const PlaneList& list = GFX_DEVICE.getClippingPlanes();
    bool clipPlaneState = false;
    for(GLuint clipPlaneIndex = 0; clipPlaneIndex < list.size(); clipPlaneIndex++){
        clipPlaneState = list[clipPlaneIndex].active();
        if(clipPlaneState != _activeClipPlanes[clipPlaneIndex]){
            _activeClipPlanes[clipPlaneIndex] = clipPlaneState;
            if(clipPlaneState){
                glEnable(GL_CLIP_DISTANCE0 + clipPlaneIndex);
            }else{
                glDisable(GL_CLIP_DISTANCE0 + clipPlaneIndex);
            }
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
    if (read && write)
        glBindFramebuffer(GL_FRAMEBUFFER, id);
    else if(read && !write)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, id);
    else
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);

    return true;
}

bool GL_API::setActiveVAO(GLuint id, const bool force){
    if(_activeVAOId == id && !force)
        return false; //<prevent double bind
        
    _activeVAOId = id;
    glBindVertexArray(id);

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
    GLuint oldProgramId = (_activeShaderProgram != nullptr) ? _activeShaderProgram->getId() : 0;
    if(oldProgramId == newProgramId && !force)
        return false; //<prevent double bind

    if(_activeShaderProgram != nullptr) _activeShaderProgram->unbind(false);

    _activeShaderProgram = program;

    glUseProgram(newProgramId);
    return true;
}

void GL_API::clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLuint renderTarget, bool force){
    vec4<F32>& prevClearColor = _prevClearColor[renderTarget];
    if (!FLOAT_COMPARE(prevClearColor.r, r) || !FLOAT_COMPARE(prevClearColor.g, g) || !FLOAT_COMPARE(prevClearColor.b, b) || !FLOAT_COMPARE(prevClearColor.a, a)){
        prevClearColor.set(r, g, b, a);
        glClearColor(r,g,b,a);
    }
}

void GL_API::restoreViewport(){
    if(!_viewportUpdateGL)  return;

    if(!_viewportForced) Divide::GL::_viewport.pop(); //push / pop only if new viewport (not-forced)

    const vec4<GLint>& prevViewport = Divide::GL::_viewport.top();
    glViewport(prevViewport.x, prevViewport.y, prevViewport.z, prevViewport.w);
    
}

vec4<GLint> GL_API::setViewport(const vec4<GLint>& viewport, bool force){
    _viewportUpdateGL = !viewport.compare(Divide::GL::_viewport.top());

    if(_viewportUpdateGL) {

        _viewportForced = force;
        if(!_viewportForced) Divide::GL::_viewport.push(viewport); //push / pop only if new viewport (not-forced)
        else                 Divide::GL::_viewport.top() = viewport;
        
        glViewport(viewport.x,viewport.y,viewport.z,viewport.w);
    }
    
    return Divide::GL::_viewport.top();
}

#define SHOULD_TOGGLE(state) (!oldBlock || oldBlock->getDescriptorConst().state != newBlock.getDescriptorConst().state)

#define TOGGLE_NO_CHECK(state, enumValue) newBlock.getDescriptorConst().state ? glEnable(enumValue) : glDisable(enumValue);

#define TOGGLE_WITH_CHECK(state, enumValue) if(!oldBlock || oldBlock->getDescriptorConst().state != newBlock.getDescriptorConst().state) { \
                                                newBlock.getDescriptorConst().state ? glEnable(enumValue) : glDisable(enumValue); \
                                            }
void GL_API::activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock){
    // ------------------- PASS INDEPENDENT STATES -------------------------------------- //

    TOGGLE_WITH_CHECK(_blendEnable, GL_BLEND);
    TOGGLE_WITH_CHECK(_cullMode, GL_CULL_FACE);

    if (SHOULD_TOGGLE(_blendSrc) || SHOULD_TOGGLE(_blendDest)){
        glBlendFuncSeparate(glBlendTable[newBlock.getDescriptorConst()._blendSrc], glBlendTable[newBlock.getDescriptorConst()._blendDest], GL_ONE, GL_ZERO);
    }

    if (SHOULD_TOGGLE(_blendOp)){
        glBlendEquation(glBlendOpTable[newBlock.getDescriptorConst()._blendOp]);
    }

    if (SHOULD_TOGGLE(_cullMode)) {
        glCullFace(glCullModeTable[newBlock.getDescriptorConst()._cullMode]);
    }

    if (SHOULD_TOGGLE(_zBias)){
        if (IS_ZERO(newBlock.getDescriptorConst()._zBias)){
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
        else {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(newBlock.getDescriptorConst()._zBias, newBlock.getDescriptorConst()._zUnits);
        }
    }

    if (SHOULD_TOGGLE(_fillMode)){
        glPolygonMode(GL_FRONT_AND_BACK, glFillModeTable[newBlock.getDescriptorConst()._fillMode]);
    }

    // ------------------- DEPTH PASS DEPENDENT STATES -------------------------------------- //

    //if(!GFX_DEVICE.isDepthPrePass()) {
    TOGGLE_WITH_CHECK(_stencilEnable, GL_STENCIL_TEST);

    TOGGLE_WITH_CHECK(_zEnable, GL_DEPTH_TEST);

    if (SHOULD_TOGGLE(_writeRedChannel) || SHOULD_TOGGLE(_writeBlueChannel) || 
        SHOULD_TOGGLE(_writeGreenChannel) || SHOULD_TOGGLE(_writeAlphaChannel)) {
        glColorMask(newBlock.getDescriptorConst()._writeRedChannel,
                    newBlock.getDescriptorConst()._writeBlueChannel,
                    newBlock.getDescriptorConst()._writeGreenChannel,
                    newBlock.getDescriptorConst()._writeAlphaChannel);
    }

    if (SHOULD_TOGGLE(_zFunc)){
        glDepthFunc(glCompareFuncTable[newBlock.getDescriptorConst()._zFunc]);
    }
    if (SHOULD_TOGGLE(_zWriteEnable)){
        glDepthMask(newBlock.getDescriptorConst()._zWriteEnable);
    }


    if (SHOULD_TOGGLE(_stencilFunc) || SHOULD_TOGGLE(_stencilRef) || SHOULD_TOGGLE(_stencilMask)) {
        glStencilFunc(glCompareFuncTable[newBlock.getDescriptorConst()._stencilFunc],
                                         newBlock.getDescriptorConst()._stencilRef,
                                         newBlock.getDescriptorConst()._stencilMask);
    }

    if (SHOULD_TOGGLE(_stencilFailOp) || SHOULD_TOGGLE(_stencilZFailOp) || SHOULD_TOGGLE(_stencilPassOp)) {
        glStencilOp(glStencilOpTable[newBlock.getDescriptorConst()._stencilFailOp],
                    glStencilOpTable[newBlock.getDescriptorConst()._stencilZFailOp],
                    glStencilOpTable[newBlock.getDescriptorConst()._stencilPassOp]);
    }

    if (SHOULD_TOGGLE(_stencilWriteMask)){
        glStencilMask(newBlock.getDescriptorConst()._stencilWriteMask);
    }
    //}
}