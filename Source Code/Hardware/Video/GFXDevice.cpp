#include "GFXDevice.h"
#include "Core/Headers/Application.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Graphs/Headers/RenderQueue.h"

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
	case 10: //Placeholder
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

void GFXDevice::renderModel(Object3D* const model){
	if(!model) return;
	if(model->shouldDelete()){
		//ToDo: Properly unload this!
		//delete node;
		//node = NULL;
		return;
	}
	if(model->getIndices().empty()){
		model->onDraw(); //something wrong! Re-run pre-draw tests!
	}
	_api.renderModel(model);

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
	SceneNode* sn = NULL;
	SceneGraphNode* sgn = NULL;
	Transform* t = NULL;
	LightManager::getInstance().bindDepthMaps();
	//Sort the render queue by the specified key
	RenderQueue::getInstance().sort();
	//Draw the entire queue;
	for(U16 i = 0; i < RenderQueue::getInstance().getRenderQueueStackSize(); i++){
		sgn = RenderQueue::getInstance().getItem(i)._node;
		assert(sgn);
		t = sgn->getTransform();
		sn = sgn->getNode();
		assert(sn);
		sn->onDraw();
		//draw bounding box if needed
		if(!getDepthMapRendering()){
			BoundingBox& bb = sgn->getBoundingBox();
			if(bb.getVisibility() || SceneManager::getInstance().getActiveScene()->drawBBox()){
				drawBox3D(bb.getMin(),bb.getMax());
			}
		}

		//setup materials and render the node
		//Avoid changing states by not unbind old material/shader/states and checking agains default
		//As nodes are sorted, this should be very fast
		setObjectState(t);
		sn->prepareMaterial(sgn);
		sn->render(sgn); 
		sn->releaseMaterial();
		releaseObjectState(t);
	}
	LightManager::getInstance().unbindDepthMaps();
}
