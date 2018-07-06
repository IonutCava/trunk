#include "GFXDevice.h"
#include "Rendering/Application.h"
#include "Geometry/Object3D.h"
#include "Importer/DVDConverter.h"
#include "Managers/TextureManager.h"
#include "Managers/SceneManager.h"
#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Quad3D.h"
#include "Geometry/Predefined/Text3D.h"
#include "Rendering/PostFX/PostFX.h"
#include "EngineGraphs/RenderQueue.h"

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
	Application::getInstance().setWindowWidth(w);
    Application::getInstance().setWindowHeight(h);
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

void GFXDevice::setRenderState(RenderState& state,bool force) {
	//Memorize the previous rendering state as the current state before changing
	_previousRenderState = _currentRenderState;
	//Apply the new updates to the state
	_api.setRenderState(state,force);
	//Update the current state
	_currentRenderState = state;
}

void GFXDevice::toggleWireframe(bool state){
	_wireframeMode = !_wireframeMode;
	_api.toggleWireframe(_wireframeMode);
}

void GFXDevice::processRenderQueue(){
	//Sort the render queue by the specified key
	RenderQueue::getInstance().sort();
	//Draw the entire queue;
	SceneNode* sn = NULL;
	SceneGraphNode* sgn = NULL;

	for(U16 i = 0; i < RenderQueue::getInstance().getRenderQueueStackSize(); i++){
		sgn = RenderQueue::getInstance().getItem(i)._node;
		sn = sgn->getNode();
		sn->onDraw();
		//draw bounding box if needed
		if(!getDepthMapRendering()){
			BoundingBox& bb = sgn->getBoundingBox();
			if(bb.getVisibility() || SceneManager::getInstance().getActiveScene()->drawBBox()){
				//sn->drawBBox();
				drawBox3D(bb.getMin(),bb.getMax());
			}
		}

		//setup materials and render the node
		//Avoid changing states by not unbind old material/shader/states and checking agains default
		//As nodes are sorted, this should be very fast
		sn->prepareMaterial();
		sn->render(sgn); 
		sn->releaseMaterial();
	}
}
