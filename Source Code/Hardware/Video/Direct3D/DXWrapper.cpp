#include "Headers/DXWrapper.h"

#include "GUI/Headers/GUI.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

I8 DX_API::initHardware(const vec2<U16>& resolution, I32 argc, char **argv){
    PRINT_FN(Locale::get("START_D3D_API"));
    D3D_ENUM_TABLE::fill();
    //CEGUI::Direct3D10Renderer& renderer = CEGUI::Direct3D10Renderer::create( /*myD3D10Device*/nullptr );
    //GUI::getInstance().bindRenderer(renderer);
    return DX_INIT_ERROR;
}

void DX_API::exitRenderLoop(bool killCommand)
{
}

void DX_API::closeRenderingApi()
{
}

void DX_API::changeResolutionInternal(U16 w, U16 h)
{
}

void DX_API::setWindowPos(U16 w, U16 h) const
{
}

void DX_API::setMousePosition(U16 x, U16 y) const
{
}

F32* DX_API::lookAt(const mat4<F32>& viewMatrix)
{
    return nullptr;
}

void DX_API::beginFrame()
{
}

void DX_API::endFrame()
{
}

void DX_API::idle()
{
}

void DX_API::flush()
{
}

void DX_API::getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat)
{
}

void DX_API::toggleRasterization(bool state)
{
}

F32* DX_API::setProjection(const vec4<F32>& rect, const vec2<F32>& planes)
{
    return nullptr;
}

F32* DX_API::setProjection(F32 FoV, F32 aspectRatio, const vec2<F32>& planes)
{
    return nullptr;
}

void DX_API::setAnaglyphFrustum(F32 camIOD, const vec2<F32>& zPlanes, F32 aspectRatio, F32 verticalFoV, bool rightFrustum)
{
}

void DX_API::updateClipPlanes()
{
}

void DX_API::drawText(const TextLabel& textLabel, const vec2<I32>& position)
{
}

void DX_API::drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset)
{
}

void DX_API::drawLines(const vectorImpl<vec3<F32> >& pointsA,const vectorImpl<vec3<F32> >& pointsB,const vectorImpl<vec4<U8> >& colors, const mat4<F32>& globalOffset,const bool orthoMode,const bool disableDepth)
{
}

void DX_API::drawPoints(U32 numPoints)
{
}

void DX_API::debugDraw(const SceneRenderState& sceneRenderState)
{
}

void DX_API::renderInViewport(const vec4<I32>& rect, const DELEGATE_CBK& callback)
{
}

void DX_API::initDevice(U32 targetFrameRate)
{
}

bool DX_API::initShaders()
{
    return true;
}

bool DX_API::deInitShaders()
{
    return true;
}

void DX_API::Screenshot(char *filename, const vec4<F32>& rect)
{
}

bool DX_API::loadInContext(const CurrentContext& context, const DELEGATE_CBK& callback)
{
    switch(context){
        case GFX_RENDERING_CONTEXT:
            break;
        case GFX_LOADING_CONTEXT:
            break;
    };
    return false;
}

void DX_API::activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock)
{

}