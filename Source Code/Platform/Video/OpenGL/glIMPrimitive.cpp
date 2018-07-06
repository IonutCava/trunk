#include "Headers/glIMPrimitive.h"
#include "Headers/glResources.h"
#include "Headers/GLWrapper.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include <glim.h>

namespace Divide {

glIMPrimitive::glIMPrimitive() : IMPrimitive() {
    _imInterface.reset(new NS_GLIM::GLIM_BATCH());
    _imInterface->SetVertexAttribLocation(
        enum_to_uint(AttribLocation::VERTEX_POSITION_LOCATION));
}

glIMPrimitive::~glIMPrimitive() {}

void glIMPrimitive::beginBatch() {
    _imInterface->BeginBatch();
    IMPrimitive::beginBatch();
}

void glIMPrimitive::begin(PrimitiveType type) {
    _imInterface->Begin(GLUtil::GL_ENUM_TABLE::glimPrimitiveType[enum_to_uint(type)]);
}

void glIMPrimitive::vertex(F32 x, F32 y, F32 z) {
    _imInterface->Vertex(x, y, z);
}

void glIMPrimitive::attribute4ub(const stringImpl& attribName, U8 x, U8 y, U8 z,
                                 U8 w) {
    _imInterface->Attribute4ub(attribName.c_str(), x, y, z, w);
}

void glIMPrimitive::attribute4f(const stringImpl& attribName, F32 x, F32 y,
                                F32 z, F32 w) {
    _imInterface->Attribute4f(attribName.c_str(), x, y, z, w);
}

void glIMPrimitive::attribute1i(const stringImpl& attribName, I32 value) {
    _imInterface->Attribute1i(attribName.c_str(), value);
}

void glIMPrimitive::end() { _imInterface->End(); }

void glIMPrimitive::endBatch() { _imInterface->EndBatch(); }

void glIMPrimitive::clear() {
    IMPrimitive::clear();
    _imInterface->Clear();
}

void glIMPrimitive::render(bool forceWireframe, U32 instanceCount) {
    DIVIDE_ASSERT(_drawShader != nullptr,
                  "glIMPrimitive error: Draw call received without a valid "
                  "shader defined!");
    _imInterface->SetShaderProgramHandle(_drawShader->getID());
    _imInterface->RenderBatchInstanced(instanceCount,
                                       forceWireframe || _forceWireframe);
    GFX_DEVICE.registerDrawCall();
}
};