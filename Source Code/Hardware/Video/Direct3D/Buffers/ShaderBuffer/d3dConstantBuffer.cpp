#include "Headers/d3dConstantBuffer.h"


d3dConstantBuffer::d3dConstantBuffer(const bool unbound) : ShaderBuffer(unbound)
{
}

d3dConstantBuffer::~d3dConstantBuffer()
{
}

void d3dConstantBuffer::Create(bool dynamic, bool stream, U32 primitiveCount, ptrdiff_t primitiveSize) {
    
    ShaderBuffer::Create(dynamic, stream, primitiveCount, primitiveSize);
}

void d3dConstantBuffer::UpdateData(ptrdiff_t offset, ptrdiff_t size, const void *data, const bool invalidateBuffer) const {
}

void d3dConstantBuffer::SetData(const void *data){
}

bool d3dConstantBuffer::BindRange(U32 bindIndex, ptrdiff_t offset, ptrdiff_t size) const {
    return true;
}

bool d3dConstantBuffer::Bind(U32 bindIndex) const {
    return true;
}

void d3dConstantBuffer::PrintInfo(const ShaderProgram* shaderProgram, U32 bindIndex)
{
}