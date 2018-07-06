#include "Headers/glIMPrimitive.h"
#include "Headers/glResources.h"
#include "Headers/GLWrapper.h"
#include "Hardware/Video/Headers/GFXDevice.h"

#include "core.h"
#include <glim.h>

glIMPrimitive::glIMPrimitive() : IMPrimitive()
{
    _imInterface = New NS_GLIM::GLIM_BATCH();
    _imInterface->SetVertexAttribLocation(Divide::VERTEX_POSITION_LOCATION);
}

glIMPrimitive::~glIMPrimitive()
{
    SAFE_DELETE(_imInterface);
}

void glIMPrimitive::beginBatch() {
    _imInterface->BeginBatch();
    IMPrimitive::beginBatch();
}

void glIMPrimitive::begin(PrimitiveType type) {
    _imInterface->Begin(Divide::GLUtil::GL_ENUM_TABLE::glimPrimitiveType[type]);
}

void glIMPrimitive::vertex(const vec3<F32>& vert) {
    _imInterface->Vertex( vert.x, vert.y, vert.z );
}

void glIMPrimitive::attribute4ub(const std::string& attribName, const vec4<U8>& value) {
    _imInterface->Attribute4ub(attribName.c_str(), value.x, value.y, value.z, value.w);
}

void glIMPrimitive::attribute4f(const std::string& attribName, const vec4<F32>& value) {
    _imInterface->Attribute4f(attribName.c_str(), value.x, value.y, value.z, value.w);
}

void glIMPrimitive::attribute1i(const std::string& attribName, I32 value) {
    _imInterface->Attribute1i(attribName.c_str(), value);
}

void glIMPrimitive::end() {
    _imInterface->End();
}

void glIMPrimitive::endBatch() {
    _imInterface->EndBatch();
}

void glIMPrimitive::clear() {
    IMPrimitive::clear();
    _imInterface->Clear();
}

void glIMPrimitive::render(bool forceWireframe, U32 instanceCount) {
    DIVIDE_ASSERT(_drawShader != nullptr, "glIMPrimitive error: Draw call received without a valid shader defined!");
   _imInterface->SetShaderProgramHandle(_drawShader->getId());
    if (instanceCount == 1) {
        _imInterface->RenderBatch(forceWireframe || _forceWireframe);
    } else {
        _imInterface->RenderBatchInstanced(instanceCount, forceWireframe || _forceWireframe);
    }
    GFX_DEVICE.registerDrawCall();
}