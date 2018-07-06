#include "Headers/glShader.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"

#include "Core/Headers/ParamHandler.h"
#include <regex>

namespace Divide {

namespace {
    // these must match the last 4 characters of the atom file
    ULL fragAtomExt = _ID("frag");
    ULL vertAtomExt = _ID("vert");
    ULL geomAtomExt = _ID("geom");
    ULL tescAtomExt = _ID("tesc");
    ULL teseAtomExt = _ID("tese");
    ULL compAtomExt = _ID(".cpt");
    ULL comnAtomExt = _ID(".cmn");
    // Shader subfolder name that contains shader files for OpenGL
    const char* parentShaderLoc = "GLSL";
    // Atom folder names in parent shader folder
    const char* fragAtomLoc = "fragmentAtoms";
    const char* vertAtomLoc = "vertexAtoms";
    const char* geomAtomLoc = "geometryAtoms";
    const char* tescAtomLoc = "tessellationCAtoms";
    const char* teseAtomLoc = "tessellationEAtoms";
    const char* compAtomLoc = "computeAtoms";
    const char* comnAtomLoc = "common";
    // include command regex pattern
    const std::regex includePattern("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");

};

stringImpl glShader::shaderAtomLocationPrefix[to_const_uint(ShaderType::COUNT) + 1];

IMPLEMENT_ALLOCATOR(glShader, 0, 0);
glShader::glShader(GFXDevice& context,
                   const stringImpl& name,
                   const ShaderType& type,
                   const bool optimise)
    : Shader(context, name, type, optimise) {
    switch (type) {
        default:
            Console::errorfn(Locale::get(_ID("ERROR_GLSL_UNKNOWN_ShaderType")),
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

    if (shaderAtomLocationPrefix[to_const_uint(ShaderType::VERTEX)].empty()) {
        ParamHandler& par = ParamHandler::instance();
        stringImpl locPrefix = par.getParam<stringImpl>(_ID("assetsLocation"), "assets") +
                               "/" +
                               par.getParam<stringImpl>(_ID("shaderLocation"), "shaders") +
                               "/" + 
                               parentShaderLoc +
                               "/";

        shaderAtomLocationPrefix[to_const_uint(ShaderType::FRAGMENT)] = locPrefix + fragAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::VERTEX)] = locPrefix + vertAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::GEOMETRY)] = locPrefix + geomAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::TESSELATION_CTRL)] = locPrefix + tescAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::TESSELATION_EVAL)] = locPrefix + teseAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::COMPUTE)] = locPrefix + compAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::COUNT)] = locPrefix + comnAtomLoc;
    }
}

glShader::~glShader() {
    glDeleteShader(_shader);
}

bool glShader::load(const stringImpl& source) {
    if (source.empty()) {
        Console::errorfn(Locale::get(_ID("ERROR_GLSL_NOT_FOUND")),
                         getName().c_str());
        return false;
    }

    stringImpl parsedSource = _skipIncludes ? source
                                            : preprocessIncludes(source, getName(), 0);

    const char* src = parsedSource.c_str();

    GLsizei sourceLength = (GLsizei)parsedSource.length();
    glShaderSource(_shader, 1, &src, &sourceLength);

    if (!_skipIncludes) {
        ShaderProgram::shaderFileWrite(Shader::CACHE_LOCATION_TEXT + getName(), src);
    }

    return true;
}

bool glShader::compile() {
    if (_compiled) {
        return true;
    }

    glCompileShader(_shader);
    validate();

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
        Console::errorfn(Locale::get(_ID("GLSL_VALIDATING_SHADER")), _name.c_str(),
                         &shaderLog[0]);
    } else {
        Console::d_printfn(Locale::get(_ID("GLSL_VALIDATING_SHADER")), _name.c_str(),
                           &shaderLog[0]);
    }
#endif
}

stringImpl glShader::preprocessIncludes(const stringImpl& source,
                                        const stringImpl& filename,
                                        I32 level /*= 0 */) {
    if (level > 32) {
        Console::errorfn(Locale::get(_ID("ERROR_GLSL_INCLUD_LIMIT")));
    }

    size_t line_number = 1;
    std::smatch matches;

    stringImpl output, line;
    stringImpl include_file, include_string;

    istringstreamImpl input(source);
    while (std::getline(input, line)) {
        if (std::regex_search(line, matches, includePattern)) {
            include_file = matches[1].str().c_str();

            I32 index = -1;
            // switch will throw warnings due to promotion to int
            ULL extHash = _ID_RT(Util::GetTrailingCharacters(include_file, 4));
            if (extHash == fragAtomExt) {
                index = to_const_int(ShaderType::FRAGMENT);
            } else if (extHash == vertAtomExt) {
                index = to_const_int(ShaderType::VERTEX); 
            } else if (extHash == geomAtomExt) {
                index = to_const_int(ShaderType::GEOMETRY);
            } else if (extHash == tescAtomExt) {
                index = to_const_int(ShaderType::TESSELATION_CTRL);
            } else if (extHash == teseAtomExt) {
                index = to_const_int(ShaderType::TESSELATION_EVAL);
            } else if (extHash == compAtomExt) {
                index = to_const_int(ShaderType::COMPUTE);
            } else if (extHash == comnAtomExt) {
                index = to_const_int(ShaderType::COUNT);
            }

            assert(index != -1);

            include_string = ShaderProgram::shaderFileRead(include_file, shaderAtomLocationPrefix[index]);

            if (include_string.empty()) {
                Console::errorfn(Locale::get(_ID("ERROR_GLSL_NO_INCLUDE_FILE")),
                                 getName().c_str(), line_number,
                                 include_file.c_str());
            }
            output.append(preprocessIncludes(include_string, include_file, level + 1));
        } else {
            output.append(line);
        }
        output.append("\n");
        ++line_number;
    }

    return output;
}
};