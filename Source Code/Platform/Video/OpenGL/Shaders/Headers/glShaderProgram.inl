/* Copyright (c) 2017 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _PLATFORM_VIDEO_OPENGLS_PROGRAM_INL_
#define _PLATFORM_VIDEO_OPENGLS_PROGRAM_INL_

namespace Divide {

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const U32& value) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarU32Map::iterator it = _uniformsByName._shaderVarsU32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsU32)) {
        if (it->second == value) {
            return -1;
        } else {
            it->second = value;
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsU32, location, value);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const I32& value) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarI32Map::iterator it = _uniformsByName._shaderVarsI32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsI32)) {
        if (it->second == value) {
            return -1;
        } else {
            it->second = value;
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsI32, location, value);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const F32& value) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarF32Map::iterator it = _uniformsByName._shaderVarsF32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsF32)) {
        if (COMPARE(it->second, value)) {
            return -1;
        } else {
            it->second = value;
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsF32, location, value);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vec2<F32>& value) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVec2F32Map::iterator it = _uniformsByName._shaderVarsVec2F32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVec2F32)) {
        if (it->second == value) {
            return -1;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVec2F32, location, value);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vec2<I32>& value) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarvec2I32Map::iterator it = _uniformsByName._shaderVarsVec2I32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVec2I32)) {
        if (it->second == value) {
            return -1;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVec2I32, location, value);
    }

    return binding;
}
 
template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vec3<F32>& value) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVec3F32Map::iterator it = _uniformsByName._shaderVarsVec3F32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVec3F32)) {
        if (it->second == value) {
            return -1;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVec3F32, location, value);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vec3<I32>& value) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVec3I32Map::iterator it = _uniformsByName._shaderVarsVec3I32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVec3I32)) {
        if (it->second == value) {
            return -1;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVec3I32, location, value);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vec4<F32>& value) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVec4F32Map::iterator it = _uniformsByName._shaderVarsVec4F32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVec4F32)) {
        if (it->second == value) {
            return -1;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVec4F32, location, value);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vec4<I32>& value) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVec4I32Map::iterator it = _uniformsByName._shaderVarsVec4I32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVec4I32)) {
        if (it->second == value) {
            return -1;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVec4I32, location, value);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const mat3<F32>& value) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarMat3Map::iterator it = _uniformsByName._shaderVarsMat3.find(location);
    if (it != std::end(_uniformsByName._shaderVarsMat3)) {
        if (it->second == value) {
            return -1;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsMat3, location, value);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const mat4<F32>& value) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarMat4Map::iterator it = _uniformsByName._shaderVarsMat4.find(location);
    if (it != std::end(_uniformsByName._shaderVarsMat4)) {
        if (it->second == value) {
            return -1;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsMat4, location, value);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vectorImpl<I32>& values) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVectorI32Map::iterator it = _uniformsByName._shaderVarsVectorI32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVectorI32)) {
        if (it->second == values) {
            return -1;
        } else {
            it->second = values;
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVectorI32, location, values);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vectorImpl<F32>& values) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVectorF32Map::iterator it = _uniformsByName._shaderVarsVectorF32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVectorF32)) {
        if (it->second == values) {
            return -1;
        } else {
            it->second = values;
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVectorF32, location, values);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vectorImpl<vec2<F32>>& values) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVectorVec2F32Map::iterator it = _uniformsByName._shaderVarsVectorVec2F32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVectorVec2F32)) {
        if (it->second == values) {
            return -1;
        } else {
            it->second = values;
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVectorVec2F32, location, values);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vectorImpl<vec3<F32>>& values) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVectorVec3F32Map::iterator it = _uniformsByName._shaderVarsVectorVec3F32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVectorVec3F32)) {
        if (it->second == values) {
            return -1;
        } else {
            it->second = values;
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVectorVec3F32, location, values);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vectorImpl<vec4<F32>>& values) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVectorVec4F32Map::iterator it = _uniformsByName._shaderVarsVectorVec4F32.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVectorVec4F32)) {
        if (it->second == values) {
            return -1;
        } else {
            it->second = values;
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVectorVec4F32, location, values);
    }

    return binding;
}


template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vectorImpl<mat3<F32>>& values) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVectorMat3Map::iterator it = _uniformsByName._shaderVarsVectorMat3.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVectorMat3)) {
        if (it->second == values) {
            return -1;
        } else {
            it->second = values;
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVectorMat3, location, values);
    }

    return binding;
}

template <>
inline I32 glShaderProgram::cachedValueUpdate(const stringImplFast& location, const vectorImpl<mat4<F32>>& values) {
    I32 binding = Binding(location.c_str());

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByName::ShaderVarVectorMat4Map::iterator it = _uniformsByName._shaderVarsVectorMat4.find(location);
    if (it != std::end(_uniformsByName._shaderVarsVectorMat4)) {
        if (it->second == values) {
            return -1;
        } else {
            it->second = values;
        }
    } else {
        hashAlg::insert(_uniformsByName._shaderVarsVectorMat4, location, values);
    }

    return binding;
}
}; //namespace Divide

#endif //_PLATFORM_VIDEO_OPENGLS_PROGRAM_INL_
