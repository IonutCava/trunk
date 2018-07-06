#include "CubeScene.h"
#include "Managers/CameraManager.h"
#include "Rendering/common.h"
#include "PhysX/PhysX.h"
#include "GUI/GUI.h"
#include "Geometry/Predefined/Quad3D.h"
#include "Managers/ResourceManager.h"
using namespace std;

void CubeScene::render()
{
	RenderState s(true,true,false,true);
	GFXDevice::getInstance().setRenderState(s);
	vec3 camPosition = CameraManager::getInstance().getActiveCamera()->getEye();

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
		GFXDevice::getInstance().clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
		GFXDevice::getInstance().renderElements(GeometryArray);
		for(U8 i = 0; i < getLights().size(); i++)
			getLights()[i]->draw();
		GUI::getInstance().draw();
	_deferredBuffer->End();
	
	//Pass 2
	//Draw a 2D fullscreen quad that has the lighting shader applied to it and all generated textures bound to that shader
	GFXDevice::getInstance().toggle2D(true);
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
			_deferredShader->Uniform("cameraPosition",camPosition);

			GFXDevice::getInstance().drawQuad3D(_renderQuad);

		_lightTexture->Unbind(3);
		_deferredBuffer->Unbind(2);
		_deferredBuffer->Unbind(1);
		_deferredBuffer->Unbind(0);
	_deferredShader->unbind();
	GFXDevice::getInstance().toggle2D(false);


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

	for(tr1::unordered_map<string,Object3D*>::iterator iter = GeometryArray.begin(); iter != GeometryArray.end(); iter++)
	{
		if((iter->second)->getName().compare("Cutia1") == 0)
			(iter->second)->getTransform()->rotateEuler(vec3(0.3f*i, 0.6f*i,0));
		if((iter->second)->getName().compare("HelloText") == 0)
			(iter->second)->getTransform()->rotate(vec3(0.6f,0.2f,0.4f),i);
		if((iter->second)->getName().compare("Bila") == 0)
			(iter->second)->getTransform()->translateY(j*0.25f);
	}

	if(GeometryArray["dwarf"])
		GeometryArray["dwarf"]->getTransform()->rotate(vec3(0,1,0),i);

	if(PhysX::getInstance().getScene() != NULL)	
		PhysX::getInstance().UpdateActors();
} 

void CubeScene::processInput()
{
	_inputManager.tick();

	Camera* cam = CameraManager::getInstance().getActiveCamera();

	moveFB  = Engine::getInstance().moveFB;
	moveLR  = Engine::getInstance().moveLR;
	angleLR = Engine::getInstance().angleLR;
	angleUD = Engine::getInstance().angleUD;
	
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

	GFXDevice::getInstance().setDeferredShading(true);
	_deferredBuffer = GFXDevice::getInstance().newFBO();
	_lightTexture = GFXDevice::getInstance().newPBO();
	//30 lights? >:)
	for(U8 row=0; row<3; row++)
		for(U8 col=0; col < 10; col++){
			Light_ptr light(new Light((U8)(row*10+col)));
			getLights().push_back(light);
	}

	_deferredShader = ResourceManager::getInstance().LoadResource<Shader>("DeferredShadingPass2");

	_deferredBuffer->Create(FrameBufferObject::FBO_2D_DEFERRED,Engine::getInstance().getWindowDimensions().x,Engine::getInstance().getWindowDimensions().y);
	_lightTexture->Create(2,getLights().size());

	F32 width = Engine::getInstance().getWindowDimensions().width;
	F32 height = Engine::getInstance().getWindowDimensions().height;
	_renderQuad = ResourceManager::getInstance().LoadResource<Quad3D>("MRT RenderQuad",true);
	_renderQuad->getCorner(Quad3D::TOP_LEFT) = vec3(0, height, 0);
	_renderQuad->getCorner(Quad3D::TOP_RIGHT) = vec3(width, height, 0);
	_renderQuad->getCorner(Quad3D::BOTTOM_LEFT) = vec3(0,0,0);
	_renderQuad->getCorner(Quad3D::BOTTOM_RIGHT) = vec3(width, 0, 0);
	_renderQuad->getMaterial().diffuse = vec4(1,1,1,1);
	_renderQuad->getTransform()->setPosition(vec3(0,0,-1));
	_eventTimers.push_back(0.0f);
	return true;
}

bool CubeScene::unload()
{
	Con::getInstance().printfn("Deleting Deferred Rendering RenderTarget!");
	ResourceManager::getInstance().remove(_renderQuad);
	_renderQuad = NULL;
	return Scene::unload();
}

void CubeScene::onKeyDown(const OIS::KeyEvent& key)
{
	Scene::onKeyDown(key);
	switch(key.key)
	{
		case OIS::KC_W:
			Engine::getInstance().moveFB = 0.25f;
			break;
		case OIS::KC_A:
			Engine::getInstance().moveLR = 0.25f;
			break;
		case OIS::KC_S:
			Engine::getInstance().moveFB = -0.25f;
			break;
		case OIS::KC_D:
			Engine::getInstance().moveLR = -0.25f;
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
			Engine::getInstance().moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			Engine::getInstance().moveLR = 0;
			break;
		default:
			break;
	}

}
