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
	//CEGUI::Direct3D10Renderer& renderer = CEGUI::Direct3D10Renderer::create( /*myD3D10Device*/NULL );
	//GUI::getInstance().bindRenderer(renderer);
	return DX_INIT_ERROR;
}

void DX_API::exitRenderLoop(const bool killCommand)
{
}

void DX_API::closeRenderingApi()
{
}

void DX_API::changeResolution(U16 w, U16 h)
{
}

void DX_API::setWindowPos(U16 w, U16 h)
{
}

void DX_API::lookAt(const vec3<F32>& eye,const vec3<F32>& center,const vec3<F32>& up,const bool invertx,const bool inverty)
{
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

void DX_API::getMatrix(const EXTENDED_MATRIX& mode, mat4<F32>& mat)
{
}

void DX_API::getMatrix(const EXTENDED_MATRIX& mode, mat3<F32>& mat)
{
}

void DX_API::toggle2D(bool _2D)
{
}

void DX_API::lockMatrices(const MATRIX_MODE& setCurrentMatrix, bool lockView, bool lockProjection)
{
}

void DX_API::releaseMatrices(const MATRIX_MODE& setCurrentMatrix, bool releaseView, bool releaseProjection)
{
}

void DX_API::setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes)
{
}

void DX_API::setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes)
{
}

void DX_API::drawText(const std::string& text, const I32 width, const std::string& fontName, const F32 fontSize)
{
}

void DX_API::drawText(const std::string& text, const I32 width, const vec2<I32> position, const std::string& fontName, const F32 fontSize)
{
}

void DX_API::drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset)
{
}

void DX_API::drawLines(const vectorImpl<vec3<F32> >& pointsA,const vectorImpl<vec3<F32> >& pointsB,const vectorImpl<vec4<U8> >& colors, const mat4<F32>& globalOffset,const bool orthoMode,const bool disableDepth)
{
}

void DX_API::debugDraw()
{
}

void DX_API::renderInViewport(const vec4<I32>& rect, boost::function0<void> callback)
{
}

void DX_API::renderInstance(RenderInstance* const instance)
{
}

void DX_API::renderBuffer(VertexBufferObject* const vbo, Transform* const vboTransform)
{
	vbo->DrawRange();
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

RenderStateBlock* DX_API::newRenderStateBlock(const RenderStateBlockDescriptor& descriptor)
{
	return NULL;
}

void DX_API::updateStateInternal(RenderStateBlock* block, bool force)
{
}

bool DX_API::loadInContext(const CurrentContext& context, boost::function0<void> callback)
{
	switch(context){
		case GFX_RENDERING_CONTEXT:
			break;
		case GFX_LOADING_CONTEXT:
			break;
	};
	return false;
}