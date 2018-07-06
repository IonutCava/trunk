#include "Headers/Shader.h"
#include "Headers/ShaderProgram.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

Shader::Shader(const stringImpl& name, const ShaderType& type,
               const bool optimise)
    : _shader(std::numeric_limits<U32>::max()),
      _name(name),
      _type(type){
    _compiled = false;
}

Shader::~Shader()
{
    Console::d_printfn(Locale::get("SHADER_DELETE"), getName().c_str());
}

};