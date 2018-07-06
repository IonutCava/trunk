/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GL_RESOURCES_H_
#define _GL_RESOURCES_H_

enum MATRIX_MODE;
enum EXTENDED_MATRIX;
#if defined( __WIN32__ ) || defined( _WIN32 )
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include "windows.h"
#  ifdef min
#    undef min
#  endif
#  ifdef max
#	 undef max
#  endif
//////////////////////////////////////////////////////////////////////
////////////////////////////////////Needed Linux Headers//////////////
#elif defined OIS_LINUX_PLATFORM
#  include <X11/Xlib.h>
//////////////////////////////////////////////////////////////////////
////////////////////////////////////Needed Mac Headers//////////////
#elif defined OIS_APPLE_PLATFORM
#  include <Carbon/Carbon.h>
#endif

#if !defined(__gl_h_) && !defined(__GL_H__) && !defined(__X_GL_H)

#include <stack>
#ifndef GLEW_STATIC
#define GLEW_STATIC
#define GLEW_MX
#endif
#include <glew.h>
#if defined(_WIN32)
#include <wglew.h>
#else
#include <glxew.h>
#endif

#ifndef GLM_FORCE_INLINE
#define GLM_FORCE_INLINE
#endif
#include <glm.hpp>
#include <GL/glfw3.h>
#include "Utility/Headers/Vector.h"
#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>
#include <boost/thread/tss.hpp>


#if defined(_MSC_VER)
#	pragma warning( push )
#		pragma warning(disable:4505) ///<unreferenced local function removal
#		pragma warning(disable:4100) ///<unreferenced formal parameter
#elif defined(__GNUC__)
#	pragma GCC diagnostic push
#		//pragma GCC diagnostic ignored "-Wall"
#endif

namespace Divide {
	namespace GL {
		void glfw_error_callback(GLint error, const char* description);
		void initGlew();
#ifdef _DEBUG
		///from: https://sites.google.com/site/opengltutorialsbyaks/introduction-to-opengl-4-1---tutorial-05
		///Check the current operation for errors
		void DebugOutputToFile(GLenum source, GLenum type, GLuint id, GLenum severity, const GLchar* message);
		void DebugOutputToFileAMD(GLenum source, GLuint id, GLenum severity, const GLchar* message);
		void CALLBACK DebugCallbackARB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam);
		void CALLBACK DebugCallbackAMD(GLenum source, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam);
		///Check the current operation for errors
		void GLCheckError(const std::string& File, GLuint Line, GLchar* operation);
		///Clear previous error codes
		void GLFlushErrors();
#endif
	}
}

#ifdef _DEBUG
#    define GLCheck(Func) (Divide::GL::GLFlushErrors(), (Func), Divide::GL::GLCheckError(__FILE__, __LINE__,#Func))
#else
#    define GLCheck(Func) (Func)
#endif

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

///Half float conversion from: http://www.opengl.org/discussion_boards/archive/index.php/t-154530.html [thx gking]
namespace glDataConversion {
	union nv_half_data {
		GLhalf bits;
		struct {
			unsigned long m : 10;
			unsigned long e : 5;
			unsigned long s : 1;
		} ieee;
	};

	union ieee_single {
		GLfloat f;
		struct {
			unsigned long m : 23;
			unsigned long e : 8;
			unsigned long s : 1;
		} ieee;
	};

	inline static GLfloat htof(GLhalf val) {
		nv_half_data h;
		h.bits = val;
		ieee_single sng;
		sng.ieee.s = h.ieee.s;

		// handle special cases
		if ( (h.ieee.e==0) && (h.ieee.m==0) ) { // zero
			sng.ieee.m=0;
			sng.ieee.e=0;
		}else if ( (h.ieee.e==0) && (h.ieee.m!=0) ) { // denorm -- denorm half will fit in non-denorm single
			const GLfloat half_denorm = (1.0f/16384.0f); // 2^-14
			GLfloat mantissa = ((GLfloat)(h.ieee.m)) / 1024.0f;
			GLfloat sgn = (h.ieee.s)? -1.0f :1.0f;
			sng.f = sgn*mantissa*half_denorm;
		}else if ( (h.ieee.e==31) && (h.ieee.m==0) ) { // infinity
			sng.ieee.e = 0xff;
			sng.ieee.m = 0;
		}else if ( (h.ieee.e==31) && (h.ieee.m!=0) ) { // NaN
			sng.ieee.e = 0xff;
			sng.ieee.m = 1;
		}else {
			sng.ieee.e = h.ieee.e+112;
			sng.ieee.m = (h.ieee.m << 13);
		}
		return sng.f;
	}

	inline static GLhalf ftoh(GLfloat val) {
		ieee_single f;
		f.f = val;
		nv_half_data h;

		h.ieee.s = f.ieee.s;

		// handle special cases
		//const GLfloat half_denorm = (1.0f/16384.0f);

		if ( (f.ieee.e==0) && (f.ieee.m==0) ) { // zero
			h.ieee.m = 0;
			h.ieee.e = 0;
		}else if ( (f.ieee.e==0) && (f.ieee.m!=0) ) { // denorm -- denorm float maps to 0 half
			h.ieee.m = 0;
			h.ieee.e = 0;
		}else if ( (f.ieee.e==0xff) && (f.ieee.m==0) ) { // infinity
			h.ieee.m = 0;
			h.ieee.e = 31;
		}else if ( (f.ieee.e==0xff) && (f.ieee.m!=0) ) { // NaN
			h.ieee.m = 1;
			h.ieee.e = 31;
		}else { // regular number
			GLint new_exp = f.ieee.e-127;
			if (new_exp<-24) { // this maps to 0
				h.ieee.m = 0;
				h.ieee.e = 0;
			}

			if (new_exp<-14) { // this maps to a denorm
				h.ieee.e = 0;
				GLuint exp_val = (GLuint) (-14 - new_exp); // 2^-exp_val
				switch (exp_val) {
					case 0: fprintf(stderr, "ftoh: logical error in denorm creation!\n"); h.ieee.m = 0; break;
					case 1: h.ieee.m = 512 + (f.ieee.m>>14); break;
					case 2: h.ieee.m = 256 + (f.ieee.m>>15); break;
					case 3: h.ieee.m = 128 + (f.ieee.m>>16); break;
					case 4: h.ieee.m = 64 + (f.ieee.m>>17); break;
					case 5: h.ieee.m = 32 + (f.ieee.m>>18); break;
					case 6: h.ieee.m = 16 + (f.ieee.m>>19); break;
					case 7: h.ieee.m = 8 + (f.ieee.m>>20); break;
					case 8: h.ieee.m = 4 + (f.ieee.m>>21); break;
					case 9: h.ieee.m = 2 + (f.ieee.m>>22); break;
					case 10: h.ieee.m = 1; break;
				}
			}else if (new_exp>15) { // map this value to infinity
				h.ieee.m = 0;
				h.ieee.e = 31;
			}else {
				h.ieee.e = new_exp+15;
				h.ieee.m = (f.ieee.m >> 13);
			}
		}
		return h.bits;
	}

	///For use with GL_INT_2_10_10_10_REV
	//inline static GLuint ftopacked(GLfloat val) {
	//}
};
/* ARB_vertex_type_2_10_10_10_rev */
#define P10(f) ((I32)((f) * 0x1FF))
#define UP10(f) ((I32)((f) * 0x3FF))
#define PN2(f) ((I32)((f) < 0.333 ? 3 /* = -0.333 */ : (f) < 1 ? 0 /* = 0.333 */ : 1)) /* normalized */
#define P2(f) ((I32)(f)) /* unnormalized */
#define UP2(f) ((I32)((f) * 0x3))
#define P1010102(x,y,z,w) (P10(x) | (P10(y) << 10) | (P10(z) << 20) | (P2(w) << 30))
#define PN1010102(x,y,z,w) (P10(x) | (P10(y) << 10) | (P10(z) << 20) | (PN2(w) << 30))
#define UP1010102(x,y,z,w) (UP10(x) | (UP10(y) << 10) | (UP10(z) << 20) | (UP2(w) << 30))

template<class T>
class mat4;

namespace Divide {
    namespace GL {
	///State the various attribute locations to use in GLSL with VAO/VBO's
	enum {
		 VERTEX_POSITION_LOCATION    = 0,
		 VERTEX_NORMAL_LOCATION      = 1,
		 VERTEX_TEXCOORD_LOCATION    = 2,
		 VERTEX_TANGENT_LOCATION     = 3,
		 VERTEX_BITANGENT_LOCATION   = 4,
		 VERTEX_BONE_WEIGHT_LOCATION = 5,
		 VERTEX_BONE_INDICE_LOCATION = 6
	};

	/*--------- Context Management -----*/
	extern bool _applicationClosing;
	extern bool _contextAvailable;
	extern bool _useDebugOutputCallback;

	///Main rendering window
	extern GLFWwindow* _mainWindow;
	///Background thread for loading resources
	extern GLFWwindow* _loaderWindow;

	/*----------- GLU overrides ------*/
    typedef std::stack<glm::mat4, vectorImpl<glm::mat4 > > matrixStack;
	typedef std::stack<glm::vec3, vectorImpl<glm::vec3 > > vector3Stack;

    void _initStacks();
    /*-----------------BEGIN: FIXED PIPELINE EMULATION -----------------------*/
	void _matrixMode(const MATRIX_MODE& mode);

    void _LookAt(const glm::vec3& eye,const glm::vec3& lookAt,const glm::vec3& up, bool invertx, bool inverty);
    void _ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
    void _perspective(GLfloat fovy, GLfloat aspect, GLfloat zNear,  GLfloat zFar);
    void _pushMatrix();
    void _popMatrix();
    void _loadIdentity();
    /*-----------------END: FIXED PIPELINE EMULATION -----------------------*/
    void _setModelMatrix(const mat4<GLfloat>& matrix,bool uniform = true);//<Use uniform to avoid inverseTranspose for normals
	void _resetModelMatrix(bool force = false);//<Use this to set the model matrix back to identity
    void _queryMatrix(const MATRIX_MODE& mode, glm::mat4& mat);
    void _queryMatrix(const EXTENDED_MATRIX& mode, glm::mat4& mat);
    void _queryMatrix(const EXTENDED_MATRIX& mode, glm::mat3& mat);

    ///Cache management - try and avoid recalculating this if not needed
    void _clean();

	/*-----------------Locals------------------------------------------------*/
    extern glm::mat4   _MVCachedMatrix;
    extern glm::mat4   _MVPCachedMatrix;
	extern glm::mat4   _identityMatrix;
    ///If _projection changed, this will change to true
    extern bool _PDirty;
    ///If _model changed, this will change to true
    extern bool _MDirty;
	///If the _view changed
	extern bool _VDirty;

    ///Matrix management
    ///Current active matrix for push/pop operations: PROJECTION/VIEW/TEXTURE
    extern MATRIX_MODE _currentMatrixMode;
    ///Current view matrix. Changed only by LookAt call
	extern matrixStack _viewMatrix;
    ///Current projection matrix. Changed by Perspective and Ortho calls
	extern matrixStack _projectionMatrix;
    ///Current texture matrix. Multiply and change manually if needed
    extern matrixStack _textureMatrix;
    ///The current model matrix. Change per model or set to identity
    extern glm::mat4   _modelMatrix;
    ///A bias matrix is useful for shadow calculations
    extern glm::mat4   _biasMatrix;
	///current View_Matrix eye vector
	extern vector3Stack _currentEyeVector;
	///current View_Matrix look-at vector
	extern vector3Stack _currentLookAtVector;
    ///If the model matrix had a uniform scale applied or not
    extern bool _isUniformScaled;
	///Use for lazy reset of the model matrix
	extern bool _resetModelMatrixFlag;
    }//GL
}

#ifdef GLEW_MX
	GLEWContext* glewGetContext();
#	if defined(_WIN32)
		WGLEWContext* wglewGetContext();
#	else//_WIN32
		GLXEWContext* glxewGetContext();
#	endif//_WIN32
#endif//GLEW_MX

#if defined(_MSC_VER)
#	pragma warning( pop )
#elif defined(__GNUC__)
#	pragma GCC diagnostic pop
#endif

#endif
#endif