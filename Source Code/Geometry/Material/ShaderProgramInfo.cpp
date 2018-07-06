#include "stdafx.h"

#include "Headers/ShaderProgramInfo.h"

namespace Divide {

ShaderProgramInfo::ShaderProgramInfo()
    : _customShader(false),
      _shaderRef(ShaderProgram::defaultShader()),
      _shaderCompStage(BuildStage::COUNT)
{
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

bool ShaderProgramInfo::update() {
    if (computeStage() == ShaderProgramInfo::BuildStage::COMPUTED) {
        if (_shaderRef->getState() == ResourceState::RES_LOADED) {
            computeStage(ShaderProgramInfo::BuildStage::READY);
            return true;
        }
    }

    return false;
}

const ShaderProgram_ptr& ShaderProgramInfo::getProgram() const {
    return _shaderRef;
}

ShaderProgramInfo::BuildStage ShaderProgramInfo::computeStage() const {
    return _shaderCompStage;
}

void ShaderProgramInfo::computeStage(BuildStage stage) {
    _shaderCompStage = stage;
}

}; //namespace Divide