#include "Headers/glImmediateModeEmulation.h"
#include "Headers/glResources.h"

#include "core.h"
#include <glim/include/glim.h>

#ifdef GLEW_MX
#	ifdef _DEBUG
#		pragma comment(lib, "GLIMOGL_MX_d.lib")
#	else //_DEBUG
#		pragma comment(lib, "GLIMOGL_MX.lib")
#	endif //_DEBUG
#else //GLEW_MX
#	ifdef _DEBUG
#		pragma comment(lib, "GLIMOGL_d.lib")
#	else//_DEBUG
#		pragma comment(lib, "GLIMOGL.lib")
#	endif //_DEBUG
#endif //GLEW_MX

NS_GLIM::GLIM_ENUM glimPrimitiveType[PrimitiveType_PLACEHOLDER];

glIMPrimitive::glIMPrimitive() : IMPrimitive()
{
    _imInterface = New NS_GLIM::GLIM_BATCH();
    _imInterface->SetVertexAttribLocation(Divide::GL::VERTEX_POSITION_LOCATION);
    assert(_imInterface != NULL);
    glimPrimitiveType[API_POINTS] = NS_GLIM::GLIM_POINTS;
    glimPrimitiveType[LINES] = NS_GLIM::GLIM_LINES;
    glimPrimitiveType[LINE_LOOP] = NS_GLIM::GLIM_LINE_LOOP;
    glimPrimitiveType[LINE_STRIP] = NS_GLIM::GLIM_LINE_STRIP;
    glimPrimitiveType[TRIANGLES] = NS_GLIM::GLIM_TRIANGLES;
    glimPrimitiveType[TRIANGLE_STRIP] = NS_GLIM::GLIM_TRIANGLE_STRIP;
    glimPrimitiveType[TRIANGLE_FAN] = NS_GLIM::GLIM_TRIANGLE_FAN;
    glimPrimitiveType[QUADS] = NS_GLIM::GLIM_QUADS;
    glimPrimitiveType[QUAD_STRIP] = NS_GLIM::GLIM_QUAD_STRIP;
    glimPrimitiveType[POLYGON] = NS_GLIM::GLIM_POLYGON;
}

glIMPrimitive::~glIMPrimitive()
{
    SAFE_DELETE(_imInterface);
}

void glIMPrimitive::beginBatch() {
    _imInterface->BeginBatch();
    _inUse = true;
    _zombieCounter = 0;
}

void glIMPrimitive::begin(PrimitiveType type){
    _imInterface->Begin(glimPrimitiveType[type]);
}

void glIMPrimitive::vertex(const vec3<F32>& vert){
    _imInterface->Vertex( vert.x, vert.y, vert.z );
}

void glIMPrimitive::attribute4ub(const std::string& attribName, const vec4<U8>& value){
    _imInterface->Attribute4ub(attribName.c_str(), value.x, value.y, value.z, value.w);
}

void glIMPrimitive::attribute4f(const std::string& attribName, const vec4<F32>& value){
    _imInterface->Attribute4f(attribName.c_str(), value.x, value.y, value.z, value.w);
}

void glIMPrimitive::end(){
    _imInterface->End();
}

void glIMPrimitive::endBatch() {
    _imInterface->EndBatch();
}

void glIMPrimitive::clear() {
    _imInterface->Clear();
}

void glIMPrimitive::renderBatch(bool wireframe) {
    _imInterface->RenderBatch(wireframe);
}

void glIMPrimitive::renderBatchInstanced(I32 count, bool wireframe) {
    _imInterface->RenderBatchInstanced(count, wireframe);
}