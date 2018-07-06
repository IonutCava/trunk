#include "Headers/glShader.h"
#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"
#include "Hardware/Video/OpenGL/Headers/glResources.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <glsl/glsl_optimizer.h>
#include "Hardware/Video/Shaders/Headers/ShaderManager.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {

#define _COMPILE_SHADER_OUTPUT_IN_RELEASE

glShader::glShader(const std::string& name, const ShaderType& type, const bool optimise) : Shader(name, type, optimise)
{
    switch (type) {
        default:      ERROR_FN(Locale::get("ERROR_GLSL_UNKNOWN_ShaderType"),type);     break;
        case VERTEX_SHADER   : _shader = glCreateShader(GL_VERTEX_SHADER);    break;
        case FRAGMENT_SHADER : _shader = glCreateShader(GL_FRAGMENT_SHADER);  break;
        case GEOMETRY_SHADER : _shader = glCreateShader(GL_GEOMETRY_SHADER);  break;
        case TESSELATION_CTRL_SHADER: _shader = glCreateShader(GL_TESS_CONTROL_SHADER); break;
        case TESSELATION_EVAL_SHADER: _shader = glCreateShader(GL_TESS_EVALUATION_SHADER); break;
        case COMPUTE_SHADER: _shader = glCreateShader(GL_COMPUTE_SHADER); break;
    };
}

glShader::~glShader()
{
    glDeleteShader(_shader);
}

bool glShader::load(const std::string& source){
    if(source.empty()){
        ERROR_FN(Locale::get("ERROR_GLSL_SHADER_NOT_FOUND"),getName().c_str());
        return false;
    }
    std::string parsedSource = preprocessIncludes(source,getName(),0);
    boost::trim(parsedSource);
    
#ifdef NDEBUG

    if((_type == FRAGMENT_SHADER || _type == VERTEX_SHADER) && _optimise){
        glslopt_ctx* ctx = GL_API::getInstance().getGLSLOptContext();
        DIVIDE_ASSERT(ctx != nullptr, "glShader error: Invalid shader optimization context!");
        glslopt_shader_type shaderType = (_type == FRAGMENT_SHADER ? kGlslOptShaderFragment : kGlslOptShaderVertex);
        glslopt_shader* shader = glslopt_optimize (ctx, shaderType, parsedSource.c_str(), 0);
        if (glslopt_get_status (shader)) {
            parsedSource = glslopt_get_output(shader);
        } else {
            std::string errorLog("\n -- "); errorLog.append(glslopt_get_log(shader));
            ERROR_FN(Locale::get("ERROR_GLSL_OPTIMISE"), getName().c_str(), errorLog.c_str());
        }
        glslopt_shader_delete (shader);
    }

#endif
    const char* src = parsedSource.c_str();
    GLsizei sourceLength = (GLsizei)parsedSource.length();
    glShaderSource(_shader, 1, &src, &sourceLength);
#ifndef NDEBUG
    ShaderManager::getInstance().shaderFileWrite((char*)(std::string("shaderCache/Text/"+getName()).c_str()), src);
#endif
    return true;
}

bool glShader::compile(){
    if (_compiled) {
        return true;
    }

    glCompileShader(_shader);

    _compiled = true;

    return _compiled;
}

void glShader::validate() {
#if defined(_DEBUG) || defined(_PROFILE) || defined(COMPILE_SHADER_OUTPUT_IN_RELEASE)
    GLint length = 0, status = 0;

    glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);
    glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &length);
    if(length <= 1) return;
    vectorImpl<char> shaderLog(length);
    glGetShaderInfoLog(_shader, length, NULL, &shaderLog[0]);
    shaderLog.push_back('\n');
    if(status == GL_FALSE){
        ERROR_FN(Locale::get("GLSL_VALIDATING_SHADER"), _name.c_str(),&shaderLog[0]);
    }else{
        D_PRINT_FN(Locale::get("GLSL_VALIDATING_SHADER"), _name.c_str(),&shaderLog[0]);
    }
#endif
}

std::string glShader::preprocessIncludes( const std::string& source, const std::string& filename, I32 level /*= 0 */ ){
    if(level > 32){
        ERROR_FN(Locale::get("ERROR_GLSL_INCLUD_LIMIT"));
    }

    static const boost::regex re("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
    std::stringstream input, output;

    input << source;

    size_t line_number = 1;
    boost::smatch matches;

    std::string line, include_file, include_string, loc;
    ParamHandler& par = ParamHandler::getInstance();
    std::string shaderAtomLocationPrefix = par.getParam<std::string>("assetsLocation") + "/" +
                                           par.getParam<std::string>("shaderLocation")+"/GLSL/";
    while(std::getline(input,line))	{
        if (boost::regex_search(line, matches, re))	{
            include_file = matches[1];

            if(include_file.find("frag") != std::string::npos){
                loc =  "fragmentAtoms";
            }else if(include_file.find("vert") != std::string::npos){
                loc =  "vertexAtoms";
            }else if(include_file.find("geom") != std::string::npos){
                loc =  "geometryAtoms";
            }else if(include_file.find("tesc") != std::string::npos){
                loc =  "tessellationCAtoms";
            }else if (include_file.find("tese") != std::string::npos){
                loc = "tessellationEAtoms";
            }else if (include_file.find("cpt") != std::string::npos){
                loc = "computeAtoms";
            }else if(include_file.find("cmn") != std::string::npos){
                loc = "common";
            }

            include_string = ShaderManager::getInstance().shaderFileRead(include_file,shaderAtomLocationPrefix+loc);

            if(include_string.empty()){
                ERROR_FN(Locale::get("ERROR_GLSL_NO_INCLUDE_FILE"),getName().c_str(), line_number, include_file.c_str());
            }
            output << preprocessIncludes(include_string, include_file, level + 1) << std::endl;
        }else{
            output <<  line << std::endl;
        }
        ++line_number;
    }
    return output.str();
}

};