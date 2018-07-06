#include "Headers/glShader.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"

#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Core/Headers/ParamHandler.h"
#include <glsl/glsl_optimizer.h>
#include <regex>

namespace Divide {

#define _COMPILE_SHADER_OUTPUT_IN_RELEASE

glShader::glShader(const stringImpl& name,
                   const ShaderType& type,
                   const bool optimise)
    : Shader(name, type, optimise) {
    switch (type) {
        default:
            Console::errorfn(Locale::get("ERROR_GLSL_UNKNOWN_ShaderType"),
                             type);
            break;
        case ShaderType::VERTEX_SHADER:
            _shader = glCreateShader(GL_VERTEX_SHADER);
            break;
        case ShaderType::FRAGMENT_SHADER:
            _shader = glCreateShader(GL_FRAGMENT_SHADER);
            break;
        case ShaderType::GEOMETRY_SHADER:
            _shader = glCreateShader(GL_GEOMETRY_SHADER);
            break;
        case ShaderType::TESSELATION_CTRL_SHADER:
            _shader = glCreateShader(GL_TESS_CONTROL_SHADER);
            break;
        case ShaderType::TESSELATION_EVAL_SHADER:
            _shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
            break;
        case ShaderType::COMPUTE_SHADER:
            _shader = glCreateShader(GL_COMPUTE_SHADER);
            break;
    };
}

glShader::~glShader() {
    glDeleteShader(_shader);
}

bool glShader::load(const stringImpl& source) {
    if (source.empty()) {
        Console::errorfn(Locale::get("ERROR_GLSL_SHADER_NOT_FOUND"),
                         getName().c_str());
        return false;
    }
    stringImpl parsedSource = preprocessIncludes(source, getName(), 0);
    Util::trim(parsedSource);

#ifdef NDEBUG

    if ((_type == ShaderType::FRAGMENT_SHADER ||
         _type == ShaderType::VERTEX_SHADER) &&
        _optimise) {
        glslopt_ctx* ctx = GL_API::getInstance().getGLSLOptContext();
        DIVIDE_ASSERT(ctx != nullptr,
                      "glShader error: Invalid shader optimization context!");
        glslopt_shader_type shaderType =
            (_type == ShaderType::FRAGMENT_SHADER ? kGlslOptShaderFragment
                                                  : kGlslOptShaderVertex);
        glslopt_shader* shader =
            glslopt_optimize(ctx, shaderType, parsedSource.c_str(), 0);
        if (glslopt_get_status(shader)) {
            parsedSource = glslopt_get_output(shader);
        } else {
            stringImpl errorLog("\n -- ");
            errorLog.append(glslopt_get_log(shader));
            Console::errorfn(Locale::get("ERROR_GLSL_OPTIMISE"),
                             getName().c_str(), errorLog.c_str());
        }
        glslopt_shader_delete(shader);
    }

#endif
    const char* src = parsedSource.c_str();
    GLsizei sourceLength = (GLsizei)parsedSource.length();
    glShaderSource(_shader, 1, &src, &sourceLength);
#ifndef NDEBUG
    ShaderManager::getInstance().shaderFileWrite(
        (char*)(stringImpl("shaderCache/Text/" + getName()).c_str()), src);
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
#if defined(_DEBUG) || defined(_PROFILE) || \
    defined(COMPILE_SHADER_OUTPUT_IN_RELEASE)
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

    input << source.c_str();

    size_t line_number = 1;
    std::smatch matches;

    std::string line;
    stringImpl include_file, include_string, loc;
    ParamHandler& par = ParamHandler::getInstance();
    stringImpl shaderAtomLocationPrefix(
        par.getParam<stringImpl>("assetsLocation", "assets") + "/" +
        par.getParam<stringImpl>("shaderLocation", "shaders") + "/GLSL/");
    while (std::getline(input, line)) {
        if (std::regex_search(line, matches, re)) {
            include_file = stringAlg::toBase(matches[1].str());

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
            output << stringAlg::fromBase(preprocessIncludes(
                          include_string, include_file, level + 1))
                   << std::endl;
        } else {
            output << line << std::endl;
        }
        ++line_number;
    }
    return stringAlg::toBase(output.str());
}
};