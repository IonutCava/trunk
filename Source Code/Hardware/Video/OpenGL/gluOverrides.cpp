#include "Headers/glResources.h"
#ifndef GLM_MESSAGES
#undef  GLM_MESSAGES
#endif
#include <gtc/matrix_inverse.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"

namespace Divide {
	namespace GL {
/*-----------Context Management----*/
	bool _applicationClosing = false;
	bool _contextAvailable = false;
	bool _useDebugOutputCallback = false;
	GLFWwindow* _mainWindow     = NULL;
	GLFWwindow* _loaderWindow   = NULL;
/*----------- GLU overrides ------*/
    ///Matrix management
	MATRIX_MODE  _currentMatrixMode;
	matrixStack  _viewMatrix;
	matrixStack  _projectionMatrix;
    matrixStack  _textureMatrix;
    glm::mat4    _MVPCachedMatrix;
    glm::mat4    _MVCachedMatrix;
	glm::mat4    _identityMatrix;
    glm::mat4    _biasMatrix;
    glm::mat4    _modelMatrix;
	vector3Stack _currentEyeVector;
	vector3Stack _currentLookAtVector;

    bool _MDirty = true;
	bool _VDirty = true;
	bool _PDirty = true;
    bool _isUniformScaled = true;
	bool _resetModelMatrixFlag = true;

	void _matrixMode(const MATRIX_MODE& mode) {
		_currentMatrixMode = mode;
	}

    void _initStacks(){
		_currentEyeVector.push(glm::vec3());
		_currentLookAtVector.push(glm::vec3());
        _projectionMatrix.push(_identityMatrix);
        _viewMatrix.push(_identityMatrix);
        _textureMatrix.push(_identityMatrix);
        _biasMatrix = glm::mat4(0.5, 0.0, 0.0, 0.0,
		                        0.0, 0.5, 0.0, 0.0,
		                        0.0, 0.0, 0.5, 0.0,
		                        0.5, 0.5, 0.5, 1.0);
    }

    void _LookAt(const glm::vec3& eye,const glm::vec3& lookAt,const glm::vec3& up, bool invertx, bool inverty){
       _matrixMode(VIEW_MATRIX);
	   _currentEyeVector.top() = eye;
	   _currentLookAtVector.top() = lookAt;
       _viewMatrix.top() = glm::lookAt(_currentEyeVector.top(),_currentLookAtVector.top(),up);
       if(invertx) glm::scale(_viewMatrix.top(),glm::vec3(-1,1,1));
	   GL_API::getInstance().updateViewMatrix();
	   _VDirty = true;
    }

    void _ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar){
        _matrixMode(PROJECTION_MATRIX);
        _projectionMatrix.top() = glm::ortho(left,right,bottom,top,zNear,zFar);
		GL_API::getInstance().updateProjectionMatrix();
        _PDirty = true;
    }

    void _perspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar){
        _matrixMode(PROJECTION_MATRIX);
        _projectionMatrix.top() = glm::perspective(fovy,aspect,zNear,zFar);
		GL_API::getInstance().updateProjectionMatrix();
        _PDirty = true;
    }

	void _resetModelMatrix(bool force){
		_resetModelMatrixFlag = !force;

		if(force){
			_modelMatrix = _identityMatrix;
			_isUniformScaled = _MDirty = true;
		}
	}

    void _setModelMatrix(const mat4<F32>& matrix,bool uniform){
        _modelMatrix = glm::make_mat4(matrix.mat);
        _isUniformScaled = uniform;
		_resetModelMatrixFlag = false;
        _MDirty = true;
    }

    void _pushMatrix(){
        if(_currentMatrixMode == PROJECTION_MATRIX)   _projectionMatrix.push(_projectionMatrix.top());
        else if(_currentMatrixMode == VIEW_MATRIX){   
			_viewMatrix.push(_viewMatrix.top());
			_currentEyeVector.push(_currentEyeVector.top());
			_currentLookAtVector.push(_currentLookAtVector.top());
		}
        else if(_currentMatrixMode == TEXTURE_MATRIX) _textureMatrix.push(_textureMatrix.top());
        else assert(_currentMatrixMode == -1);
    }

    void _popMatrix(){
        if(_currentMatrixMode == PROJECTION_MATRIX){
            _projectionMatrix.pop();
			_PDirty = true;
			GL_API::getInstance().updateProjectionMatrix();
        }else if(_currentMatrixMode == VIEW_MATRIX){
            _viewMatrix.pop();
			_currentEyeVector.pop();
			_currentLookAtVector.pop();
			GL_API::getInstance().updateViewMatrix();
			_VDirty = true;
        }else if(_currentMatrixMode == TEXTURE_MATRIX){
            _textureMatrix.pop();
        }else{
            assert(_currentMatrixMode == -1);
        }
    }

    void _loadIdentity(){
        if(_currentMatrixMode == PROJECTION_MATRIX){
            _projectionMatrix.top() = _identityMatrix;
			_PDirty = true;
			GL_API::getInstance().updateProjectionMatrix();
        }else if(_currentMatrixMode == VIEW_MATRIX){
            _viewMatrix.top() = _identityMatrix;
			_currentEyeVector.top() = glm::vec3();
			_currentLookAtVector.top() = glm::vec3();
			GL_API::getInstance().updateViewMatrix();
			_VDirty = true;
        }else if(_currentMatrixMode == TEXTURE_MATRIX){
            _textureMatrix.top() = _identityMatrix;
        }else{
            assert(_currentMatrixMode == -1);
        }
    }

    void _queryMatrix(const MATRIX_MODE& mode, glm::mat4& mat){
        if(mode == VIEW_MATRIX)            mat = _viewMatrix.top();
        else if(mode == PROJECTION_MATRIX) mat = _projectionMatrix.top();
        else if(mode == TEXTURE_MATRIX)    mat = _textureMatrix.top();
        else assert(mode == -1);
    }

    void _queryMatrix(const EXTENDED_MATRIX& mode, glm::mat4& mat){
        assert(mode != NORMAL_MATRIX /*|| mode != ... */);

        switch(mode){
            case MODEL_MATRIX:{ //check if we need to reset our model matrix
				if(_resetModelMatrixFlag) _resetModelMatrix(true);
                mat = _modelMatrix;
				}break;
            case MV_MATRIX:{
				_clean(); //refresh cache
                mat = _MVCachedMatrix;
               }break;
            case MV_INV_MATRIX:{
				_clean(); //refresh cache
                mat = glm::inverse(_MVCachedMatrix);
                }break;
            case MVP_MATRIX:{
				_clean(); //refresh cache
                mat = _MVPCachedMatrix;
                }break;
            case BIAS_MATRIX:
                mat = _biasMatrix;
                break;
        };
    }

    void _queryMatrix(const EXTENDED_MATRIX& mode, glm::mat3& mat){
        assert(mode == NORMAL_MATRIX /*|| mode == ... */);
        _clean();
		// Normal Matrix
        mat = _isUniformScaled ? glm::mat3(_MVCachedMatrix) : glm::inverseTranspose(glm::mat3(_MVCachedMatrix));
    }

     void _clean(){
		if(_resetModelMatrixFlag) _resetModelMatrix(true);

		bool MVDirty = (_MDirty || _VDirty);

		if(MVDirty)	            _MVCachedMatrix = _viewMatrix.top() * _modelMatrix;
		if(_PDirty || MVDirty)	_MVPCachedMatrix = _projectionMatrix.top() * _MVCachedMatrix;
		

		_MDirty = _VDirty = _PDirty = false;
    }
}//namespace GL
}// namespace Divide