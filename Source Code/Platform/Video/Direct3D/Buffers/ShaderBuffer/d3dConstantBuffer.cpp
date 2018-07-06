#include "Headers/d3dConstantBuffer.h"

namespace Divide {

d3dConstantBuffer::d3dConstantBuffer(const stringImpl& bufferName, bool unbound,
    bool persistentMapped)
    : ShaderBuffer(bufferName, unbound, persistentMapped)

{
}

d3dConstantBuffer::~d3dConstantBuffer()
{
}

void d3dConstantBuffer::Create(U32 primitiveCount, ptrdiff_t primitiveSize) {
    ShaderBuffer::Create(primitiveCount, primitiveSize);
}

void d3dConstantBuffer::DiscardAllData() const {}

void d3dConstantBuffer::DiscardSubData(ptrdiff_t offset, ptrdiff_t size) const {}

void d3dConstantBuffer::UpdateData(ptrdiff_t offset, ptrdiff_t size,
    const bufferPtr data) const {}

bool d3dConstantBuffer::BindRange(U32 bindIndex,
    U32 offsetElementCount,
    U32 rangeElementCount) {
    return true;
}

bool d3dConstantBuffer::Bind(U32 bindIndex) {
    return true;
}

void d3dConstantBuffer::PrintInfo(const ShaderProgram* shaderProgram,
    U32 bindIndex) {}

};//namespace Divide