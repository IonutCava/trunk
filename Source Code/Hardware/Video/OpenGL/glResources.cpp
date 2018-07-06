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

#include <glim.h>

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
        GLuint _invalidObjectID = GL_INVALID_INDEX;
        GLFWwindow* _mainWindow     = nullptr;
        GLFWwindow* _loaderWindow   = nullptr;

        /// this may not seem very efficient (or useful) but it saves a lot of single-use code scattered around further down
        GLint getIntegerv(GLenum param) {
            GLint tempValue = 0;
            glGetIntegerv(param, &tempValue);
            return tempValue;
        }

        /// Send a shutdown request to the app on X-button clicks
        void glfw_close_callback(GLFWwindow *window) {
            Application::getInstance().RequestShutdown();
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
        }// InitGlew

        namespace GL_ENUM_TABLE {
            GLenum glBlendTable[BlendProperty_PLACEHOLDER];
            GLenum glBlendOpTable[BlendOperation_PLACEHOLDER];
            GLenum glCompareFuncTable[ComparisonFunction_PLACEHOLDER];
            GLenum glStencilOpTable[StencilOperation_PLACEHOLDER];
            GLenum glCullModeTable[CullMode_PLACEHOLDER];
            GLenum glFillModeTable[FillMode_PLACEHOLDER];
            GLenum glTextureTypeTable[TextureType_PLACEHOLDER];
            GLenum glImageFormatTable[GFXImageFormat_PLACEHOLDER];
            GLenum glPrimitiveTypeTable[PrimitiveType_PLACEHOLDER];
            GLenum glDataFormat[GDF_PLACEHOLDER];
            GLuint glWrapTable[TextureWrap_PLACEHOLDER];
            GLuint glTextureFilterTable[TextureFilter_PLACEHOLDER];
            NS_GLIM::GLIM_ENUM glimPrimitiveType[PrimitiveType_PLACEHOLDER];

            void fill() {
               glBlendTable[BLEND_PROPERTY_ZERO] = GL_ZERO;
               glBlendTable[BLEND_PROPERTY_ONE] = GL_ONE;
               glBlendTable[BLEND_PROPERTY_SRC_COLOR] = GL_SRC_COLOR;
               glBlendTable[BLEND_PROPERTY_INV_SRC_COLOR] = GL_ONE_MINUS_SRC_COLOR;
               glBlendTable[BLEND_PROPERTY_SRC_ALPHA] = GL_SRC_ALPHA;
               glBlendTable[BLEND_PROPERTY_INV_SRC_ALPHA] = GL_ONE_MINUS_SRC_ALPHA;
               glBlendTable[BLEND_PROPERTY_DEST_ALPHA] = GL_DST_ALPHA;
               glBlendTable[BLEND_PROPERTY_INV_DEST_ALPHA] = GL_ONE_MINUS_DST_ALPHA;
               glBlendTable[BLEND_PROPERTY_DEST_COLOR] = GL_DST_COLOR;
               glBlendTable[BLEND_PROPERTY_INV_DEST_COLOR] = GL_ONE_MINUS_DST_COLOR;
               glBlendTable[BLEND_PROPERTY_SRC_ALPHA_SAT] = GL_SRC_ALPHA_SATURATE;

               glBlendOpTable[BLEND_OPERATION_ADD] = GL_FUNC_ADD;
               glBlendOpTable[BLEND_OPERATION_SUBTRACT] = GL_FUNC_SUBTRACT;
               glBlendOpTable[BLEND_OPERATION_REV_SUBTRACT] = GL_FUNC_REVERSE_SUBTRACT;
               glBlendOpTable[BLEND_OPERATION_MIN] = GL_MIN;
               glBlendOpTable[BLEND_OPERATION_MAX] = GL_MAX;

               glCompareFuncTable[CMP_FUNC_NEVER] = GL_NEVER;
               glCompareFuncTable[CMP_FUNC_LESS] = GL_LESS;
               glCompareFuncTable[CMP_FUNC_EQUAL] = GL_EQUAL;
               glCompareFuncTable[CMP_FUNC_LEQUAL] = GL_LEQUAL;
               glCompareFuncTable[CMP_FUNC_GREATER] = GL_GREATER;
               glCompareFuncTable[CMP_FUNC_NEQUAL] = GL_NOTEQUAL;
               glCompareFuncTable[CMP_FUNC_GEQUAL] = GL_GEQUAL;
               glCompareFuncTable[CMP_FUNC_ALWAYS] = GL_ALWAYS;

               glStencilOpTable[STENCIL_OPERATION_KEEP] = GL_KEEP;
               glStencilOpTable[STENCIL_OPERATION_ZERO] = GL_ZERO;
               glStencilOpTable[STENCIL_OPERATION_REPLACE] = GL_REPLACE;
               glStencilOpTable[STENCIL_OPERATION_INCR] = GL_INCR;
               glStencilOpTable[STENCIL_OPERATION_DECR] = GL_DECR;
               glStencilOpTable[STENCIL_OPERATION_INV]  = GL_INVERT;
               glStencilOpTable[STENCIL_OPERATION_INCR_WRAP] = GL_INCR_WRAP;
               glStencilOpTable[STENCIL_OPERATION_DECR_WRAP] = GL_DECR_WRAP;

               glCullModeTable[CULL_MODE_NONE] = GL_BACK;
               glCullModeTable[CULL_MODE_CW]   = GL_BACK;
               glCullModeTable[CULL_MODE_CCW]  = GL_FRONT;
               glCullModeTable[CULL_MODE_ALL]  = GL_FRONT_AND_BACK;

               glFillModeTable[FILL_MODE_POINT]     = GL_POINT;
               glFillModeTable[FILL_MODE_WIREFRAME] = GL_LINE;
               glFillModeTable[FILL_MODE_SOLID]     = GL_FILL;

               glTextureTypeTable[TEXTURE_1D] = GL_TEXTURE_1D;
               glTextureTypeTable[TEXTURE_2D] = GL_TEXTURE_2D;
               glTextureTypeTable[TEXTURE_3D] = GL_TEXTURE_3D;
               glTextureTypeTable[TEXTURE_CUBE_MAP] = GL_TEXTURE_CUBE_MAP;
               glTextureTypeTable[TEXTURE_2D_ARRAY] = GL_TEXTURE_2D_ARRAY_EXT;
               glTextureTypeTable[TEXTURE_CUBE_ARRAY] = GL_TEXTURE_CUBE_MAP_ARRAY;
               glTextureTypeTable[TEXTURE_2D_MS] = GL_TEXTURE_2D_MULTISAMPLE;
               glTextureTypeTable[TEXTURE_2D_ARRAY_MS] = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

               glImageFormatTable[LUMINANCE] = GL_LUMINANCE;
               glImageFormatTable[LUMINANCE_ALPHA] = GL_LUMINANCE_ALPHA;
               glImageFormatTable[LUMINANCE_ALPHA16F] = GL_LUMINANCE_ALPHA16F_ARB;
               glImageFormatTable[LUMINANCE_ALPHA32F] = GL_LUMINANCE_ALPHA32F_ARB;
               glImageFormatTable[INTENSITY] = GL_INTENSITY;
               glImageFormatTable[ALPHA] = GL_ALPHA;
               glImageFormatTable[RED] = GL_RED;
               glImageFormatTable[RED8] = GL_R8;
               glImageFormatTable[RED16] = GL_R16;
               glImageFormatTable[RED16F] = GL_R16F;
               glImageFormatTable[RED32] = GL_R32UI;
               glImageFormatTable[RED32F ] = GL_R32F;
               glImageFormatTable[BLUE] = GL_BLUE;
               glImageFormatTable[GREEN] = GL_GREEN;
               glImageFormatTable[RG] = GL_RG;
               glImageFormatTable[RG8] = GL_RG8;
               glImageFormatTable[RG16] = GL_RG16;
               glImageFormatTable[RG16F] = GL_RG16F;
               glImageFormatTable[RG32]  = GL_RG32UI;
               glImageFormatTable[RG32F] = GL_RG32F;
               glImageFormatTable[RGB] = GL_RGB;
               glImageFormatTable[BGR] = GL_BGR;
               glImageFormatTable[RGB8] = GL_RGB8;
               glImageFormatTable[RGB8I] = GL_RGB8I;
               glImageFormatTable[RGB16] = GL_RGB16;
               glImageFormatTable[RGB16F] = GL_RGB16F;
               glImageFormatTable[BGRA] = GL_BGRA;
               glImageFormatTable[RGBA] = GL_RGBA;
               glImageFormatTable[RGBA4] = GL_RGBA4;
               glImageFormatTable[RGBA8] = GL_RGBA8;
               glImageFormatTable[RGBA8I] = GL_RGBA8I;
               glImageFormatTable[RGBA16F] = GL_RGBA16F;
               glImageFormatTable[RGBA32F] = GL_RGBA32F;
               glImageFormatTable[DEPTH_COMPONENT] = GL_DEPTH_COMPONENT;
               glImageFormatTable[DEPTH_COMPONENT16] = GL_DEPTH_COMPONENT16;
               glImageFormatTable[DEPTH_COMPONENT24] = GL_DEPTH_COMPONENT24;
               glImageFormatTable[DEPTH_COMPONENT32] = GL_DEPTH_COMPONENT32;
               glImageFormatTable[DEPTH_COMPONENT32F] = GL_DEPTH_COMPONENT32F;

               glPrimitiveTypeTable[API_POINTS] = GL_POINTS;
               glPrimitiveTypeTable[LINES] = GL_LINES;
               glPrimitiveTypeTable[LINE_LOOP] = GL_LINE_LOOP;
               glPrimitiveTypeTable[LINE_STRIP] = GL_LINE_STRIP;
               glPrimitiveTypeTable[TRIANGLES] = GL_TRIANGLES;
               glPrimitiveTypeTable[TRIANGLE_STRIP] = GL_TRIANGLE_STRIP;
               glPrimitiveTypeTable[TRIANGLE_FAN] = GL_TRIANGLE_FAN;
               //glPrimitiveTypeTable[QUADS] = GL_QUADS; //<Deprecated
               glPrimitiveTypeTable[QUAD_STRIP] = GL_QUAD_STRIP;
               glPrimitiveTypeTable[POLYGON] = GL_POLYGON;

               glDataFormat[UNSIGNED_BYTE] = GL_UNSIGNED_BYTE;
               glDataFormat[UNSIGNED_SHORT] = GL_UNSIGNED_SHORT;
               glDataFormat[UNSIGNED_INT] = GL_UNSIGNED_INT;
               glDataFormat[SIGNED_BYTE] = GL_BYTE;
               glDataFormat[SIGNED_SHORT] = GL_SHORT;
               glDataFormat[SIGNED_INT] = GL_INT;
               glDataFormat[FLOAT_16] = GL_HALF_FLOAT;
               glDataFormat[FLOAT_32] = GL_FLOAT;

               glWrapTable[TEXTURE_MIRROR_REPEAT] = GL_MIRRORED_REPEAT;
               glWrapTable[TEXTURE_REPEAT] = GL_REPEAT;
               glWrapTable[TEXTURE_CLAMP] = GL_CLAMP;
               glWrapTable[TEXTURE_CLAMP_TO_EDGE] = GL_CLAMP_TO_EDGE;
               glWrapTable[TEXTURE_CLAMP_TO_BORDER] =  GL_CLAMP_TO_BORDER;
               glWrapTable[TEXTURE_DECAL] = GL_DECAL;

               glTextureFilterTable[TEXTURE_FILTER_LINEAR] = GL_LINEAR;
               glTextureFilterTable[TEXTURE_FILTER_NEAREST] = GL_NEAREST;
               glTextureFilterTable[TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST] = GL_NEAREST_MIPMAP_NEAREST;
               glTextureFilterTable[TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST] = GL_LINEAR_MIPMAP_NEAREST;
               glTextureFilterTable[TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR] = GL_NEAREST_MIPMAP_LINEAR;
               glTextureFilterTable[TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR] = GL_LINEAR_MIPMAP_LINEAR;

               glimPrimitiveType[API_POINTS] = NS_GLIM::GLIM_POINTS;
               glimPrimitiveType[LINES] = NS_GLIM::GLIM_LINES;
               glimPrimitiveType[LINE_LOOP] = NS_GLIM::GLIM_LINE_LOOP;
               glimPrimitiveType[LINE_STRIP] = NS_GLIM::GLIM_LINE_STRIP;
               glimPrimitiveType[TRIANGLES] = NS_GLIM::GLIM_TRIANGLES;
               glimPrimitiveType[TRIANGLE_STRIP] = NS_GLIM::GLIM_TRIANGLE_STRIP;
               glimPrimitiveType[TRIANGLE_FAN] = NS_GLIM::GLIM_TRIANGLE_FAN;
               glimPrimitiveType[QUAD_STRIP] = NS_GLIM::GLIM_QUAD_STRIP;
               glimPrimitiveType[POLYGON] = NS_GLIM::GLIM_POLYGON;
            }
        }
    }/// GLutil

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