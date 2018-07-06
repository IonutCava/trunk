/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GL_RESOURCES_H_
#define _GL_RESOURCES_H_

#include "config.h"

enum MATRIX_MODE;
enum EXTENDED_MATRIX;
#if defined( OS_WINDOWS )
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
#elif defined OS_LINUX
#  include <X11/Xlib.h>
//////////////////////////////////////////////////////////////////////
////////////////////////////////////Needed Mac Headers//////////////
#elif defined OS_APPLE
#  include <Carbon/Carbon.h>
#endif

#if !defined(__gl_h_) && !defined(__GL_H__) && !defined(__X_GL_H)

#include <stack>
#ifndef GLEW_STATIC
#define GLEW_STATIC
#define GLEW_MX
#endif
#include <glew.h>
#if defined( OS_WINDOWS )
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
#include "Hardware/Platform/Headers/PlatformDefines.h"
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

template<class T> class mat4;
template<class T> class mat3;
template<class T> class vec3;
template<class T> class vec4;

namespace Divide {
    namespace GL {

	///State the various attribute locations to use in GLSL with VAO/VBO's
	enum {
		 VERTEX_POSITION_LOCATION    = 0,
		 VERTEX_COLOR_LOCATION       = 1,
		 VERTEX_NORMAL_LOCATION      = 2,
		 VERTEX_TEXCOORD_LOCATION    = 3,
		 VERTEX_TANGENT_LOCATION     = 4,
		 VERTEX_BITANGENT_LOCATION   = 5,
		 VERTEX_BONE_WEIGHT_LOCATION = 6,
		 VERTEX_BONE_INDICE_LOCATION = 7
	};
	
	/*----------- GLU overrides ------*/
    typedef std::stack<glm::mat4, vectorImpl<glm::mat4 > > matrixStack;
	typedef std::stack<vec3<F32>, vectorImpl<vec3<F32> > > vector3Stack;
	typedef std::stack<vec4<U32>, vectorImpl<vec4<U32> > > viewportStack;

	/*--------- Object Management-------*/
	extern GLuint _invalidObjectID;
	/*--------- Context Management -----*/
	extern bool _applicationClosing;
	extern bool _contextAvailable;
	extern bool _useDebugOutputCallback;
	
	///Main rendering window
	extern GLFWwindow* _mainWindow;
	///Background thread for loading resources
	extern GLFWwindow* _loaderWindow;
	 ///Matrix management
    ///Current active matrix for push/pop operations: PROJECTION/VIEW/TEXTURE
    extern MATRIX_MODE _currentMatrixMode;
    ///Current view matrix. Changed only by LookAt call
	extern matrixStack _viewMatrix;
    ///Current projection matrix. Changed by Perspective and Ortho calls
	extern matrixStack _projectionMatrix;
    ///Current texture matrix. Multiply and change manually if needed
    extern matrixStack _textureMatrix;
	///Current viewpoert stack
	extern viewportStack _viewport;
    ///The current model matrix. Change per model or set to identity
    extern glm::mat4   _modelMatrix;
    ///A bias matrix is useful for shadow calculations
    extern glm::mat4   _biasMatrix;
	///A cache value for anaglyph eye offset
	extern GLfloat     _anaglyphIOD;
	///current camera view direction
	extern vector3Stack _currentViewDirection;
    ///If the model matrix had a uniform scale applied or not
    extern bool _isUniformScaled;
	///Use for lazy reset of the model matrix
	extern bool _resetModelMatrixFlag;

    void _initStacks();
    /*-----------------BEGIN: FIXED PIPELINE EMULATION -----------------------*/
	inline void _matrixMode(const MATRIX_MODE& mode) { _currentMatrixMode = mode; }

    void _LookAt(const GLfloat* viewMatrix, const vec3<F32>& viewDirection);
    void _ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
    void _perspective(GLfloat fovy, GLfloat aspect, GLfloat zNear,  GLfloat zFar);
	void _anaglyph(GLfloat IOD, GLdouble zNear, GLdouble zFar, GLfloat aspect, GLfloat fovy, bool rightFrustum = false);
    void _pushMatrix();
    void _popMatrix();
    void _loadIdentity();
    /*-----------------END: FIXED PIPELINE EMULATION -----------------------*/
    void _setModelMatrix(const mat4<GLfloat>& matrix,bool uniform = true);//<Use uniform to avoid inverseTranspose for normals
	void _resetModelMatrix(bool force = false);//<Use this to set the model matrix back to identity
    void _queryMatrix(const MATRIX_MODE& mode,     mat4<GLfloat>& mat);
    void _queryMatrix(const EXTENDED_MATRIX& mode, mat4<GLfloat>& mat);
    void _queryMatrix(const EXTENDED_MATRIX& mode, mat3<GLfloat>& mat);

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
	///A cache value for anaglyph eye offset
	extern GLfloat     _anaglyphIOD;
	///current camera view direction
	extern vector3Stack _currentViewDirection;
    ///If the model matrix had a uniform scale applied or not
    extern bool _isUniformScaled;
	///Use for lazy reset of the model matrix
	extern bool _resetModelMatrixFlag;
    }//GL
}

#ifdef GLEW_MX
	GLEWContext* glewGetContext();
#   if defined( OS_WINDOWS )
		WGLEWContext* wglewGetContext();
#	else
		GLXEWContext* glxewGetContext();
#	endif
#endif//GLEW_MX

#if defined(_MSC_VER)
#	pragma warning( pop )
#elif defined(__GNUC__)
#	pragma GCC diagnostic pop
#endif

#endif
#endif