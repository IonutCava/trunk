#include "Headers/ShaderProgramInfo.h"

namespace Divide {

ShaderProgramInfo::ShaderProgramInfo()
{
    _customShader = false;
    _shaderRef = nullptr;
    _shader.clear();
    _shaderCompStage = BuildStage::COUNT;
}

ShaderProgramInfo::ShaderProgramInfo(const ShaderProgramInfo& other)
  : _customShader(other._customShader),
    _shaderRef(other._shaderRef),
    _shader(other._shader),
    _shaderDefines(other._shaderDefines)
{
    _shaderCompStage.store(other._shaderCompStage);
}

ShaderProgramInfo& ShaderProgramInfo::operator=(const ShaderProgramInfo& other) {
    _customShader = other._customShader;
    _shaderRef = other._shaderRef;
    _shader = other._shader;
    _shaderCompStage.store(other._shaderCompStage);
    _shaderDefines = other._shaderDefines;
    return *this;
}

const ShaderProgram_ptr& ShaderProgramInfo::getProgram() const {
    return _shaderRef == nullptr ? ShaderProgram::defaultShader()
                                 : _shaderRef;
}

}; //namespace Divide