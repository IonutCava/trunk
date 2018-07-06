#include "Headers/d3dConstantBuffer.h"


d3dConstantBuffer::d3dConstantBuffer(bool unbound, bool persistentMapped) : ShaderBuffer(unbound, persistentMapped)
{
}

d3dConstantBuffer::~d3dConstantBuffer()
{
}

void d3dConstantBuffer::Create(U32 primitiveCount, ptrdiff_t primitiveSize) {
    
    ShaderBuffer::Create(primitiveCount, primitiveSize);
}

void d3dConstantBuffer::DiscardAllData() {
}

void d3dConstantBuffer::DiscardSubData(ptrdiff_t offset, ptrdiff_t size) {
}

void d3dConstantBuffer::UpdateData(ptrdiff_t offset, ptrdiff_t size, const void *data, const bool invalidateBuffer) const {
}


bool d3dConstantBuffer::BindRange(Divide::ShaderBufferLocation bindIndex, U32 offsetElementCount, U32 rangeElementCount) const {
    return true;
}

bool d3dConstantBuffer::Bind(Divide::ShaderBufferLocation bindIndex) const {
    return true;
}

void d3dConstantBuffer::PrintInfo(const ShaderProgram* shaderProgram, Divide::ShaderBufferLocation bindIndex)
{
}