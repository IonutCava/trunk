#include "Headers/d3dConstantBuffer.h"


d3dConstantBuffer::d3dConstantBuffer(const bool unbound) : ShaderBuffer(unbound)
{
}

d3dConstantBuffer::~d3dConstantBuffer()
{
}

void d3dConstantBuffer::Create(bool dynamic, bool stream){
}

void d3dConstantBuffer::ReserveBuffer(U32 primitiveCount, ptrdiff_t primitiveSize) const {
}

void d3dConstantBuffer::ChangeSubData(ptrdiff_t offset, ptrdiff_t size, const void *data, const bool invalidateBuffer) const {
}

bool d3dConstantBuffer::bindRange(U32 bindIndex, ptrdiff_t offset, ptrdiff_t size) const {
    return true;
}

bool d3dConstantBuffer::bind(U32 bindIndex) const {
    return true;
}

void d3dConstantBuffer::printInfo(const ShaderProgram* shaderProgram, U32 bindIndex)
{
}