//OpenGL state management: Lights, matrices, viewport, bound objects etc
#include "Headers/GLWrapper.h"
#include "Headers/glRenderStateBlock.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/Headers/Frustum.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBufferObject/Headers/glVertexArrayObject.h"
#include "Hardware/Video/OpenGL/Shaders/Headers/glUniformBufferObject.h"
#include <gtc/type_ptr.hpp>
#include <gtc/matrix_transform.hpp>

glShaderProgram* GL_API::_activeShaderProgram = NULL;
GLuint GL_API::_activeVAOId = 0;
GLuint GL_API::_activeTextureUnit = 0;
vec4<GLfloat> GL_API::_prevClearColor = DefaultColors::DIVIDE_BLUE();

bool GL_API::_anisotropySupported = false;
bool GL_API::_texCompressionSupported = false;
bool GL_API::_viewportForced = false;
bool GL_API::_viewportUpdateGL = false;
bool GL_API::_useMSAA = false;

void GL_API::clearStates(const bool skipShader,const bool skipTextures,const bool skipBuffers, const bool forceAll){
    if(!skipShader || forceAll) {
        setActiveProgram(NULL,forceAll);
    }

    if(!skipTextures || forceAll){
        setActiveTextureUnit(0,forceAll);
    }

    if(!skipBuffers || forceAll){
        setActiveVAO(0,forceAll);
        GLCheck(glBindBuffer(GL_ARRAY_BUFFER, 0));
        GLCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }

    GL_API::clearColor(DefaultColors::DIVIDE_BLUE());
}

void GL_API::updateStateInternal(RenderStateBlock* block, bool force){
    assert(block != NULL);

    glRenderStateBlock* glBlock = static_cast<glRenderStateBlock*>(block);
    glRenderStateBlock* glCurrent = NULL;
    if(!force){
        glCurrent = static_cast<glRenderStateBlock*>(GFX_DEVICE._currentStateBlock);
    }
    glBlock->activate(glCurrent);
   _currentGLRenderStateBlock = glBlock;
}

void GL_API::toggle2D(bool state){
    if(state == _2DRendering) return;

    _2DRendering = state;

    if(state){ //2D
        SET_STATE_BLOCK(_state2DRendering);
        Divide::GL::_matrixMode(PROJECTION_MATRIX);
        Divide::GL::_pushMatrix(); //1
        Divide::GL::_ortho(0,_cachedResolution.width,0,_cachedResolution.height,-1,1);
        Divide::GL::_matrixMode(VIEW_MATRIX);
        Divide::GL::_pushMatrix(); //2
        Divide::GL::_loadIdentity();
    }else{ //3D
        Divide::GL::_matrixMode(VIEW_MATRIX);
        Divide::GL::_popMatrix(); //2
        Divide::GL::_matrixMode(PROJECTION_MATRIX);
        Divide::GL::_popMatrix(); //1
        SET_PREVIOUS_STATE_BLOCK();
    }
}

///Update OpenGL light state
void GL_API::setLight(Light* const light){
    assert(light != NULL);

    LightProperties crtLight = light->getProperties();
    
    F32 lightType = crtLight._position.w;
	
    
    mat4<F32> viewMatrix;
	getMatrix(VIEW_MATRIX, viewMatrix);

    crtLight._position.set(viewMatrix * vec4<F32>(crtLight._position.xyz(), std::min(crtLight._position.w, 1.0f)));

    if(lightType < 0.5f){ //directional light
        crtLight._position.normalize();
    }else if(lightType > 1.5f){ //spot light
		F32 spotExponent = crtLight._direction.w;
	    crtLight._direction.set(viewMatrix * vec4<F32>(crtLight._direction.xyz(), 0.0f));
        crtLight._direction.normalize();
		crtLight._direction.w = spotExponent;
    }
    crtLight._position.w = lightType;
   
    _uniformBufferObjects[Lights_UBO]->ChangeSubData(light->getSlot() * sizeof(LightProperties),  // offset
                                                     sizeof(LightProperties),                     // size
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

void GL_API::lookAt(const vec3<GLfloat>& eye, const vec3<GLfloat>& target, const vec3<GLfloat>& up){
    vec3<GLfloat> viewDirection(eye-target);

    Divide::GL::_lookAt(glm::value_ptr(glm::lookAt(glm::vec3(eye.x, eye.y, eye.z),
                                                   glm::vec3(target.x, target.y, target.z),
                                                   glm::vec3(up.x, up.y, up.z))),
                        normalize(viewDirection));
}

void GL_API::getMatrix(const MATRIX_MODE& mode, mat4<GLfloat>& mat) {
    if(mode == VIEW_PROJECTION_MATRIX)          mat.set(_ViewProjectionCacheMatrix);
    else if(mode == VIEW_MATRIX)                mat.set(glm::value_ptr(Divide::GL::_viewMatrix.top()));
    else if(mode == PROJECTION_MATRIX)          mat.set(glm::value_ptr(Divide::GL::_projectionMatrix.top()));
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

    //This is such a cheap operation that no other checks are needed (even if we double set the VIEW_MATRIX)
    Divide::GL::_matrixMode(setCurrentMatrix);
}

//Setting ortho projection:
// -sets the current matrix to GL_PROJECTION
// -resets it to identity
// -sets an ortho perspective based on the input rect and limits
// -and sets the matrix mode back to GL_MODELVIEW
void GL_API::setOrthoProjection(const vec4<GLfloat>& rect, const vec2<GLfloat>& planes){
    Divide::GL::_ortho(rect.x, rect.y, rect.z, rect.w, planes.x, planes.y);
    Divide::GL::_matrixMode(VIEW_MATRIX);
}

//Setting perspective projection:
// -sets the current matrix to GL_PROJECTION
// -resets it to identity
// -sets a perspective projection based on the input FoV, aspect ration and limits
// -and sets the matrix mode back to GL_MODELVIEW
void GL_API::setPerspectiveProjection(GLfloat FoV,GLfloat aspectRatio, const vec2<GLfloat>& planes){
    Divide::GL::_perspective(FoV, aspectRatio, planes.x, planes.y);
    Divide::GL::_matrixMode(VIEW_MATRIX);
}

void GL_API::setAnaglyphFrustum(GLfloat camIOD, bool rightFrustum){
    ParamHandler& par = ParamHandler::getInstance();
    const vec2<GLfloat>& zPlanes = Frustum::getInstance().getZPlanes();
    GLfloat fov    = par.getParam<GLfloat>("runtime.verticalFOV");
    GLfloat ratio  = par.getParam<GLfloat>("runtime.aspectRatio");
    Divide::GL::_anaglyph(camIOD,(GLdouble)zPlanes.x, (GLdouble)zPlanes.y,ratio,fov,rightFrustum);
}

void GL_API::updateClipPlanes(){
    //ToDo: Hack because clearly, HW clipping is too complicated for some ATI cards ... -Ionut
    if(getGPUVendor() == GPU_VENDOR_AMD)
        return;

    const PlaneList& list = GFX_DEVICE.getClippingPlanes();
    bool clipPlaneState = false;
    for(GLuint clipPlaneIndex = 0; clipPlaneIndex < list.size(); clipPlaneIndex++){
        clipPlaneState = list[clipPlaneIndex].active();
        if(clipPlaneState != _activeClipPlanes[clipPlaneIndex]){
            _activeClipPlanes[clipPlaneIndex] = clipPlaneState;
            if(clipPlaneState){
                GLCheck(glEnable(GL_CLIP_DISTANCE0 + clipPlaneIndex));
            }else{
                GLCheck(glDisable(GL_CLIP_DISTANCE0 + clipPlaneIndex));
            }
        }
    }
}

void GL_API::setActiveTextureUnit(GLuint unit,const bool force){
    if(_activeTextureUnit == unit && !force)
        return; //< prevent double bind

    _activeTextureUnit = unit;
    GLCheck(glActiveTexture(GL_TEXTURE0 + unit));
}

void GL_API::setActiveVAO(GLuint id,const bool force){
    if(_activeVAOId == id && !force)
        return; //<prevent double bind

    _activeVAOId = id;
    GLCheck(glBindVertexArray(id));
}

void GL_API::setActiveProgram(glShaderProgram* const program,const bool force){
    GLuint newProgramId = (program != NULL) ? program->getId() : 0;
    GLuint oldProgramId = (_activeShaderProgram != NULL) ? _activeShaderProgram->getId() : 0;
    if(oldProgramId == newProgramId && !force)
        return; //<prevent double bind

    if(_activeShaderProgram != NULL) _activeShaderProgram->unbind(false);

    _activeShaderProgram = program;

    GLCheck(glUseProgram(newProgramId));
}

void GL_API::clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a, bool force){
    if(!FLOAT_COMPARE(_prevClearColor.r,r ) ||
       !FLOAT_COMPARE(_prevClearColor.g,g ) ||
       !FLOAT_COMPARE(_prevClearColor.b,b ) ||
       !FLOAT_COMPARE(_prevClearColor.a,a )){
           _prevClearColor.set(r,g,b,a);
            GLCheck(glClearColor(r,g,b,a));
    }
}

void GL_API::restoreViewport(){
	if(!_viewportUpdateGL)  return;

	if(!_viewportForced) Divide::GL::_viewport.pop(); //push / pop only if new viewport (not-forced)

	const vec4<GLuint>& prevViewport = Divide::GL::_viewport.top();
	GLCheck(glViewport(prevViewport.x, prevViewport.y, prevViewport.z, prevViewport.w));
	
}

vec4<GLuint> GL_API::setViewport(const vec4<GLuint>& viewport, bool force){
    _viewportUpdateGL = !viewport.compare(Divide::GL::_viewport.top());

	if(_viewportUpdateGL) {

		_viewportForced = force;
		if(!_viewportForced) Divide::GL::_viewport.push(viewport); //push / pop only if new viewport (not-forced)
		else                 Divide::GL::_viewport.top() = viewport;
		
		GLCheck(glViewport(viewport.x,viewport.y,viewport.z,viewport.w));
	}
	
    return Divide::GL::_viewport.top();
}