#include "Headers/GLWrapper.h"

namespace Divide {
    namespace GLUtil {
        void glfw_error_callback(GLint errorCode, const char* msg){
            const char* errorDesc = "";
            switch (errorCode){
                case GLFW_NOT_INITIALIZED: errorDesc = "GLFW_NOT_INITIALIZED"; break;
                case GLFW_NO_CURRENT_CONTEXT : errorDesc = "GLFW_NO_CURRENT_CONTEXT"; break;
                case GLFW_INVALID_ENUM       : errorDesc = "GLFW_INVALID_ENUM"; break;
                case GLFW_INVALID_VALUE      : errorDesc = "GLFW_INVALID_VALUE"; break;
                case GLFW_OUT_OF_MEMORY      : errorDesc = "GLFW_OUT_OF_MEMORY"; break;
                case GLFW_API_UNAVAILABLE    : errorDesc = "GLFW_API_UNAVAILABLE"; break;
                case GLFW_VERSION_UNAVAILABLE: errorDesc = "GLFW_VERSION_UNAVAILABLE"; break;
                case GLFW_PLATFORM_ERROR     : errorDesc = "GLFW_PLATFORM_ERROR"; break;
                case GLFW_FORMAT_UNAVAILABLE : errorDesc = "GLFW_FORMAT_UNAVAILABLE"; break;
            }
            ERROR_FN(Locale::get("GLFW_ERROR"), errorDesc, msg);
        }

#if defined(_DEBUG) || defined(_PROFILE)

        void APIENTRY CALLBACK DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam){
            static std::string gl_source;
            static std::string gl_severity;
            static std::string gl_type;

            if (source == GL_DEBUG_SOURCE_API)                  gl_source = "OpenGL";
            else if (source == GL_DEBUG_SOURCE_WINDOW_SYSTEM)   gl_source = "Windows";
            else if (source == GL_DEBUG_SOURCE_SHADER_COMPILER) gl_source = "Shader Compiler";
            else if (source == GL_DEBUG_SOURCE_THIRD_PARTY)     gl_source = "Third Party";
            else if (source == GL_DEBUG_SOURCE_APPLICATION)     gl_source = "Application";
            else if (source == GL_DEBUG_SOURCE_OTHER)           gl_source = "Other";

            if (type == GL_DEBUG_TYPE_ERROR)                    gl_type = "Error";
            else if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR) gl_type = "Deprecated behavior";
            else if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)  gl_type = "Undefined behavior";
            else if (type == GL_DEBUG_TYPE_PORTABILITY)         gl_type = "Portability";
            else if (type == GL_DEBUG_TYPE_PERFORMANCE)         gl_type = "Performance";
            else if (type == GL_DEBUG_TYPE_OTHER)               gl_type = "Other";

            if (severity == GL_DEBUG_SEVERITY_HIGH)             gl_severity = "High";
            else if (severity == GL_DEBUG_SEVERITY_MEDIUM)      gl_severity = "Medium";
            else if (severity == GL_DEBUG_SEVERITY_LOW)         gl_severity = "Low";

            ERROR_FN(Locale::get("ERROR_GENERIC_GL_DEBUG"), (GLuint)(userParam) == 0 ? " [Main Thread] " : " [Loader Thread] ",
                                                            gl_source.c_str(), gl_type.c_str(), id, gl_severity.c_str(),
                                                            message);
        }
#endif
    } //namespace GLUtil
}//namespace Divide