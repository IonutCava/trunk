#include "GFXDevice.h"
#include "Rendering/common.h"
#include "Geometry/Object3D.h"
#include "Importer/DVDConverter.h"
#include "Managers/TextureManager.h"
#include "Managers/SceneManager.h"
#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Quad3D.h"
#include "Geometry/Predefined/Text3D.h"
#include "Rendering/PostFX/PostFX.h"

using namespace std;

void GFXDevice::setApi(RenderAPI api)
{
	switch(api)
	{
	case 0: //OpenGL 1.0
	case 1: //OpenGL 1.2
	case 2: //OpenGL 2.0
	case 3: //OpenGL 2.1
	case 4: //OpenGL 2.2
	case 5: //OpenGL 3.0
	case 6: //OpenGL 3.2
	default:
		_api = GL_API::getInstance();
		break;

	case 7: //DX 8
	case 8: //DX 9
	case 9: //DX 10
		_api = DX_API::getInstance();
		break;
	//Ionut: OpenGL 4.0 and DX 11 in another life maybe :)
	};
	_api.setId(api);
}

void GFXDevice::resizeWindow(U16 w, U16 h){
	Engine::getInstance().setWindowWidth(w);
    Engine::getInstance().setWindowHeight(h);
	_api.resizeWindow(w,h);
	PostFX::getInstance().reshapeFBO(w, h);
}

void GFXDevice::renderModel(SceneGraphNode* node){
	Object3D* model = node->getNode<Object3D>();
	if(!model) return;
	if(model->shouldDelete()){
		delete node;
		node = NULL;
		return;
	}
	switch(model->getType())
	{
		case BOX_3D:
			drawBox3D(node);
			break;
		case SPHERE_3D:
			drawSphere3D(node);
			break;
		case QUAD_3D:
			drawQuad3D(node);
			break;
		case TEXT_3D:
			drawText3D(node);
			break;
		case SUBMESH:
			_api.renderModel(node);
			break;
		default:
			break;
	};
}

void GFXDevice::toggleWireframe(bool state)
{
	_wireframeMode = !_wireframeMode;
	_api.toggleWireframe(_wireframeMode);
}

