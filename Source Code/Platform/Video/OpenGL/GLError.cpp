#include "Headers/GLWrapper.h"

#ifdef ENABLE_GPU_VALIDATION
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
namespace GLUtil {

/// Print OpenGL specific messages
void
DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
              GLsizei length, const GLchar* message, const void* userParam) {

    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
        // Translate message source
        const char* gl_source = "Unknown Source";
        if (source == GL_DEBUG_SOURCE_API) {
            gl_source = "OpenGL";
        } else if (source == GL_DEBUG_SOURCE_WINDOW_SYSTEM) {
            gl_source = "Windows";
        } else if (source == GL_DEBUG_SOURCE_SHADER_COMPILER) {
            gl_source = "Shader Compiler";
        } else if (source == GL_DEBUG_SOURCE_THIRD_PARTY) {
            gl_source = "Third Party";
        } else if (source == GL_DEBUG_SOURCE_APPLICATION) {
            gl_source = "Application";
        } else if (source == GL_DEBUG_SOURCE_OTHER) {
            gl_source = "Other";
        }
        // Translate message type
        const char* gl_type = "Unknown Type";
        if (type == GL_DEBUG_TYPE_ERROR) {
            gl_type = "Error";
        } else if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR) {
            gl_type = "Deprecated behavior";
        } else if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR) {
            gl_type = "Undefined behavior";
        } else if (type == GL_DEBUG_TYPE_PORTABILITY) {
            gl_type = "Portability";
        } else if (type == GL_DEBUG_TYPE_PERFORMANCE) {
            gl_type = "Performance";
        } else if (type == GL_DEBUG_TYPE_OTHER) {
            gl_type = "Other";
        }

        // Translate message severity
        const char* gl_severity = "Unknown Severity";
        if (severity == GL_DEBUG_SEVERITY_HIGH) {
            gl_severity = "High";
        } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
            gl_severity = "Medium";
        } else if (severity == GL_DEBUG_SEVERITY_LOW) {
            gl_severity = "Low";
        } else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
            gl_severity = "Info";
        }
        // Print the message and the details
        Console::errorfn(Util::StringFormat(Locale::get(_ID("ERROR_GENERIC_GL_DEBUG")),
                                            userParam == nullptr
                                                       ? " [Main Thread] "
                                                       : " [Loader Thread] ",
                                            gl_source, gl_type, id, gl_severity, message).c_str());
    }

}
}  // namespace GLUtil
}  // namespace Divide

#endif //ENABLE_GPU_VALIDATION