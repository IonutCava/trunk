#include "Headers/PhysXScene.h"
#include "Headers/PhysXImplementation.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "GUI/Headers/GUI.h"
#include "Environment/Sky/Headers/Sky.h"
#include "Managers/Headers/CameraManager.h"
using namespace std;

//begin copy-paste: randarea scenei
void PhysXScene::render(){
	Sky& sky = Sky::getInstance();

	sky.setParams(CameraManager::getInstance().getActiveCamera()->getEye(),_sunVector, false,true,true);
	sky.draw();

	_sceneGraph->render();

}
//end copy-paste

//begin copy-paste: Desenam un cer standard

void PhysXScene::preRender(){

}
//<<end copy-paste

void PhysXScene::processEvents(F32 time){
	F32 FpsDisplay = 0.3f;
	if (time - _eventTimers[0] >= FpsDisplay){
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		GUI::getInstance().modifyText("RenderBinCount", "Number of items in Render Bin: %d", RenderQueue::getInstance().getRenderQueueStackSize());
		_eventTimers[0] += FpsDisplay;
	}
}


void PhysXScene::processInput(){
	Scene::processInput();

	Camera* cam = CameraManager::getInstance().getActiveCamera();
	moveFB  = Application::getInstance().moveFB;
	moveLR  = Application::getInstance().moveLR;
	angleLR = Application::getInstance().angleLR;
	angleUD = Application::getInstance().angleUD;
	
	if(angleLR)	cam->RotateX(angleLR * Framerate::getInstance().getSpeedfactor()/5);
	if(angleUD)	cam->RotateY(angleUD * Framerate::getInstance().getSpeedfactor()/5);
	if(moveFB || moveLR){
		if(moveFB) cam->PlayerMoveForward(moveFB * Framerate::getInstance().getSpeedfactor());
		if(moveLR) cam->PlayerMoveStrafe(moveLR * Framerate::getInstance().getSpeedfactor());
	}
}

bool PhysXScene::load(const string& name){
	setInitialData();
	bool state = false;
	//Adaugam o lumina
	vec2 sunAngle(0.0f, RADIANS(45.0f));
	_sunVector = vec4(-cosf(sunAngle.x) * sinf(sunAngle.y),-cosf(sunAngle.y),-sinf(sunAngle.x) * sinf(sunAngle.y),0.0f );
	Light* light = addDefaultLight();
	light->setLightProperties(LIGHT_POSITION,_sunVector);
	light->setLightProperties(LIGHT_AMBIENT,vec4(1.0f,1.0f,1.0f,1.0f));
	light->setLightProperties(LIGHT_DIFFUSE,vec4(1.0f,1.0f,1.0f,1.0f));
	light->setLightProperties(LIGHT_SPECULAR,vec4(1.0f,1.0f,1.0f,1.0f));
	//Incarcam resursele scenei
	state = loadResources(true);	
	state = loadEvents(true);
	return state;
}

bool PhysXScene::loadResources(bool continueOnErrors){
	 _mousePressed = false;
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;

	GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec3(60,20,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3(0.0f,0.2f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
	GUI::getInstance().addText("RenderBinCount",
								vec3(60,30,0),
								BITMAP_8_BY_13,
								vec3(0.6f,0.2f,0.2f),
								"Number of items in Render Bin: %d",0);

	
	_eventTimers.push_back(0.0f); //Fps
	//Add a new physics scene
	_physx = static_cast<PhysXImplementation* >(PHYSICS_DEVICE.NewSceneInterface(this));
	//Initialize the physics scene
	_physx->init();
	CameraManager::getInstance().getActiveCamera()->RotateX(RADIANS(-75));
	CameraManager::getInstance().getActiveCamera()->RotateY(RADIANS(25));
	CameraManager::getInstance().getActiveCamera()->setEye(vec3(0,30,-40));
	return true;
}

bool PhysXScene::unload(){
	Sky::getInstance().DestroyInstance();
	if(_physx){
		_physx->exit();
		delete _physx;
	}
	return Scene::unload();
}

void PhysXScene::createStack(){
	F32 stackSize = 10;
	F32 CubeSize = 1.0f;
	F32 Spacing = 0.0001f;
	vec3 Pos(0, 10 + CubeSize,0);
	F32 Offset = -stackSize * (CubeSize * 2.0f + Spacing) * 0.5f + 0;
	while(stackSize){
		for(U16 i=0;i<stackSize;i++){
			Pos.x = Offset + i * (CubeSize * 2.0f + Spacing);
			PHYSICS_DEVICE.createBox(_physx,Pos,CubeSize);
		}
		Offset += CubeSize;
		Pos.y += (CubeSize * 2.0f + Spacing);
		stackSize--;
	}
}

void PhysXScene::onKeyDown(const OIS::KeyEvent& key){
	Scene::onKeyDown(key);
	switch(key.key)	{
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
		case OIS::KC_1:
			PHYSICS_DEVICE.createPlane(_physx,vec3(0,0,0),random(0.5f,2.0f));
			break;
		case OIS::KC_2:
			PHYSICS_DEVICE.createBox(_physx,vec3(0,random(10,30),0),random(0.5f,2.0f));
			break;
		case OIS::KC_3:
			//create tower of 10 boxes
			for(U8 i = 0 ; i < 10; i++){
				PHYSICS_DEVICE.createBox(_physx,vec3(0,5.0f+5*i,0),0.5f);
			}
			break;
		case OIS::KC_4:{
			Event_ptr e(New Event(0,true,true,boost::bind(&PhysXScene::createStack, boost::ref(*this))));
			addEvent(e);
			/*F32 stackSize = 50;
			F32 CubeSize = 1.0f;
			F32 Spacing = 0.0001f;
			vec3 Pos(0, 10 + CubeSize,0);
		    F32 Offset = -stackSize * (CubeSize * 2.0f + Spacing) * 0.5f + 0;
			while(stackSize){
				for(U16 i=0;i<stackSize;i++){
					Pos.x = Offset + i * (CubeSize * 2.0f + Spacing);
					PHYSICS_DEVICE.createBox(_physx,Pos,CubeSize);
				}
				Offset += CubeSize;
				Pos.y += (CubeSize * 2.0f + Spacing);
				stackSize--;
			}*/
		}
		   break;
		default:
			break;
	}
}

void PhysXScene::onKeyUp(const OIS::KeyEvent& key){
	Scene::onKeyUp(key);
	switch(key.key)	{
		case OIS::KC_W:
		case OIS::KC_S:
			Application::getInstance().moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			Application::getInstance().moveLR = 0;
			break;
		case OIS::KC_F1:
			_sceneGraph->print();
			break;
		default:
			break;
	}

}


void PhysXScene::onMouseMove(const OIS::MouseEvent& key){
	Scene::onMouseMove(key);
	if(_mousePressed){
		if(_prevMouse.x - key.state.X.abs > 1 )
			Application::getInstance().angleLR = -0.15f;
		else if(_prevMouse.x - key.state.X.abs < -1 )
			Application::getInstance().angleLR = 0.15f;
		else
			Application::getInstance().angleLR = 0;

		if(_prevMouse.y - key.state.Y.abs > 1 )
			Application::getInstance().angleUD = -0.1f;
		else if(_prevMouse.y - key.state.Y.abs < -1 )
			Application::getInstance().angleUD = 0.1f;
		else
			Application::getInstance().angleUD = 0;
	}
	
	_prevMouse.x = key.state.X.abs;
	_prevMouse.y = key.state.Y.abs;
}

void PhysXScene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickDown(key,button);
	if(button == 0) 
		_mousePressed = true;
}

void PhysXScene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickUp(key,button);
	if(button == 0)	{
		_mousePressed = false;
		Application::getInstance().angleUD = 0;
		Application::getInstance().angleLR = 0;
	}
}