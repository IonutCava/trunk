#include "Headers/d3dShaderProgram.h"

namespace Divide {

IMPLEMENT_CUSTOM_ALLOCATOR(d3dShaderProgram, 0, 0);
d3dShaderProgram::d3dShaderProgram(GFXDevice& context, const stringImpl& name, const stringImpl& resourceLocation, bool asyncLoad) :
    ShaderProgram(context, name, resourceLocation, asyncLoad)
{
}

d3dShaderProgram::~d3dShaderProgram()
{
}

bool d3dShaderProgram::unload() {
    return ShaderProgram::unload();
}

bool d3dShaderProgram::recompileInternal() {
    return true;
}

bool d3dShaderProgram::bind() {
    return false;
}

bool d3dShaderProgram::isBound() const {
    return false;
}

bool d3dShaderProgram::isValid() const {
    return false;
}

// Subroutines
void d3dShaderProgram::SetSubroutines(ShaderType type, const vectorImpl<U32>& indices) const {
}

void d3dShaderProgram::SetSubroutine(ShaderType type, U32 index) const {
}

U32 d3dShaderProgram::GetSubroutineIndex(ShaderType type, const char* name) const {
    return 0;
}

U32 d3dShaderProgram::GetSubroutineUniformLocation(ShaderType type, const char* name) const {
    return 0;
}

U32 d3dShaderProgram::GetSubroutineUniformCount(ShaderType type) const {
    return 0;
}

// Uniforms
void d3dShaderProgram::Uniform(const char* ext, U32 value) {}
void d3dShaderProgram::Uniform(const char* ext, I32 value) {}
void d3dShaderProgram::Uniform(const char* ext, F32 value) {}
void d3dShaderProgram::Uniform(const char* ext, const vec2<F32>& value) {}
void d3dShaderProgram::Uniform(const char* ext, const vec2<I32>& value) {}
void d3dShaderProgram::Uniform(const char* ext, const vec3<F32>& value) {}
void d3dShaderProgram::Uniform(const char* ext, const vec3<I32>& value) {}
void d3dShaderProgram::Uniform(const char* ext, const vec4<F32>& value) {}
void d3dShaderProgram::Uniform(const char* ext, const vec4<I32>& value) {}
void d3dShaderProgram::Uniform(const char* ext, const mat3<F32>& value, bool transpose) {}
void d3dShaderProgram::Uniform(const char* ext, const mat4<F32>& value, bool transpose) {}
void d3dShaderProgram::Uniform(const char* ext, const vectorImpl<I32>& values) {}
void d3dShaderProgram::Uniform(const char* ext, const vectorImpl<F32>& values) {}
void d3dShaderProgram::Uniform(const char* ext, const vectorImpl<vec2<F32> >& values) {}
void d3dShaderProgram::Uniform(const char* ext, const vectorImpl<vec3<F32> >& values) {}
void d3dShaderProgram::Uniform(const char* ext, const vectorImplAligned<vec4<F32> >& values) {}
void d3dShaderProgram::Uniform(const char* ext, const vectorImpl<mat3<F32> >& values, bool transpose) {}
void d3dShaderProgram::Uniform(const char* ext, const vectorImplAligned<mat4<F32> >& values, bool transpose) {}

void d3dShaderProgram::Uniform(I32 location, U32 value) {}
void d3dShaderProgram::Uniform(I32 location, I32 value) {}
void d3dShaderProgram::Uniform(I32 location, F32 value) {}
void d3dShaderProgram::Uniform(I32 location, const vec2<F32>& value) {}
void d3dShaderProgram::Uniform(I32 location, const vec2<I32>& value) {}
void d3dShaderProgram::Uniform(I32 location, const vec3<F32>& value){}
void d3dShaderProgram::Uniform(I32 location, const vec3<I32>& value) {}
void d3dShaderProgram::Uniform(I32 location, const vec4<F32>& value) {}
void d3dShaderProgram::Uniform(I32 location, const vec4<I32>& value) {}
void d3dShaderProgram::Uniform(I32 location, const mat3<F32>& value, bool transpose) {}
void d3dShaderProgram::Uniform(I32 location, const mat4<F32>& value, bool transpose) {}
void d3dShaderProgram::Uniform(I32 location, const vectorImpl<I32>& values) {}
void d3dShaderProgram::Uniform(I32 location, const vectorImpl<F32>& values) {}
void d3dShaderProgram::Uniform(I32 location, const vectorImpl<vec2<F32> >& values){}
void d3dShaderProgram::Uniform(I32 location, const vectorImpl<vec3<F32> >& values) {}
void d3dShaderProgram::Uniform(I32 location, const vectorImplAligned<vec4<F32> >& values) {}
void d3dShaderProgram::Uniform(I32 location, const vectorImpl<mat3<F32> >& values, bool transpose) {}
void d3dShaderProgram::Uniform(I32 location, const vectorImplAligned<mat4<F32> >& values, bool transpose) {}
I32 d3dShaderProgram::getUniformLocation(const char* name) { return -1; }
void d3dShaderProgram::DispatchCompute(U32 xGroups, U32 yGroups, U32 zGroups) {}

void d3dShaderProgram::SetMemoryBarrier(MemoryBarrierType type) {
}

bool d3dShaderProgram::load() {
    return ShaderProgram::load();
}
};
