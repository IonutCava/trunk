#include "Headers/glShader.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"

#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Core/Headers/ParamHandler.h"
#include <regex>

namespace Divide {

glShader::glShader(const stringImpl& name,
                   const ShaderType& type,
                   const bool optimise)
    : Shader(name, type, optimise) {
    switch (type) {
        default:
            Console::errorfn(Locale::get("ERROR_GLSL_UNKNOWN_ShaderType"),
                             type);
            break;
        case ShaderType::VERTEX:
            _shader = glCreateShader(GL_VERTEX_SHADER);
            break;
        case ShaderType::FRAGMENT:
            _shader = glCreateShader(GL_FRAGMENT_SHADER);
            break;
        case ShaderType::GEOMETRY:
            _shader = glCreateShader(GL_GEOMETRY_SHADER);
            break;
        case ShaderType::TESSELATION_CTRL:
            _shader = glCreateShader(GL_TESS_CONTROL_SHADER);
            break;
        case ShaderType::TESSELATION_EVAL:
            _shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
            break;
        case ShaderType::COMPUTE:
            _shader = glCreateShader(GL_COMPUTE_SHADER);
            break;
    };
}

glShader::~glShader() {
    glDeleteShader(_shader);
}

bool glShader::load(const stringImpl& source) {
    if (source.empty()) {
        Console::errorfn(Locale::get("ERROR_GLSL_NOT_FOUND"),
                         getName().c_str());
        return false;
    }
    stringImpl parsedSource = preprocessIncludes(source, getName(), 0);
    const char* src = Util::Trim(parsedSource).c_str();

    GLsizei sourceLength = (GLsizei)parsedSource.length();
    glShaderSource(_shader, 1, &src, &sourceLength);
#if defined(_DEBUG)
    ShaderManager::getInstance().shaderFileWrite(
        "shaderCache/Text/" + getName(), parsedSource);
#endif
    return true;
}

bool glShader::compile() {
    if (_compiled) {
        return true;
    }

    glCompileShader(_shader);

    _compiled = true;

    return _compiled;
}

void glShader::validate() {
#if defined(ENABLE_GPU_VALIDATION)
    GLint length = 0, status = 0;

    glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);
    glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1)
        return;
    vectorImpl<char> shaderLog(length);
    glGetShaderInfoLog(_shader, length, NULL, &shaderLog[0]);
    shaderLog.push_back('\n');
    if (status == 0) {
        Console::errorfn(Locale::get("GLSL_VALIDATING_SHADER"), _name.c_str(),
                         &shaderLog[0]);
    } else {
        Console::d_printfn(Locale::get("GLSL_VALIDATING_SHADER"), _name.c_str(),
                           &shaderLog[0]);
    }
#endif
}

stringImpl glShader::preprocessIncludes(const stringImpl& source,
                                        const stringImpl& filename,
                                        I32 level /*= 0 */) {
    if (level > 32) {
        Console::errorfn(Locale::get("ERROR_GLSL_INCLUD_LIMIT"));
    }

    static const std::regex re("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
    std::stringstream input, output;

    input << source;

    size_t line_number = 1;
    std::smatch matches;

    stringImpl line;
    stringImpl include_file, include_string, loc;
    ParamHandler& par = ParamHandler::getInstance();
    stringImpl shaderAtomLocationPrefix(
        par.getParam<stringImpl>("assetsLocation", "assets") + "/" +
        par.getParam<stringImpl>("shaderLocation", "shaders") + "/GLSL/");
    while (std::getline(input, line)) {
        if (std::regex_search(line, matches, re)) {
            include_file = matches[1].str();

            if (include_file.find("frag") != stringImpl::npos) {
                loc = "fragmentAtoms";
            } else if (include_file.find("vert") != stringImpl::npos) {
                loc = "vertexAtoms";
            } else if (include_file.find("geom") != stringImpl::npos) {
                loc = "geometryAtoms";
            } else if (include_file.find("tesc") != stringImpl::npos) {
                loc = "tessellationCAtoms";
            } else if (include_file.find("tese") != stringImpl::npos) {
                loc = "tessellationEAtoms";
            } else if (include_file.find("cpt") != stringImpl::npos) {
                loc = "computeAtoms";
            } else if (include_file.find("cmn") != stringImpl::npos) {
                loc = "common";
            }

            include_string = ShaderManager::getInstance().shaderFileRead(
                include_file, shaderAtomLocationPrefix + loc);

            if (include_string.empty()) {
                Console::errorfn(Locale::get("ERROR_GLSL_NO_INCLUDE_FILE"),
                                 getName().c_str(), line_number,
                                 include_file.c_str());
            }
            output << preprocessIncludes(include_string, include_file, level + 1) << "\n";
        } else {
            output << line << "\n";
        }
        ++line_number;
    }
    return output.str();
}
};