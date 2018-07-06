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

#include "Core/Headers/Console.h"

namespace Divide {
    namespace {
        bool compare(PushConstantType type, const vectorImplFast<AnyParam>& lhsVec, const vectorImplFast<AnyParam>& rhsVec) {
            for (vectorAlg::vecSize i = 0; i < lhsVec.size(); ++i) {
                const AnyParam& lhs = lhsVec[i];
                const AnyParam& rhs = rhsVec[i];

                switch (type) {
                    case PushConstantType::BOOL:  return lhs.constant_cast<bool>() == rhs.constant_cast<bool>();
                    case PushConstantType::INT:   return lhs.constant_cast<I32>() == rhs.constant_cast<I32>();
                    case PushConstantType::UINT:  return lhs.constant_cast<U32>() == rhs.constant_cast<U32>();
                    case PushConstantType::DOUBLE:return lhs.constant_cast<D64>() == rhs.constant_cast<D64>();
                    case PushConstantType::FLOAT: return lhs.constant_cast<F32>() == rhs.constant_cast<F32>();
                    case PushConstantType::IVEC2: return lhs.constant_cast<vec2<I32>>() == rhs.constant_cast<vec2<I32>>();
                    case PushConstantType::IVEC3: return lhs.constant_cast<vec3<I32>>() == rhs.constant_cast<vec3<I32>>();
                    case PushConstantType::IVEC4: return lhs.constant_cast<vec4<I32>>() == rhs.constant_cast<vec4<I32>>();
                    case PushConstantType::UVEC2: return lhs.constant_cast<vec2<U32>>() == rhs.constant_cast<vec2<U32>>();
                    case PushConstantType::UVEC3: return lhs.constant_cast<vec3<U32>>() == rhs.constant_cast<vec3<U32>>();
                    case PushConstantType::UVEC4: return lhs.constant_cast<vec4<U32>>() == rhs.constant_cast<vec4<U32>>();
                    case PushConstantType::DVEC2: return lhs.constant_cast<vec2<D64>>() == rhs.constant_cast<vec2<D64>>();
                    case PushConstantType::VEC2:  return lhs.constant_cast<vec2<F32>>() == rhs.constant_cast<vec2<F32>>();
                    case PushConstantType::DVEC3: return lhs.constant_cast<vec3<D64>>() == rhs.constant_cast<vec3<D64>>();
                    case PushConstantType::VEC3:  return lhs.constant_cast<vec3<F32>>() == rhs.constant_cast<vec3<F32>>();
                    case PushConstantType::DVEC4: return lhs.constant_cast<vec4<D64>>() == rhs.constant_cast<vec4<D64>>();
                    case PushConstantType::VEC4:  return lhs.constant_cast<vec4<F32>>() == rhs.constant_cast<vec4<F32>>();
                    case PushConstantType::IMAT2: return lhs.constant_cast<mat2<I32>>() == rhs.constant_cast<mat2<I32>>();
                    case PushConstantType::IMAT3: return lhs.constant_cast<mat3<I32>>() == rhs.constant_cast<mat3<I32>>();
                    case PushConstantType::IMAT4: return lhs.constant_cast<mat4<I32>>() == rhs.constant_cast<mat4<I32>>();
                    case PushConstantType::UMAT2: return lhs.constant_cast<mat2<U32>>() == rhs.constant_cast<mat2<U32>>();
                    case PushConstantType::UMAT3: return lhs.constant_cast<mat3<U32>>() == rhs.constant_cast<mat3<U32>>();
                    case PushConstantType::UMAT4: return lhs.constant_cast<mat4<U32>>() == rhs.constant_cast<mat4<U32>>();
                    case PushConstantType::MAT2:  return lhs.constant_cast<mat2<F32>>() == rhs.constant_cast<mat2<F32>>();
                    case PushConstantType::MAT3:  return lhs.constant_cast<mat3<F32>>() == rhs.constant_cast<mat3<F32>>();
                    case PushConstantType::MAT4:  return lhs.constant_cast<mat4<F32>>() == rhs.constant_cast<mat4<F32>>();
                    case PushConstantType::DMAT2: return lhs.constant_cast<mat2<D64>>() == rhs.constant_cast<mat2<D64>>();
                    case PushConstantType::DMAT3: return lhs.constant_cast<mat3<D64>>() == rhs.constant_cast<mat3<D64>>();
                    case PushConstantType::DMAT4: return lhs.constant_cast<mat4<D64>>() == rhs.constant_cast<mat4<D64>>();
                };
            }

            return false;
        }

        template<typename T>
        typename std::enable_if<!std::is_same<T, bool>::value, void>::type
        convert(const vectorImplFast<AnyParam>& valuesIn, vectorImpl<T>& valuesOut) {
            std::transform(std::cbegin(valuesIn), std::cend(valuesIn), std::begin(valuesOut), [](const AnyParam& val)
            {
                return val.constant_cast<T>();
            });
        }

        template<typename T>
        typename std::enable_if<std::is_same<T, bool>::value, void>::type
        convert(const vectorImplFast<AnyParam>& valuesIn, vectorImpl<T>& valuesOut) {
            std::transform(std::cbegin(valuesIn), std::cend(valuesIn), std::begin(valuesOut), [](const AnyParam& val)
            {
                return val.constant_cast<bool>() ? 1 : 0;
            });
        }

        //ToDo: REALLY SLOW! Find a faster way of handling this! -Ionut
        template<typename T_out, typename T_in>
        struct castData {
            castData(const vectorImplFast<AnyParam>& values)
                : _convertedData(values.size()),
                  _values(values)
            {
                convert(_values, _convertedData);
            }

            T_out* operator()() {
                return (T_out*)(_convertedData.data());
            }

            using vectorType = typename std::conditional<std::is_same<T_in, bool>::value, I32, T_in>::type;
            vectorImpl<vectorType> _convertedData;

            const vectorImplFast<AnyParam>& _values;
        };
    };

    inline bool glShaderProgram::comparePushConstants(const PushConstant& lhs, const PushConstant& rhs) const {
        if (lhs._type == rhs._type) {
            if (lhs._binding == rhs._binding) {
                if (lhs._flag == rhs._flag) {
                    return compare(lhs._type, lhs._values, rhs._values);
                }
            }
        }

        return false;
    }

    void glShaderProgram::Uniform(I32 binding, PushConstantType type, const vectorImplFast<AnyParam>& values, bool flag) const {
        GLsizei count = (GLsizei)values.size();
        switch (type) {
            case PushConstantType::BOOL:
                glProgramUniform1iv(_shaderProgramID, binding, count, castData<GLint, bool>(values)());
                break;
            case PushConstantType::INT:
                glProgramUniform1iv(_shaderProgramID, binding, count, castData<GLint, I32>(values)());
                break;
            case PushConstantType::UINT:
                glProgramUniform1uiv(_shaderProgramID, binding, count, castData<GLuint, U32>(values)());
                break;
            case PushConstantType::DOUBLE:           // Warning! Downcasting to float -Ionut
                glProgramUniform1fv(_shaderProgramID, binding, count, castData<GLfloat, D64>(values)());
                break;
            case PushConstantType::FLOAT:
                glProgramUniform1fv(_shaderProgramID, binding, count, castData<GLfloat, F32>(values)());
                break;
            case PushConstantType::IVEC2:
                glProgramUniform2iv(_shaderProgramID, binding, count, castData<GLint, vec2<I32>>(values)());
                break;
            case PushConstantType::IVEC3:
                glProgramUniform3iv(_shaderProgramID, binding, count, castData<GLint, vec3<I32>>(values)());
                break;
            case PushConstantType::IVEC4:
                glProgramUniform4iv(_shaderProgramID, binding, count, castData<GLint, vec4<I32>>(values)());
                break;
            case PushConstantType::UVEC2:
                glProgramUniform2uiv(_shaderProgramID, binding, count, castData<GLuint, vec2<U32>>(values)());
                break;
            case PushConstantType::UVEC3:
                glProgramUniform3uiv(_shaderProgramID, binding, count, castData<GLuint, vec3<U32>>(values)());
                break;
            case PushConstantType::UVEC4:
                glProgramUniform4uiv(_shaderProgramID, binding, count, castData<GLuint, vec4<U32>>(values)());
                break;
            case PushConstantType::DVEC2:            // Warning! Downcasting to float -Ionut
                glProgramUniform2fv(_shaderProgramID, binding, count, castData<GLfloat, vec2<D64>>(values)());
                break;
            case PushConstantType::VEC2:
                glProgramUniform2fv(_shaderProgramID, binding, count, castData<GLfloat, vec2<F32>>(values)());
                break;
            case PushConstantType::DVEC3:            // Warning! Downcasting to float -Ionut
                glProgramUniform3fv(_shaderProgramID, binding, count, castData<GLfloat, vec3<D64>>(values)());
                break;
            case PushConstantType::VEC3:
                glProgramUniform3fv(_shaderProgramID, binding, count, castData<GLfloat, vec3<F32>>(values)());
                break;
            case PushConstantType::DVEC4:            // Warning! Downcasting to float -Ionut
                glProgramUniform4fv(_shaderProgramID, binding, count, castData<GLfloat, vec4<D64>>(values)());
                break;
            case PushConstantType::VEC4:
                glProgramUniform4fv(_shaderProgramID, binding, count, castData<GLfloat, vec4<F32>>(values)());
                break;
            case PushConstantType::IMAT2:
                glProgramUniformMatrix2fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat2<I32>>(values)());
                break;
            case PushConstantType::IMAT3:
                glProgramUniformMatrix3fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat3<I32>>(values)());
                break;
            case PushConstantType::IMAT4:
                glProgramUniformMatrix4fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat4<I32>>(values)());
                break;
            case PushConstantType::UMAT2:
                glProgramUniformMatrix2fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat2<U32>>(values)());
                break;
            case PushConstantType::UMAT3:
                glProgramUniformMatrix3fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat3<U32>>(values)());
                break;
            case PushConstantType::UMAT4:
                glProgramUniformMatrix4fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat4<U32>>(values)());
                break;
            case PushConstantType::MAT2:
                glProgramUniformMatrix2fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat2<F32>>(values)());
                break;
            case PushConstantType::MAT3:
                glProgramUniformMatrix3fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat3<F32>>(values)());
                break;
            case PushConstantType::MAT4: {
                glProgramUniformMatrix4fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat4<F32>>(values)());
            }break;
            case PushConstantType::DMAT2:
                glProgramUniformMatrix2fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat2<D64>>(values)());
                break;
            case PushConstantType::DMAT3:
                glProgramUniformMatrix3fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat3<D64>>(values)());
                break;
            case PushConstantType::DMAT4:
                glProgramUniformMatrix4fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, mat4<D64>>(values)());
                break;
            default:
                DIVIDE_ASSERT(false, "glShaderProgram::Uniform error: Unhandled data type!");
        }
    }

    inline void glShaderProgram::UploadPushConstant(const PushConstant& constant) {
        I32 binding = cachedValueUpdate(constant);

        if (binding != -1) {
            Uniform(binding, constant._type, constant._values, constant._flag);
        }
    }

    inline void glShaderProgram::UploadPushConstants(const PushConstants& constants) {
        for (auto constant : constants.data()) {
            if (!constant.second._binding.empty() && constant.second._type != PushConstantType::COUNT) {
                UploadPushConstant(constant.second);
            }
        }
    }
}; //namespace Divide

#endif //_PLATFORM_VIDEO_OPENGLS_PROGRAM_INL_
