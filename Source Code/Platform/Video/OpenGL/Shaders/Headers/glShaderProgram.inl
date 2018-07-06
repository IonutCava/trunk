/* Copyright (c) 2016 DIVIDE-Studio
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
inline bool glShaderProgram::cachedValueUpdate(I32 location, const U32& value) {
    if (location == -1 || _shaderProgramID == 0) {
        return false;
    }

    ShaderVarU32Map::iterator it = _shaderVarsU32.find(location);
    if (it != std::end(_shaderVarsU32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } else {
        hashAlg::emplace(_shaderVarsU32, location, value);
    }

    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const I32& value) {
    if (location == -1 || _shaderProgramID == 0) {
        return false;
    }

    ShaderVarI32Map::iterator it = _shaderVarsI32.find(location);
    if (it != std::end(_shaderVarsI32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } else {
        hashAlg::emplace(_shaderVarsI32, location, value);
    }

    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const F32& value) {
    if (location == -1 || _shaderProgramID == 0) {
        return false;
    }

    ShaderVarF32Map::iterator it = _shaderVarsF32.find(location);
    if (it != std::end(_shaderVarsF32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } else {
        hashAlg::emplace(_shaderVarsF32, location, value);
    }

    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const vec2<F32>& value) {
    if (location == -1 || _shaderProgramID == 0) {
        return false;
    }

    ShaderVarVec2F32Map::iterator it = _shaderVarsVec2F32.find(location);
    if (it != std::end(_shaderVarsVec2F32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::emplace(_shaderVarsVec2F32, location, value);
    }

    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const vec2<I32>& value) {
    if (location == -1 || _shaderProgramID == 0) {
        return false;
    }

    ShaderVarvec2I32Map::iterator it = _shaderVarsVec2I32.find(location);
    if (it != std::end(_shaderVarsVec2I32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::emplace(_shaderVarsVec2I32, location, value);
    }

    return true;
}
 
template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const vec3<F32>& value) {
    if (location == -1 || _shaderProgramID == 0) {
        return false;
    }

    ShaderVarVec3F32Map::iterator it = _shaderVarsVec3F32.find(location);
    if (it != std::end(_shaderVarsVec3F32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::emplace(_shaderVarsVec3F32, location, value);
    }

    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const vec3<I32>& value) {
    if (location == -1 || _shaderProgramID == 0) {
        return false;
    }

    ShaderVarVec3I32Map::iterator it = _shaderVarsVec3I32.find(location);
    if (it != std::end(_shaderVarsVec3I32)) {
        if (it->second == value) {
            return false;
        }
        else {
            it->second.set(value);
        }
    } else {
        hashAlg::emplace(_shaderVarsVec3I32, location, value);
    }

    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const vec4<F32>& value) {
    if (location == -1 || _shaderProgramID == 0) {
        return false;
    }

    ShaderVarVec4F32Map::iterator it = _shaderVarsVec4F32.find(location);
    if (it != std::end(_shaderVarsVec4F32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::emplace(_shaderVarsVec4F32, location, value);
    }

    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const vec4<I32>& value) {
    if (location == -1 || _shaderProgramID == 0) {
        return false;
    }

    ShaderVarVec4I32Map::iterator it = _shaderVarsVec4I32.find(location);
    if (it != std::end(_shaderVarsVec4I32)) {
        if (it->second == value) {
            return false;
        }
        else {
            it->second.set(value);
        }
    }
    else {
        hashAlg::emplace(_shaderVarsVec4I32, location, value);
    }

    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const mat3<F32>& value) {
    if (location == -1 || _shaderProgramID == 0) {
        return false;
    }

    ShaderVarMat3Map::iterator it = _shaderVarsMat3.find(location);
    if (it != std::end(_shaderVarsMat3)) {
        if (it->second == value) {
            return false;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::emplace(_shaderVarsMat3, location, value);
    }

    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const mat4<F32>& value) {
    if (location == -1 || _shaderProgramID == 0) {
        return false;
    }

    ShaderVarMat4Map::iterator it = _shaderVarsMat4.find(location);
    if (it != std::end(_shaderVarsMat4)) {
        if (it->second == value) {
            return false;
        } else {
            it->second.set(value);
        }
    } else {
        hashAlg::emplace(_shaderVarsMat4, location, value);
    }

    return true;
}


void glShaderProgram::Uniform(const char* ext, U32 value) {
    Uniform(getUniformLocation(ext), value);
}

void glShaderProgram::Uniform(const char* ext, I32 value) {
    Uniform(getUniformLocation(ext), value);
}

void glShaderProgram::Uniform(const char* ext, F32 value) {
    Uniform(getUniformLocation(ext), value);
}

void glShaderProgram::Uniform(const char* ext, const vec2<F32>& value) {
    Uniform(getUniformLocation(ext), value);
}

void glShaderProgram::Uniform(const char* ext, const vec2<I32>& value) {
    Uniform(getUniformLocation(ext), value);
}

void glShaderProgram::Uniform(const char* ext, const vec3<F32>& value) {
    Uniform(getUniformLocation(ext), value);
}

void glShaderProgram::Uniform(const char* ext, const vec3<I32>& value) {
    Uniform(getUniformLocation(ext), value);
}

void glShaderProgram::Uniform(const char* ext, const vec4<F32>& value) {
    Uniform(getUniformLocation(ext), value);
}

void glShaderProgram::Uniform(const char* ext, const vec4<I32>& value) {
    Uniform(getUniformLocation(ext), value);
}

void glShaderProgram::Uniform(const char* ext, const mat3<F32>& value, bool transpose) {
    Uniform(getUniformLocation(ext), value, transpose);
}

void glShaderProgram::Uniform(const char* ext, const mat4<F32>& value, bool transpose) {
    Uniform(getUniformLocation(ext), value, transpose);
}

void glShaderProgram::Uniform(const char* ext, const vectorImpl<I32>& values) {
    Uniform(getUniformLocation(ext), values);
}

void glShaderProgram::Uniform(const char* ext, const vectorImpl<F32>& values) {
    Uniform(getUniformLocation(ext), values);
}

void glShaderProgram::Uniform(const char* ext, const vectorImpl<vec2<F32> >& values) {
    Uniform(getUniformLocation(ext), values);
}

void glShaderProgram::Uniform(const char* ext, const vectorImpl<vec3<F32> >& values) {
    Uniform(getUniformLocation(ext), values);
}

void glShaderProgram::Uniform(const char* ext, const vectorImplBest<vec4<F32> >& values) {
    Uniform(getUniformLocation(ext), values);
}

void glShaderProgram::Uniform(const char* ext, const vectorImpl<mat3<F32> >& values, bool transpose) {
    Uniform(getUniformLocation(ext), values, transpose);
}

void glShaderProgram::Uniform(const char* ext, const vectorImplBest<mat4<F32> >& values, bool transpose) {
    Uniform(getUniformLocation(ext), values, transpose);
}

}; //namespace Divide

#endif //_PLATFORM_VIDEO_OPENGLS_PROGRAM_INL_
