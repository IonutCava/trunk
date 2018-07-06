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
        ///Used for anaglyph rendering
        struct CameraFrustum {
            GLdouble leftfrustum;
            GLdouble rightfrustum;
            GLdouble bottomfrustum;
            GLdouble topfrustum;
            GLfloat modeltranslation;
        } _leftCam, _rightCam;

        /*-----------Object Management----*/
        GLuint _invalidObjectID = std::numeric_limits<U32>::max();

        /*-----------Context Management----*/
        bool _applicationClosing = false;
        bool _contextAvailable = false;
        bool _useDebugOutputCallback = false;
        GLFWwindow* _mainWindow     = NULL;
        GLFWwindow* _loaderWindow   = NULL;

        /*----------- GLU overrides ------*/
        ///Matrix management
        MATRIX_MODE   _currentMatrixMode;
        matrixStack   _viewMatrix;
        matrixStack   _projectionMatrix;
        matrixStack   _textureMatrix;
        matrixStack   _modelMatrix;
        viewportStack _viewport;
        glm::mat4     _MVPCachedMatrix;
        glm::mat4     _MVCachedMatrix;
        glm::mat4     _identityMatrix;
        glm::mat4     _biasMatrix;
        vector3Stack  _currentViewDirection;

        bool _MDirty = true;
        bool _VDirty = true;
        bool _PDirty = true;
        bool _isUniformScaled = true;
        bool _resetModelMatrixFlag = true;
        GLfloat  _anaglyphIOD = -0.01f;

        void _initStacks(){
            _currentViewDirection.push(vec3<GLfloat>());
            _projectionMatrix.push(_identityMatrix);
            _viewMatrix.push(_identityMatrix);
            _textureMatrix.push(_identityMatrix);
            _modelMatrix.push(_identityMatrix);
            _viewport.push(vec4<U32>(0,0,0,0));
            _biasMatrix = glm::mat4(0.5, 0.0, 0.0, 0.0,
                                    0.0, 0.5, 0.0, 0.0,
                                    0.0, 0.0, 0.5, 0.0,
                                    0.5, 0.5, 0.5, 1.0);
        }

        void _unproject(const vec3<GLfloat>& windowCoords, vec3<GLfloat>& objCoords) {
            glm::vec4 viewport(_viewport.top().x, _viewport.top().y, _viewport.top().z, _viewport.top().w);
            glm::vec3 wincoord(windowCoords.x, windowCoords.y, windowCoords.z);
            glm::vec3 objcoord = glm::unProject(wincoord, _viewMatrix.top(), _projectionMatrix.top(), viewport);
            objCoords.set(objcoord.x, objcoord.y, objcoord.z);
        }

        void _lookAt(const GLfloat* viewMatrix, const vec3<GLfloat>& viewDirection){
           _matrixMode(VIEW_MATRIX);
           _currentViewDirection.top() = viewDirection;
           _viewMatrix.top() = glm::make_mat4(viewMatrix);
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

        void _anaglyph(GLfloat IOD, GLdouble zNear, GLdouble zFar, GLfloat aspect, GLfloat fovy, bool rightFrustum){
            if(!FLOAT_COMPARE(_anaglyphIOD,IOD)){
                static const GLdouble DTR = 0.0174532925;
                static const GLdouble screenZ = 10.0;

                //sets top of frustum based on fovy and near clipping plane
                GLdouble top = zNear*tan(DTR*fovy/2);
                GLdouble right = aspect*top;
                //sets right of frustum based on aspect ratio
                GLdouble frustumshift = (IOD/2)*zNear/screenZ;

                _leftCam.topfrustum = top;
                _leftCam.bottomfrustum = -top;
                _leftCam.leftfrustum = -right + frustumshift;
                _leftCam.rightfrustum = right + frustumshift;
                _leftCam.modeltranslation = IOD/2;

                _rightCam.topfrustum = top;
                _rightCam.bottomfrustum = -top;
                _rightCam.leftfrustum = -right - frustumshift;
                _rightCam.rightfrustum = right - frustumshift;
                _rightCam.modeltranslation = -IOD/2;

                _anaglyphIOD = IOD;
            }

            _matrixMode(PROJECTION_MATRIX);

            CameraFrustum& tempCamera = rightFrustum ? _rightCam : _leftCam;

            _projectionMatrix.top() = glm::frustum(tempCamera.leftfrustum,
                                                   tempCamera.rightfrustum,
                                                   tempCamera.bottomfrustum,
                                                   tempCamera.topfrustum,
                                                   zNear,
                                                   zFar);
            //translate to cancel parallax
            glm::translate(_projectionMatrix.top(), glm::vec3(tempCamera.modeltranslation, 0.0, 0.0));

            GL_API::getInstance().updateProjectionMatrix();
            _PDirty = true;

            _matrixMode(VIEW_MATRIX);
        }

        void _popModelMatrix(bool force){
            _resetModelMatrixFlag = !force;

            if(!force)
                return;

            _modelMatrix.top() = _identityMatrix;
            _isUniformScaled = _MDirty = true;
        }

        void _pushModelMatrix(const mat4<GLfloat>& matrix, bool uniform){
            _modelMatrix.top() = glm::make_mat4(matrix.mat);
            _isUniformScaled = uniform;
            _resetModelMatrixFlag = false;
            _MDirty = true;
        }

        void _pushMatrix(){
            if(_currentMatrixMode == PROJECTION_MATRIX) {
                _projectionMatrix.push(_projectionMatrix.top());
            }else if(_currentMatrixMode == VIEW_MATRIX){
                _viewMatrix.push(_viewMatrix.top());
                _currentViewDirection.push(_currentViewDirection.top());
            }else if(_currentMatrixMode == TEXTURE_MATRIX) {
                _textureMatrix.push(_textureMatrix.top());
            }else {
                assert(_currentMatrixMode == -1);
            }
        }

        void _popMatrix(){
            if(_currentMatrixMode == PROJECTION_MATRIX){
                _projectionMatrix.pop();
                _PDirty = true;
                GL_API::getInstance().updateProjectionMatrix();
            }else if(_currentMatrixMode == VIEW_MATRIX){
                _viewMatrix.pop();
                _currentViewDirection.pop();
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
                _currentViewDirection.top() = vec3<GLfloat>();
                GL_API::getInstance().updateViewMatrix();
                _VDirty = true;
            }else if(_currentMatrixMode == TEXTURE_MATRIX){
                _textureMatrix.top() = _identityMatrix;
            }else{
                assert(_currentMatrixMode == -1);
            }
        }

        void _queryMatrix(const MATRIX_MODE& mode, mat4<GLfloat>& mat){
            if(mode == VIEW_MATRIX)
                mat.set(glm::value_ptr(_viewMatrix.top()));
            else if(mode == PROJECTION_MATRIX)
                mat.set(glm::value_ptr(_projectionMatrix.top()));
            else if(mode == TEXTURE_MATRIX)
                mat.set(glm::value_ptr(_textureMatrix.top()));
            else
                assert(mode == -1);
        }

        void _queryMatrix(const EXTENDED_MATRIX& mode, mat4<GLfloat>& mat){
            assert(mode != NORMAL_MATRIX /*|| mode != ... */);
            const GLfloat* matrixValue = NULL;

            switch(mode){
                case MODEL_MATRIX:{ //check if we need to reset our model matrix
                    if(_resetModelMatrixFlag)
                        _popModelMatrix(true);
                    matrixValue = glm::value_ptr(_modelMatrix.top());
                    }break;
                case MV_MATRIX:{
                    _clean(); //refresh cache
                    matrixValue = glm::value_ptr(_MVCachedMatrix);
                   }break;
                case MV_INV_MATRIX:{
                    _clean(); //refresh cache
                    matrixValue = glm::value_ptr(glm::inverse(_MVCachedMatrix));
                    }break;
                case MVP_MATRIX:{
                    _clean(); //refresh cache
                    matrixValue = glm::value_ptr(_MVPCachedMatrix);
                    }break;
                case MVP_INV_MATRIX:{
                    _clean(); //refresh cache
                    matrixValue = glm::value_ptr(glm::inverse(_MVPCachedMatrix));
                    }break;
                case BIAS_MATRIX:
                    matrixValue = glm::value_ptr(_biasMatrix);
                    break;
            };

            assert(matrixValue != NULL);
            mat.set(matrixValue);
        }

        void _queryMatrix(const EXTENDED_MATRIX& mode, mat3<GLfloat>& mat){
            assert(mode == NORMAL_MATRIX /*|| mode == ... */);
            _clean();
            // Normal Matrix
           _isUniformScaled ? mat.set(glm::value_ptr(glm::mat3(_MVCachedMatrix))) :
                              mat.set(glm::value_ptr(glm::inverseTranspose(glm::mat3(_MVCachedMatrix))));
        }

         void _clean(){
            if(_resetModelMatrixFlag)
                _popModelMatrix(true);

            bool MVDirty = (_MDirty || _VDirty);

            if(MVDirty)
                _MVCachedMatrix = _viewMatrix.top() * _modelMatrix.top();
            if(_PDirty || MVDirty)
                _MVPCachedMatrix = _projectionMatrix.top() * _MVCachedMatrix;

            _MDirty = _VDirty = _PDirty = false;
        }
    }//namespace GL
}// namespace Divide