#include "stdafx.h"

#include "Headers/glShaderProgram.h"

namespace Divide {
    using namespace std;
    SharedMutex glShaderProgram::s_atomLock;
    ShaderProgram::AtomMap glShaderProgram::s_atoms;
    ShaderProgram::AtomInclusionMap glShaderProgram::s_atomIncludes;

    I64 glShaderProgram::s_shaderFileWatcherID = -1;
    ResourcePath glShaderProgram::shaderAtomLocationPrefix[to_base(ShaderType::COUNT) + 1];
    U64 glShaderProgram::shaderAtomExtensionHash[to_base(ShaderType::COUNT) + 1];
    Str8 glShaderProgram::shaderAtomExtensionName[to_base(ShaderType::COUNT) + 1];

}//namespace Divide
