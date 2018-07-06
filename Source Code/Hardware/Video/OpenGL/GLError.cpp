#include "Headers/GLWrapper.h"

namespace Divide{
    namespace GL{
void glfw_error_callback(GLint error, const char* description){
    ERROR_FN(Locale::get("ERROR_GENERIC_GLFW"), description);
}
#ifdef _DEBUG
void GLFlushErrors() {
    if(Divide::GL::_useDebugOutputCallback) return;
    if(!Divide::GL::_contextAvailable) return;
    GLenum ErrorCode;
    std::string Error = "unknown error";
    std::string Desc  = "no description";
    while((ErrorCode  = glGetError()) != GL_NO_ERROR && !Divide::GL::_applicationClosing){
        // Decode the error code
        switch (ErrorCode) {
            case GL_INVALID_ENUM : {
                Error = "GL_INVALID_ENUM";
                Desc  = "an unacceptable value has been specified for an enumerated argument";
                break;
            }

            case GL_INVALID_VALUE : {
                Error = "GL_INVALID_VALUE";
                Desc  = "a numeric argument is out of range";
                break;
            }

            case GL_INVALID_OPERATION : {
                Error = "GL_INVALID_OPERATION";
                Desc  = "the specified operation is not allowed in the current state";
                break;
            }

            case GL_STACK_OVERFLOW : {
                Error = "GL_STACK_OVERFLOW";
                Desc  = "this command would cause a stack overflow";
                break;
            }

            case GL_STACK_UNDERFLOW : {
                Error = "GL_STACK_UNDERFLOW";
                Desc  = "this command would cause a stack underflow";
                break;
            }

            case GL_OUT_OF_MEMORY : {
                Error = "GL_OUT_OF_MEMORY";
                Desc  = "there is not enough memory left to execute the command";
                break;
            }

            case GL_INVALID_FRAMEBUFFER_OPERATION : {
                Error = "GL_INVALID_FRAMEBUFFER_OPERATION";
                Desc  = "the object bound to FRAMEBUFFER_BINDING is not \"framebuffer complete\"";
                break;
            }
        }
        std::stringstream ss;
        ss << Error << ", "
           << Desc;
        assert(false);
    }
}

void GLCheckError(const std::string& File, GLuint Line, char* operation) {
    if(Divide::GL::_useDebugOutputCallback) return;
    if(!Divide::GL::_contextAvailable) return;
    // Get the last error
    GLenum ErrorCode  = glGetError();
    if(ErrorCode == GL_NO_ERROR || Divide::GL::_applicationClosing) return;
    std::string Error = "unknown error";
    std::string Desc  = "no description";

    // Decode the error code
    switch (ErrorCode) {
        case GL_INVALID_ENUM : {
            Error = "GL_INVALID_ENUM";
            Desc  = "an unacceptable value has been specified for an enumerated argument";
            break;
        }

        case GL_INVALID_VALUE : {
            Error = "GL_INVALID_VALUE";
            Desc  = "a numeric argument is out of range";
            break;
        }

        case GL_INVALID_OPERATION : {
            Error = "GL_INVALID_OPERATION";
            Desc  = "the specified operation is not allowed in the current state";
            break;
        }

        case GL_STACK_OVERFLOW : {
            Error = "GL_STACK_OVERFLOW";
            Desc  = "this command would cause a stack overflow";
            break;
        }

        case GL_STACK_UNDERFLOW : {
            Error = "GL_STACK_UNDERFLOW";
            Desc  = "this command would cause a stack underflow";
            break;
        }

        case GL_OUT_OF_MEMORY : {
            Error = "GL_OUT_OF_MEMORY";
            Desc  = "there is not enough memory left to execute the command";
            break;
        }

        case GL_INVALID_FRAMEBUFFER_OPERATION : {
            Error = "GL_INVALID_FRAMEBUFFER_OPERATION";
            Desc  = "the object bound to FRAMEBUFFER_BINDING is not \"framebuffer complete\"";
            break;
        }
    }

    std::stringstream ss;
       ss << File.substr(File.find_last_of("\\/") + 1) << " (" << Line << ") : "
          << Error << ", "
          << Desc;
    ERROR_FN(Locale::get("ERROR_GENERIC_GL"),operation, ss.str().c_str());
//	assert(false);
}

void CALLBACK DebugCallbackARB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam){
    DebugOutputToFile(source, type, id, severity, message);
}

void CALLBACK DebugCallbackAMD(GLenum source, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam){
    DebugOutputToFileAMD(source, id, severity, message);
}
std::string gl_source;
std::string gl_severity;
std::string gl_type;

void DebugOutputToFileAMD(GLenum source, GLuint id, GLenum severity, const GLchar* message) {
    if(source == GL_DEBUG_CATEGORY_API_ERROR_AMD)
        gl_source = "OpenGL";
    else if(source == GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD)
        gl_source = "Windows";
    else if(source == GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD)
        gl_source = "Shader Compiler";
    else if(source == GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD)
        gl_source = "Undefined Behavior";
    else if(source == GL_DEBUG_CATEGORY_DEPRECATION_AMD)
        gl_source = "Deprecation";
    else if(source == GL_DEBUG_CATEGORY_PERFORMANCE_AMD)
        gl_source = "Performance";
    else if(source == GL_DEBUG_CATEGORY_APPLICATION_AMD)
        gl_source = "Application";
    else if(source == GL_DEBUG_CATEGORY_OTHER_AMD)
        gl_source = "Other";

    if(severity == GL_DEBUG_SEVERITY_HIGH_AMD)
      gl_severity = "High";
    else if(severity == GL_DEBUG_SEVERITY_MEDIUM_AMD)
      gl_severity = "Medium";
    else if(severity == GL_DEBUG_SEVERITY_LOW_AMD)
      gl_severity = "Low";

    ERROR_FN(Locale::get("ERROR_GENERIC_GL_AMD"),gl_source.c_str(),id,gl_severity.c_str(),message);
}

void DebugOutputToFile(GLenum source, GLenum type, GLuint id, GLenum severity, const GLchar* message) {
    if(source == GL_DEBUG_SOURCE_API_ARB)
       gl_source = "OpenGL";
    else if(source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
       gl_source = "Windows";
    else if(source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
       gl_source = "Shader Compiler";
    else if(source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
       gl_source = "Third Party";
    else if(source == GL_DEBUG_SOURCE_APPLICATION_ARB)
       gl_source = "Application";
    else if(source == GL_DEBUG_SOURCE_OTHER_ARB)
       gl_source = "Other";

    if(type == GL_DEBUG_TYPE_ERROR_ARB)
       gl_type = "Error";
    else if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)
      gl_type = "Deprecated behavior";
    else if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
       gl_type = "Undefined behavior";
    else if(type == GL_DEBUG_TYPE_PORTABILITY_ARB)
       gl_type =  "Portability";
    else if(type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
       gl_type = "Performance";
    else if(type == GL_DEBUG_TYPE_OTHER_ARB)
       gl_type =  "Other";

    if(severity == GL_DEBUG_SEVERITY_HIGH_ARB)
       gl_severity = "High";
    else if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
       gl_severity = "Medium";
    else if(severity == GL_DEBUG_SEVERITY_LOW_ARB)
       gl_severity = "Low";

    ERROR_FN(Locale::get("ERROR_GENERIC_GL_ARB"),
             gl_source.c_str(),
             gl_type.c_str(),
             id,
             gl_severity.c_str(),
             message);
}
#endif
    } //namespace GL
}//namespace Divide