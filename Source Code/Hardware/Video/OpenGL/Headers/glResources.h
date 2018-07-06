/*
   Copyright (c) 2014 DIVIDE-Studio
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

//#define _GLDEBUG_IN_RELEASE

#include "config.h"

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
    namespace GLUtil {

        GLint getIntegerv(GLenum param);
        void glfw_close_callback(GLFWwindow *window);
        void glfw_focus_callback(GLFWwindow *window, I32);
        void glfw_error_callback(GLint error, const char* description);
        void initGlew();

#if defined(_DEBUG) || defined(_PROFILE) || defined(_GLDEBUG_IN_RELEASE)
        /// Check the current operation for errors
        void APIENTRY CALLBACK DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
#endif

        /// Half float conversion from: http://www.opengl.org/discussion_boards/archive/index.php/t-154530.html [thx gking]
        static GLfloat htof(GLhalf val);
        static GLhalf ftoh(GLfloat val);
        //static GLuint ftopacked(GLfloat val);

        /*--------- Object Management-------*/
        /// Invalid object value. Used to compare handles and determine if they were properly created
        extern GLuint _invalidObjectID;
        /// Main rendering window
        extern GLFWwindow* _mainWindow;
        /// Background thread for loading resources
        extern GLFWwindow* _loaderWindow;
    }
}

/* ARB_vertex_type_2_10_10_10_rev */
#define P10(f) ((I32)((f) * 0x1FF))
#define UP10(f) ((I32)((f) * 0x3FF))
#define PN2(f) ((I32)((f) < 0.333 ? 3 /* = -0.333 */ : (f) < 1 ? 0 /* = 0.333 */ : 1)) /* normalized */
#define P2(f) ((I32)(f)) /* unnormalized */
#define UP2(f) ((I32)((f) * 0x3))
#define P1010102(x,y,z,w) (P10(x) | (P10(y) << 10) | (P10(z) << 20) | (P2(w) << 30))
#define PN1010102(x,y,z,w) (P10(x) | (P10(y) << 10) | (P10(z) << 20) | (PN2(w) << 30))
#define UP1010102(x,y,z,w) (UP10(x) | (UP10(y) << 10) | (UP10(z) << 20) | (UP2(w) << 30))
#define BUFFER_OFFSET(i) reinterpret_cast<void*>(i)

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