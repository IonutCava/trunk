#include "DXWrapper.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"


void DX_API::initHardware()
{
	PRINT_FN("Initializing Direct3D rendering API! ");
	D3D_ENUM_TABLE::fill();
}

void DX_API::closeRenderingApi()
{
}

void DX_API::lookAt(const vec3<F32>& eye,const vec3<F32>& center,const vec3<F32>& up, bool invertx, bool inverty)
{
}

void DX_API::idle()
{
}

void DX_API::getModelViewMatrix(mat4<F32>& mvMat)
{
}

void DX_API::getProjectionMatrix(mat4<F32>& projMat)
{

}

void DX_API::clearBuffers(U8 buffer_mask)
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

void DX_API::drawCharacterToScreen(void* ,char)
{
}

void DX_API::drawButton(GUIElement* const button)
{
}

void DX_API::drawFlash(GUIElement* const flash)
{
}

void DX_API::drawConsole()
{
}

void DX_API::drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset)
{
}

void DX_API::drawLines(const std::vector<vec3<F32> >& pointsA,const std::vector<vec3<F32> >& pointsB,const std::vector<vec4<F32> >& colors, const mat4<F32>& globalOffset)
{
}

void DX_API::renderInViewport(const vec4<F32>& rect, boost::function0<void> callback){
}

void DX_API::renderModel(Object3D* const model)
{
	PRIMITIVE_TYPE type = TRIANGLES;
	//render in the switch or after. hacky, but works -Ionut
	bool b_continue = true;
	switch(model->getType()){
		case TEXT_3D:{
			Text3D* text = dynamic_cast<Text3D*>(model);
			b_continue = false;
			}break;
		
		case BOX_3D:
		case SUBMESH:
			type = TRIANGLES;
			break;
		case QUAD_3D:
		case SPHERE_3D:
			type = QUADS;
			break;
		//We should never enter the default case!
		default:
			ERROR_FN("GLWrapper: Invalid Object3D type received for object: [ %s ]",model->getName().c_str());
			b_continue = false;
			break;
	}

	if(b_continue){	
		model->getGeometryVBO()->Enable();
			renderElements(type,_U16,model->getGeometryVBO()->getHWIndices().size(), &(model->getGeometryVBO()->getHWIndices()[0]));
		model->getGeometryVBO()->Disable();
	}
}

void DX_API::renderElements(PRIMITIVE_TYPE t, VERTEX_DATA_FORMAT f, U32 count, const void* first_element)
{
}

void DX_API::setMaterial(Material* mat)
{
}


void DX_API::initDevice(U32 targetFPS)
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