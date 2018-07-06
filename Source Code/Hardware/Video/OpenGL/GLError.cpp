#include "Headers/GLWrapper.h"

namespace Divide {
    namespace GLUtil {
        void glfw_error_callback(GLint error, const char* description){
            ERROR_FN(Locale::get("ERROR_GENERIC_GLFW"), description);
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