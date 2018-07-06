//OpenGL state management: Lights, matrices, viewport, bound objects etc
#include "Headers/GLWrapper.h"
#include "Headers/glRenderStateBlock.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBufferObject/Headers/glVertexArrayObject.h"
#include "Hardware/Video/OpenGL/Shaders/Headers/glUniformBufferObject.h"
#include <gtc/type_ptr.hpp>
#include <gtc/matrix_transform.hpp>

glShaderProgram* GL_API::_activeShaderProgram = NULL;
GLuint GL_API::_activeVAOId = 0;
GLuint GL_API::_activeTextureUnit = 0;
vec4<GLint> GL_API::_prevViewportRect = vec4<GLint>(0,0,0,0);
vec4<GLint> GL_API::_currentviewportRect = vec4<GLint>(0,0,0,0);
vec4<GLfloat> GL_API::_prevClearColor = DIVIDE_BLUE();

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

	GL_API::clearColor(DIVIDE_BLUE());
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

void GL_API::getMatrix(const MATRIX_MODE& mode, mat4<GLfloat>& mat){
    glm::mat4 matrix;
    Divide::GL::_queryMatrix(mode,matrix);
    mat = mat4<GLfloat>(glm::value_ptr(matrix));
}

void GL_API::getMatrix(const EXTENDED_MATRIX& mode, mat4<GLfloat>& mat){
    assert(mode != NORMAL_MATRIX /* || mode != ...*/);
    glm::mat4 matrix;
    Divide::GL::_queryMatrix(mode,matrix);
    mat = mat4<GLfloat>(glm::value_ptr(matrix));
}

void GL_API::getMatrix(const EXTENDED_MATRIX& mode, mat3<GLfloat>& mat){
    assert(mode == NORMAL_MATRIX /* || mode = ...*/);
    glm::mat3 matrix;
    Divide::GL::_queryMatrix(NORMAL_MATRIX,matrix);
    mat = mat3<GLfloat>(glm::value_ptr(matrix));
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
	GLubyte slot = light->getSlot();

    if(!light->getEnabled()){
      return;
    }
	
	U32 offset = slot * sizeof(light->getProperties());
	_uniformBufferObjects[Lights_UBO]->ChangeSubData(offset, 
												     sizeof(light->getProperties()),
													 (const GLvoid*)(&(light->getProperties())));

	LightType type = light->getLightType();

	glm::vec4 position(light->getPosition().x, 
					   light->getPosition().y, 
					   light->getPosition().z,
					   (type == LIGHT_TYPE_DIRECTIONAL) ? 0.0f : 1.0f);

	GLCheck(glLightfv(GL_LIGHT0+slot, GL_POSITION, &position.x));

	GLCheck(glLightfv(GL_LIGHT0+slot, GL_AMBIENT,  light->getVProperty(LIGHT_PROPERTY_AMBIENT)));
	GLCheck(glLightfv(GL_LIGHT0+slot, GL_DIFFUSE,  light->getVProperty(LIGHT_PROPERTY_DIFFUSE)));
	GLCheck(glLightfv(GL_LIGHT0+slot, GL_SPECULAR, light->getVProperty(LIGHT_PROPERTY_SPECULAR)));
	if(type == LIGHT_TYPE_SPOT){
		GLCheck(glLightfv(GL_LIGHT0+slot, GL_SPOT_DIRECTION, light->getDirection()));
		GLCheck(glLightf(GL_LIGHT0+slot, GL_SPOT_EXPONENT,light->getFProperty(LIGHT_PROPERTY_SPOT_EXPONENT)));
		GLCheck(glLightf(GL_LIGHT0+slot, GL_SPOT_CUTOFF, light->getFProperty(LIGHT_PROPERTY_SPOT_CUTOFF)));
	}
	if(type != LIGHT_TYPE_DIRECTIONAL){
		GLCheck(glLightf(GL_LIGHT0+slot, GL_CONSTANT_ATTENUATION,light->getFProperty(LIGHT_PROPERTY_CONST_ATT)));
		GLCheck(glLightf(GL_LIGHT0+slot, GL_LINEAR_ATTENUATION,light->getFProperty(LIGHT_PROPERTY_LIN_ATT)));
		GLCheck(glLightf(GL_LIGHT0+slot, GL_QUADRATIC_ATTENUATION,light->getFProperty(LIGHT_PROPERTY_QUAD_ATT)));
	}

}

void GL_API::updateProjectionMatrix(){
	if(!Divide::GL::_contextAvailable) return;
	_uniformBufferObjects[Matrices_UBO]->ChangeSubData(0,sizeof(glm::mat4),glm::value_ptr(Divide::GL::_projectionMatrix.top()));
}

void GL_API::updateViewMatrix(){
	if(!Divide::GL::_contextAvailable) return;
	_uniformBufferObjects[Matrices_UBO]->ChangeSubData(sizeof(glm::mat4),sizeof(glm::mat4),glm::value_ptr(Divide::GL::_viewMatrix.top()));
}

//Setting _LookAt here for camera's or shadow projection
// -set the current matrix to GL_MODELVIEW
// -reset it to identity
void GL_API::lookAt(const vec3<GLfloat>& eye,
                    const vec3<GLfloat>& center,
                    const vec3<GLfloat>& up,
                    const bool invertx,
                    const bool inverty){
    Divide::GL::_LookAt(glm::vec3(eye.x,eye.y,eye.z),
						glm::vec3(center.x,center.y,center.z),
						glm::vec3(up.x,up.y,up.z),
						invertx,
						inverty);
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

    ///This is such a cheap operation that no other checks are needed (even if we double set the VIEW_MATRIX)
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

    ///This is such a cheap operation that no other checks are needed (even if we double set the VIEW_MATRIX)
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

void GL_API::setActiveTextureUnit(GLuint unit,const bool force){
	if(_activeTextureUnit == unit && !force) return; //< prevent double bind
    _activeTextureUnit = unit;
	GLCheck(glActiveTexture(GL_TEXTURE0 + unit));
}

void GL_API::setActiveVAO(GLuint id,const bool force){
	if(_activeVAOId == id && !force) return; //<prevent double bind
    _activeVAOId = id;
    GLCheck(glBindVertexArray(id));
}

void GL_API::setActiveProgram(glShaderProgram* const program,const bool force){
	U32 newProgramId = (program != NULL) ? program->getId() : 0;
	U32 oldProgramId = (_activeShaderProgram != NULL) ? _activeShaderProgram->getId() : 0;
	if(oldProgramId == newProgramId && !force) return; //<prevent double bind

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
    if(_currentviewportRect != _prevViewportRect){
        _currentviewportRect = _prevViewportRect;
        GLCheck(glViewport(_currentviewportRect.x,_currentviewportRect.y,_currentviewportRect.z,_currentviewportRect.w));
    }
}

void GL_API::setViewport(GLint x, GLint y, GLint width, GLint height,bool force){
    vec4<GLint> viewportRect(x,y,width,height);
     if(_currentviewportRect != viewportRect){
        _prevViewportRect = force ? viewportRect : _currentviewportRect;
        _currentviewportRect = viewportRect;
        GLCheck(glViewport(_currentviewportRect.x,_currentviewportRect.y,_currentviewportRect.z,_currentviewportRect.w));
     }
}