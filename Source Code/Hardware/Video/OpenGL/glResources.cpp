#include "Headers/glResources.h"

#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Application.h"

// We are actually importing GL specific libraries in code mainly for maintenance reasons
// We can easily adjust them as needed. Same thing with PhysX libs
#ifdef GLEW_MX
#	ifdef _DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer_MX_d.lib")
#		pragma comment(lib, "glew32mxsd.lib")
#	else //_DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer_MX.lib")
#		pragma comment(lib, "glew32mxs.lib")
#	endif //_DEBUG
#else //GLEW_MX
#	ifdef _DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer_d.lib")
#		pragma comment(lib, "glew32sd.lib")
#	else//_DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer.lib")
#		pragma comment(lib, "glew32s.lib")
#	endif //_DEBUG
#endif //GLEW_MX

#ifdef GLEW_MX
///GLEW_MX requirement
static boost::thread_specific_ptr<GLEWContext> _GLEWContextPtr;
GLEWContext * glewGetContext()  { return _GLEWContextPtr.get(); }
#if defined( OS_WINDOWS )
static boost::thread_specific_ptr<WGLEWContext> _WGLEWContextPtr;
WGLEWContext* wglewGetContext() { return _WGLEWContextPtr.get(); }
#else
static boost::thread_specific_ptr<GLXEWContext> _GLXEWContextPtr;
GLXEWContext* glxewGetContext() { return _GLXEWContextPtr.get(); }
#endif
#endif//GLEW_MX

namespace Divide {
    namespace GLUtil {

        /*-----------Object Management----*/
        GLuint _invalidObjectID = std::numeric_limits<U32>::max();
        GLFWwindow* _mainWindow     = nullptr;
        GLFWwindow* _loaderWindow   = nullptr;

        /// this may not seem very efficient (or useful) but it saves a lot of single-use code scattered around further down
        GLint getIntegerv(GLenum param) {
            GLint tempValue = 0;
            glGetIntegerv(param, &tempValue);
            return tempValue;
        }

        /// Used by GLFW to inform the application if it has focus or not
        void glfw_focus_callback(GLFWwindow *window, I32 focusState) {
            Application::getInstance().hasFocus(focusState != 0);
        }

        /// As these messages are printed if a various range of conditions are not met,
        /// it's easier to just group them in a function
        void printGLInitError(GLuint err) {
            ERROR_FN(Locale::get("ERROR_GFX_DEVICE"), glewGetErrorString(err));
            PRINT_FN(Locale::get("WARN_SWITCH_D3D"));
            PRINT_FN(Locale::get("WARN_APPLICATION_CLOSE"));
        }

        /// Glew needs extra data to be initialized if it's build with the MX flag        
        void initGlew() {
#ifdef GLEW_MX
            GLEWContext * currentGLEWContextsPtr =  _GLEWContextPtr.get();
            if (currentGLEWContextsPtr == nullptr) {
                currentGLEWContextsPtr = New GLEWContext;
                _GLEWContextPtr.reset(currentGLEWContextsPtr);
                ZeroMemory(currentGLEWContextsPtr, sizeof(GLEWContext));
            }
#endif //GLEW_MX
            // As we are using the bleeding edge of OpenGL functionality, experimental must be set to 'true';
            glewExperimental = TRUE;
            // Everything is set up as needed, so initialize the OpenGL API
            GLuint err = glewInit();
            // Check for errors and print if any (They happen more than anyone thinks)
            // Bad drivers, old video card, corrupt GPU, anything can throw an error
            if (GLEW_OK != err) {
                printGLInitError(err);
                //No need to continue, as switching to DX should be set before launching the application!
                exit(GLEW_INIT_ERROR);
            }

#ifdef GLEW_MX
#   if defined( OS_WINDOWS )
            /// Same as for normal GLEW initialization, but this time, init platform specific pointers
            WGLEWContext * currentWGLEWContextsPtr =  _WGLEWContextPtr.get();
            if (currentWGLEWContextsPtr == nullptr)	{
                currentWGLEWContextsPtr = New WGLEWContext;
                _WGLEWContextPtr.reset(currentWGLEWContextsPtr);
                ZeroMemory(currentWGLEWContextsPtr, sizeof(WGLEWContext));
            }

            err = wglewInit();
#	else //! OS_WINDOWS
            /// Same as for normal GLEW initialization, but this time, init platform specific pointers
            GLXEWContext * currentGLXEWContextsPtr =  _GLXEWContextPtr.get();
            if (currentGLXEWContextsPtr == nullptr)	{
                currentGLXEWContextsPtr = New GLXEWContext;
                _GLXEWContextPtr.reset(currentGLXEWContextsPtr);
                ZeroMemory(currentGLXEWContextsPtr, sizeof(GLXEWContext));
            }

            err = glxewInit();
#	endif //OS_WINDOWS
            if (GLEW_OK != err) {
                printGLInitError(err);
                exit(GLEW_INIT_ERROR);
            }

#endif //GLEW_MX
        }
    }

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

    static GLfloat htof(GLhalf val) {
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

    static GLhalf ftoh(GLfloat val) {
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
    //static GLuint ftopacked(GLfloat val) {
    //}
}