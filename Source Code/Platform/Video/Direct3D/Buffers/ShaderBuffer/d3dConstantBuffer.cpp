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

void d3dConstantBuffer::Destroy() {
}

void d3dConstantBuffer::Create(U32 primitiveCount, U32 sizeFactor, ptrdiff_t primitiveSize) {
    ShaderBuffer::Create(primitiveCount, sizeFactor, primitiveSize);
}

void d3dConstantBuffer::DiscardAllData() const {
}

void d3dConstantBuffer::DiscardSubData(ptrdiff_t offsetElementCount,
                                       ptrdiff_t rangeElementCount) const {
}

void d3dConstantBuffer::UpdateData(ptrdiff_t offsetElementCount,
                                   ptrdiff_t rangeElementCount,
                                   const bufferPtr data) const {
}

bool d3dConstantBuffer::BindRange(U32 bindIndex, U32 offsetElementCount,
                                  U32 rangeElementCount) {
    return CheckBindRange(bindIndex, offsetElementCount, rangeElementCount);
    ;
}

bool d3dConstantBuffer::CheckBindRange(U32 bindIndex, U32 offsetElementCount,
                                       U32 rangeElementCount) {
    return false;
}

bool d3dConstantBuffer::Bind(U32 bindIndex) {
    return CheckBind(bindIndex);
}

bool d3dConstantBuffer::CheckBind(U32 bindIndex) {
    return false;
}

void d3dConstantBuffer::PrintInfo(const ShaderProgram* shaderProgram,
    U32 bindIndex) {}

};//namespace Divide