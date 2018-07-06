#include "Headers/Shader.h"
#include "Headers/ShaderProgram.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

Shader::Shader(const stringImpl& name, const ShaderType& type,const bool optimise) : _shader(std::numeric_limits<U32>::max()),
                                                                                      _name(name),
                                                                                      _type(type),
                                                                                      _compiled(false),
                                                                                      _optimise(optimise)
{
}

Shader::~Shader()
{
    D_PRINT_FN(Locale::get("SHADER_DELETE"),getName().c_str());
    // never delete a shader if it's still in use by a program
    assert(_parentShaderPrograms.empty());
}

/// Register the given shader program with this shader
void Shader::addParentProgram(ShaderProgram* const shaderProgram) {
    // simple, handle-base check
    U32 parentShaderID = shaderProgram->getId();
    vectorImpl<ShaderProgram* >::iterator it;
    it = std::find_if(_parentShaderPrograms.begin(), _parentShaderPrograms.end(), [&parentShaderID](ShaderProgram const* sp) { 
                                                                                        return sp->getId() == parentShaderID; 
                                                                                    });
    // assert if we register the same shaderProgram with the same shader multiple times
    assert(it == _parentShaderPrograms.end());
    // actually register the shader
    _parentShaderPrograms.push_back(shaderProgram);
}

/// Unregister the given shader program from this shader
void Shader::removeParentProgram(ShaderProgram* const shaderProgram) {
    // program matching works like in 'addParentProgram'
    U32 parentShaderID = shaderProgram->getId();

    vectorImpl<ShaderProgram* >::iterator it;
    it = std::find_if(_parentShaderPrograms.begin(), _parentShaderPrograms.end(), [&parentShaderID](ShaderProgram const* sp) { 
                                                                                        return sp->getId() == parentShaderID; 
                                                                                    });
    // assert if the specified shader program wasn't registered
    assert(it != _parentShaderPrograms.end());
    // actually unregister the shader
    _parentShaderPrograms.erase(it);
}

};