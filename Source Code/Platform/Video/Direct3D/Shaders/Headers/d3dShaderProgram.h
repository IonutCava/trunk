/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef HLSL_H_
#define HLSL_H_

#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

class d3dShaderProgram final : public ShaderProgram {
    DECLARE_ALLOCATOR
   public:
    explicit d3dShaderProgram(GFXDevice& context,
                              const stringImpl& name,
                              const stringImpl& resourceLocation,
                              bool asyncLoad);

    ~d3dShaderProgram();

    bool unload() override;

    bool bind() override;

    bool isBound() const override;
    bool isValid() const override;
    
    // Subroutines
    void SetSubroutines(ShaderType type, const vectorImpl<U32>& indices) const override;
    void SetSubroutine(ShaderType type, U32 index) const override;

    U32 GetSubroutineIndex(ShaderType type, const char* name) const override;

    U32 GetSubroutineUniformLocation(ShaderType type, const char* name) const override;

    U32 GetSubroutineUniformCount(ShaderType type) const override;
    // Uniforms
    void Uniform(const char* ext, U32 value) override;
    void Uniform(const char* ext, I32 value) override;
    void Uniform(const char* ext, F32 value) override;
    void Uniform(const char* ext, const vec2<F32>& value) override;
    void Uniform(const char* ext, const vec2<I32>& value) override;
    void Uniform(const char* ext, const vec3<F32>& value) override;
    void Uniform(const char* ext, const vec3<I32>& value) override;
    void Uniform(const char* ext, const vec4<F32>& value) override;
    void Uniform(const char* ext, const vec4<I32>& value) override;
    void Uniform(const char* ext,
                 const mat3<F32>& value,
                 bool transpose = false) override;
    void Uniform(const char* ext,
                 const mat4<F32>& value,
                 bool transpose = false) override;
    void Uniform(const char* ext, const vectorImpl<I32>& values) override;
    void Uniform(const char* ext, const vectorImpl<F32>& values) override;
    void Uniform(const char* ext, const vectorImpl<vec2<F32> >& values) override;
    void Uniform(const char* ext, const vectorImpl<vec3<F32> >& values) override;
    void Uniform(const char* ext, const vectorImplAligned<vec4<F32> >& values) override;
    void Uniform(const char* ext,
                 const vectorImpl<mat3<F32> >& values,
                 bool transpose = false) override;
    void Uniform(const char* ext,
                 const vectorImplAligned<mat4<F32> >& values,
                 bool transpose = false) override;

    void Uniform(I32 location, U32 value) override;
    void Uniform(I32 location, I32 value) override;
    void Uniform(I32 location, F32 value) override;
    void Uniform(I32 location, const vec2<F32>& value) override;
    void Uniform(I32 location, const vec2<I32>& value) override;
    void Uniform(I32 location, const vec3<F32>& value) override;
    void Uniform(I32 location, const vec3<I32>& value) override;
    void Uniform(I32 location, const vec4<F32>& value) override;
    void Uniform(I32 location, const vec4<I32>& value) override;
    void Uniform(I32 location, const mat3<F32>& value, bool transpose = false) override;
    void Uniform(I32 location, const mat4<F32>& value, bool transpose = false) override;
    void Uniform(I32 location, const vectorImpl<I32>& values) override;
    void Uniform(I32 location, const vectorImpl<F32>& values) override;
    void Uniform(I32 location, const vectorImpl<vec2<F32> >& values) override;
    void Uniform(I32 location, const vectorImpl<vec3<F32> >& values) override;
    void Uniform(I32 location, const vectorImplAligned<vec4<F32> >& values) override;
    void Uniform(I32 location,
                 const vectorImpl<mat3<F32> >& values,
                 bool transpose = false) override;
    void Uniform(I32 location,
                 const vectorImplAligned<mat4<F32> >& values,
                 bool transpose = false) override;
    I32 getUniformLocation(const char* name) override;
    void DispatchCompute(U32 xGroups, U32 yGroups, U32 zGroups) override;

    void SetMemoryBarrier(MemoryBarrierType type) override;

   protected:
    bool load() override;
};

};  // namespace Divide
#endif
