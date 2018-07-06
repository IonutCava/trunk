#include "Headers/glIMPrimitive.h"
#include "Headers/glResources.h"
#include "Headers/GLWrapper.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include <GLIM/glim.h>

namespace Divide {

glIMPrimitive::glIMPrimitive() : IMPrimitive() {
    _imInterface.reset(new NS_GLIM::GLIM_BATCH());
    _imInterface->SetVertexAttribLocation(
        to_uint(AttribLocation::VERTEX_POSITION));
}

glIMPrimitive::~glIMPrimitive()
{
}

void glIMPrimitive::beginBatch(bool reserveBuffers, U32 vertexCount) {
    _imInterface->BeginBatch(reserveBuffers, vertexCount);
}

void glIMPrimitive::begin(PrimitiveType type) {
    _imInterface->Begin(GLUtil::glimPrimitiveType[to_uint(type)]);
}

void glIMPrimitive::vertex(F32 x, F32 y, F32 z) {
    _imInterface->Vertex(x, y, z);
}

void glIMPrimitive::attribute4ub(U32 attribLocation, U8 x, U8 y, U8 z, U8 w) {
    _imInterface->Attribute4ub(attribLocation, x, y, z, w);
}

void glIMPrimitive::attribute4f(U32 attribLocation, F32 x, F32 y, F32 z, F32 w) {
    _imInterface->Attribute4f(attribLocation, x, y, z, w);
}

void glIMPrimitive::attribute1i(U32 attribLocation, I32 value) {
    _imInterface->Attribute1i(attribLocation, value);
}

void glIMPrimitive::end() {
    _imInterface->End();
}

void glIMPrimitive::endBatch() {
    _imInterface->EndBatch();
}

void glIMPrimitive::render(bool forceWireframe, U32 instanceCount) {
    DIVIDE_ASSERT(_drawShader != nullptr,
                  "glIMPrimitive error: Draw call received without a valid "
                  "shader defined!");
    _imInterface->SetShaderProgramHandle(_drawShader->getID());
    _imInterface->RenderBatchInstanced(instanceCount,
                                       forceWireframe || _forceWireframe);
    zombieCounter(0);

    GFX_DEVICE.registerDrawCall();
}
};