#include "CubeScene.h"
#include "Managers/CameraManager.h"
#include "Rendering/Application.h"
#include "PhysX/PhysX.h"
#include "GUI/GUI.h"
#include "Geometry/Predefined/Quad3D.h"
using namespace std;

void CubeScene::render(){
	//Pass 2
	//Draw a 2D fullscreen quad that has the lighting shader applied to it and all generated textures bound to that shader
	
	_deferredShader->bind();
		_deferredBuffer->Bind(0,0);
		_deferredBuffer->Bind(1,1);
		_deferredBuffer->Bind(2,2);
		_lightTexture->Bind(3);

			_deferredShader->UniformTexture("tImage0",0);
			_deferredShader->UniformTexture("tImage1",1);
			_deferredShader->UniformTexture("tImage2",2);
			_deferredShader->UniformTexture("lightTexture",3);
			_deferredShader->UniformTexture("lightCount",getLights().size());
			_deferredShader->Uniform("cameraPosition",CameraManager::getInstance().getActiveCamera()->getEye());
			_GFX.toggle2D(true);
			_GFX.drawQuad3D(_renderQuad);
			_GFX.toggle2D(false);

		_lightTexture->Unbind(3);
		_deferredBuffer->Unbind(2);
		_deferredBuffer->Unbind(1);
		_deferredBuffer->Unbind(0);
	_deferredShader->unbind();
	
}

void CubeScene::processEvents(F32 time)
{

	F32 updateLights = 0.05f;

	if(time - _eventTimers[0] >= updateLights){
		for(U8 row=0; row<3; row++)
			for(U8 col=0; col < getLights().size()/3; col++){
				F32 x = col * 150.0f - 5.0f + cos(GETMSTIME()*(col-row+2)*0.008f)*200.0f;
				F32 y = cos(GETMSTIME()*(col-row+2)*0.01f)*200.0f+210;;
				F32 z = row * 500.0f -500.0f - cos(GETMSTIME()*(col-row+2)*0.009f)*200.0f+110;
				F32 g = 1.0f-(row/3.0f);
				F32 b = col/getLights().size();
	
				getLights()[row*10+col]->setLightProperties("position",vec4(x,y,z,1));
				getLights()[row*10+col]->setLightProperties("diffuse",vec4(1,g,b,1));
			}

		_eventTimers[0] += updateLights;
	}

}

I8 j = 1;
F32 i = 0;
bool _switch = false;
void CubeScene::preRender()
{
	if(i >= 360) _switch = true;
	else if(i <= 0) _switch = false;

	if(!_switch) i += 0.1f;
	else i -= 0.1f;

	i >= 180 ? j = -1 : j = 1;

	SceneGraphNode* cutia1 = _sceneGraph->findNode("Cutia1");
	SceneGraphNode* hellotext = _sceneGraph->findNode("HelloText");
	SceneGraphNode* bila = _sceneGraph->findNode("Bila");
	SceneGraphNode* dwarf = _sceneGraph->findNode("dwarf");
	cutia1->getTransform()->rotateEuler(vec3(0.3f*i, 0.6f*i,0));
	hellotext->getTransform()->rotate(vec3(0.6f,0.2f,0.4f),i);
	bila->getTransform()->translateY(j*0.25f);
	dwarf->getTransform()->rotate(vec3(0,1,0),i);

	if(PhysX::getInstance().getScene() != NULL)	
		PhysX::getInstance().UpdateActors();

	U8 index = 0;
	F32* pixels = (F32*)_lightTexture->Begin();
	for(U8 row=0; row<3; row++)
		for(U8 col=0; col < getLights().size()/3; col++){
			U8 i = row*10+col;
			//Light Position																										
			pixels[index + 0]  = getLights()[i]->getPosition().x;		
			pixels[index + 1]  = getLights()[i]->getPosition().y;		
			pixels[index + 2]  = getLights()[i]->getPosition().z;		
			//Light Color
			pixels[index + 3]  = getLights()[i]->getDiffuseColor().r;					
			pixels[index + 4]  = getLights()[i]->getDiffuseColor().g;											
			pixels[index + 5]  = getLights()[i]->getDiffuseColor().b;
			index += 6;
		}
		_lightTexture->End();
	//Pass 1
	//Draw the geometry, saving parameters into the buffer
	_deferredBuffer->Begin();
		_GFX.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
		RenderState s(false,false,false,true);
		_GFX.setRenderState(s);
		_sceneGraph->render();
	_deferredBuffer->End();
} 

void CubeScene::processInput(){
	Scene::processInput();

	Camera* cam = CameraManager::getInstance().getActiveCamera();

	moveFB  = Application::getInstance().moveFB;
	moveLR  = Application::getInstance().moveLR;
	angleLR = Application::getInstance().angleLR;
	angleUD = Application::getInstance().angleUD;
	
	if(angleLR)	cam->RotateX(angleLR * Framerate::getInstance().getSpeedfactor());
	if(angleUD)	cam->RotateY(angleUD * Framerate::getInstance().getSpeedfactor());
	if(moveFB)	cam->PlayerMoveForward(moveFB * (Framerate::getInstance().getSpeedfactor()/5));
	if(moveLR)	cam->PlayerMoveStrafe(moveLR * (Framerate::getInstance().getSpeedfactor()/5));

}

bool CubeScene::load(const string& name)
{
	bool state = false;
	state = loadResources(true);	
	return state;
}

bool CubeScene::loadResources(bool continueOnErrors)
{
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;

	_GFX.setDeferredShading(true);
	_deferredBuffer = _GFX.newFBO();
	_lightTexture = _GFX.newPBO();
	//30 lights? >:)
	
	for(U8 row=0; row<3; row++)
		for(U8 col=0; col < 10; col++){
			U8 lightId = (U8)(row*10+col);
			stringstream ss; ss << (U32)lightId;
			ResourceDescriptor tempLight("Light Deferred " + ss.str());
			tempLight.setId(lightId);
			Light* light = _resManager.loadResource<Light>(tempLight);
			light->toggleImpostor(true);
			_sceneGraph->getRoot()->addNode(light);
			getLights().push_back(light);
	}
	addDefaultLight();
	ResourceDescriptor deferred("DeferredShadingPass2");
	_deferredShader = _resManager.loadResource<Shader>(deferred);

	_deferredBuffer->Create(FrameBufferObject::FBO_2D_DEFERRED,Application::getInstance().getWindowDimensions().x,Application::getInstance().getWindowDimensions().y);
	_lightTexture->Create(2,getLights().size());

	F32 width = Application::getInstance().getWindowDimensions().width;
	F32 height = Application::getInstance().getWindowDimensions().height;
	ResourceDescriptor mrt("MRT RenderQuad");
	mrt.setFlag(true); //no default material
	_renderQuad = _resManager.loadResource<Quad3D>(mrt);
	assert(_renderQuad);
	_renderQuad->getCorner(Quad3D::TOP_LEFT) = vec3(0, height, 0);
	_renderQuad->getCorner(Quad3D::TOP_RIGHT) = vec3(width, height, 0);
	_renderQuad->getCorner(Quad3D::BOTTOM_LEFT) = vec3(0,0,0);
	_renderQuad->getCorner(Quad3D::BOTTOM_RIGHT) = vec3(width, 0, 0);

	_eventTimers.push_back(0.0f);
	return true;
}

bool CubeScene::unload()
{
	Console::getInstance().printfn("Deleting Deferred Rendering RenderTarget!");
	RemoveResource(_renderQuad);
	return Scene::unload();
}

void CubeScene::onKeyDown(const OIS::KeyEvent& key)
{
	Scene::onKeyDown(key);
	switch(key.key)
	{
		case OIS::KC_W:
			Application::getInstance().moveFB = 0.25f;
			break;
		case OIS::KC_A:
			Application::getInstance().moveLR = 0.25f;
			break;
		case OIS::KC_S:
			Application::getInstance().moveFB = -0.25f;
			break;
		case OIS::KC_D:
			Application::getInstance().moveLR = -0.25f;
			break;
		default:
			break;
	}
}

void CubeScene::onKeyUp(const OIS::KeyEvent& key)
{
	Scene::onKeyUp(key);
	switch(key.key)
	{
		case OIS::KC_W:
		case OIS::KC_S:
			Application::getInstance().moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			Application::getInstance().moveLR = 0;
			break;
		default:
			break;
	}

}
