#include "Headers/Shader.h"
#include "Headers/ShaderProgram.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

const char* Shader::CACHE_LOCATION_TEXT = "shaderCache/Text/";
const char* Shader::CACHE_LOCATION_BIN = "shaderCache/Binary/";

Shader::Shader(GFXDevice& context,
               const stringImpl& name,
               const ShaderType& type,
               const bool optimise)
    : GraphicsResource(context),
      _skipIncludes(false),
      _shader(std::numeric_limits<U32>::max()),
      _name(name),
      _type(type)
{
    _compiled = false;
}

Shader::~Shader()
{
    Console::d_printfn(Locale::get(_ID("SHADER_DELETE")), getName().c_str());
}

};