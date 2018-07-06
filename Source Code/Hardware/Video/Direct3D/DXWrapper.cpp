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

void DX_API::exitRenderLoop(bool killCommand) 
{
}

void DX_API::closeRenderingApi()
{
}

void DX_API::changeResolution(U16 w, U16 h)
{
}

void DX_API::setWindowSize(U16 w, U16 h)
{
}

void DX_API::setWindowPos(U16 w, U16 h)
{
}

void DX_API::lookAt(const vec3<F32>& eye,const vec3<F32>& center,const vec3<F32>& up, bool invertx, bool inverty)
{
}

void DX_API::idle()
{
}

void DX_API::flush()
{
}

void DX_API::getMatrix(MATRIX_MODE mode, mat4<F32>& mat) 
{
}

void DX_API::clearBuffers(U16 buffer_mask)
{
}

void DX_API::swapBuffers()
{
}

void DX_API::enableFog(F32 density, F32* color)
{
}

void DX_API::toggle2D(bool _2D)
{
}

void DX_API::lockProjection()
{
}

void DX_API::releaseProjection()
{
}

void DX_API::lockModelView()
{
}

void DX_API::releaseModelView()
{
}

void DX_API::setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes)
{
}
void DX_API::setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes)
{
}
void DX_API::drawTextToScreen(GUIElement* const text)
{
}

void DX_API::drawButton(GUIElement* const button)
{
}

void DX_API::drawFlash(GUIElement* const flash)
{
}

void DX_API::drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset)
{
}

void DX_API::drawLines(const vectorImpl<vec3<F32> >& pointsA,const vectorImpl<vec3<F32> >& pointsB,const vectorImpl<vec4<F32> >& colors, const mat4<F32>& globalOffset)
{
}

void DX_API::renderInViewport(const vec4<F32>& rect, boost::function0<void> callback){
}

void DX_API::renderModel(Object3D* const model)
{
	PrimitiveType type = TRIANGLES;
	//render in the switch or after. hacky, but works -Ionut
	bool b_continue = true;
	switch(model->getType()){
		case Object3D::TEXT_3D:{
			Text3D* text = dynamic_cast<Text3D*>(model);
			b_continue = false;
			}break;
		
		case Object3D::BOX_3D:
		case Object3D::SUBMESH:
			type = TRIANGLES;
			break;
		case Object3D::QUAD_3D:
		case Object3D::SPHERE_3D:
			type = QUADS;
			break;
		//We should never enter the default case!
		default:
			ERROR_FN("ERROR_D3D_INVALID_OBJECT_TYPE",model->getName().c_str());
			b_continue = false;
			break;
	}

	if(b_continue){	
		model->getGeometryVBO()->Enable();
			renderElements(type,UNSIGNED_SHORT,model->getGeometryVBO()->getHWIndices().size(), &(model->getGeometryVBO()->getHWIndices()[0]));
		model->getGeometryVBO()->Disable();
	}
}

void DX_API::renderElements(PrimitiveType t, GFXDataFormat f, U32 count, const void* first_element)
{
}

void DX_API::setMaterial(Material* mat)
{
}


void DX_API::initDevice(U32 targetFrameRate)
{
}

void DX_API::Screenshot(char *filename, const vec4<F32>& rect)
{
}

void DX_API::setObjectState(Transform* const transform, bool force, ShaderProgram* const shader)
{
}

void DX_API::releaseObjectState(Transform* const transform, ShaderProgram* const shader)
{
}

F32 DX_API::applyCropMatrix(frustum &f,SceneGraph* sceneGraph)
{
	return 0;
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