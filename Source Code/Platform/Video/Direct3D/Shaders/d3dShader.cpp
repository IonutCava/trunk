#include "Headers/d3dShader.h"

namespace Divide {

IMPLEMENT_ALLOCATOR(d3dShader, 0, 0);
d3dShader::d3dShader(GFXDevice& context,
                     const stringImpl& name,
                     const ShaderType& type,
                     const bool optimise)
    : Shader(context, name, type, optimise)
{
}

d3dShader::~d3dShader()
{
}

bool d3dShader::load(const stringImpl& source) {
    return true;
}

bool d3dShader::compile() {
    return true;
}

bool d3dShader::validate() {
    return true;
}
};
