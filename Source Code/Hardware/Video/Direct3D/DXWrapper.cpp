#include "DXWrapper.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"
using namespace std;

void DX_API::initHardware()
{
	Console::getInstance().printfn("Initializing Direct3D rendering API! ");
}

void DX_API::closeRenderingApi()
{
}

void DX_API::lookAt(const vec3& eye,const vec3& center,const vec3& up, bool invertx, bool inverty)
{
}

void DX_API::idle()
{
}

void DX_API::getModelViewMatrix(mat4& mvMat)
{
}

void DX_API::getProjectionMatrix(mat4& projMat)
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

void DX_API::setOrthoProjection(const vec4& rect, const vec2& planes)
{
}

void DX_API::drawTextToScreen(Text* text)
{
}

void DX_API::drawCharacterToScreen(void* ,char)
{
}

void DX_API::drawButton(Button* button)
{
}

void DX_API::drawFlash(GuiFlash* flash)
{
}
void DX_API::drawBox3D(vec3 min, vec3 max)
{
}

void DX_API::renderModel(Object3D* const model)
{
	Type type = TRIANGLES;
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
			Console::getInstance().errorfn("GLWrapper: Invalid Object3D type received for object: [ %s ]",model->getName().c_str());
			b_continue = false;
			break;
	}

	if(b_continue){	
		model->getGeometryVBO()->Enable();
			renderElements(type,_U16,model->getIndices().size(), &(model->getIndices()[0]));
		model->getGeometryVBO()->Disable();
	}
}

void DX_API::renderElements(Type t, Format f, U32 count, const void* first_element)
{
}

void DX_API::setMaterial(Material* mat)
{
}


void DX_API::initDevice()
{
}

void DX_API::toggleWireframe(bool state)
{
}

void DX_API::Screenshot(char *filename, const vec4& rect)
{
}

void DX_API::setObjectState(Transform* const transform)
{
}

void DX_API::releaseObjectState(Transform* const transform)
{
}