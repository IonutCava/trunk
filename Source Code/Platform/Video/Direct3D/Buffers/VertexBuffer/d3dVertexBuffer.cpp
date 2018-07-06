#include "Headers/d3dVertexBuffer.h"

namespace Divide {
IMPLEMENT_ALLOCATOR(d3dVertexBuffer, 0, 0)
d3dVertexBuffer::d3dVertexBuffer(GFXDevice& context)
    : VertexBuffer(context)
{
}

d3dVertexBuffer::~d3dVertexBuffer()
{
    destroy();
}

bool d3dVertexBuffer::create(bool staticDraw) {
    return VertexBuffer::create(staticDraw);
}

void d3dVertexBuffer::destroy() {
}

void d3dVertexBuffer::draw(const GenericDrawCommand& command, bool useCmdBuffer) {
}

bool d3dVertexBuffer::queueRefresh() {
    return refresh();
}

bool d3dVertexBuffer::createInternal() {
    return VertexBuffer::createInternal();
}

bool d3dVertexBuffer::refresh() {
    return true;
}

void d3dVertexBuffer::checkStatus() {
}
};
