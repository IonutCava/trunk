#include "SceneGraph.h"
#include "Hardware/Video/GFXDevice.h"
#include "Managers/SceneManager.h"

void SceneGraph::render(){
	GFXDevice& gfx = GFXDevice::getInstance();
	//If we have computed shadowmaps, bind them before rendering any geomtry;
	//Always bind shadowmaps to slots 7 and 8;
	FrameBufferObject *dm0 = SceneManager::getInstance().getActiveScene()->getDepthMap(0);
	FrameBufferObject *dm1 = SceneManager::getInstance().getActiveScene()->getDepthMap(1);

	if(!gfx.getDepthMapRendering() && !gfx.getDeferredShading()){
		if(dm0)	dm0->Bind(7);
		if(dm1)	dm1->Bind(8);
	}		
	_root->updateTransformsAndBounds();
	_root->updateVisualInformation();

	_root->render(); 

	if(!gfx.getDepthMapRendering()&& !gfx.getDeferredShading()){
	 	if(dm0)	dm0->Unbind(8);
		if(dm1)	dm1->Unbind(7);
	}
}

void SceneGraph::print(){
	Console::getInstance().printfn("SceneGraph: ");
	Console::getInstance().toggleTimeStamps(false);
	_root->print();
	Console::getInstance().toggleTimeStamps(true);
}

