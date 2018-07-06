#include "Headers/d3dConstantBuffer.h"

namespace Divide {

d3dConstantBuffer::d3dConstantBuffer(const stringImpl& bufferName,
                                     const U32 sizeFactor, 
                                     bool unbound,
                                     bool persistentMapped)
    : ShaderBuffer(bufferName, sizeFactor, unbound, persistentMapped)

{
}

d3dConstantBuffer::~d3dConstantBuffer()
{
}

void d3dConstantBuffer::Destroy() {
}

void d3dConstantBuffer::Create(U32 primitiveCount, ptrdiff_t primitiveSize) {
    ShaderBuffer::Create(primitiveCount, primitiveSize);
}

void d3dConstantBuffer::UpdateData(ptrdiff_t offsetElementCount,
                                   ptrdiff_t rangeElementCount,
                                   const bufferPtr data) {
}

bool d3dConstantBuffer::BindRange(U32 bindIndex, U32 offsetElementCount,
                                  U32 rangeElementCount) {
    return false;
}

bool d3dConstantBuffer::Bind(U32 bindIndex) {
    return false;
}

};//namespace Divide