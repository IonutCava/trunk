#include "Headers/PhysXScene.h"
#include "Headers/PhysXImplementation.h"

#include "GUI/Headers/GUI.h"
#include "Environment/Sky/Headers/Sky.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

//begin copy-paste: randarea scenei
void PhysXScene::render(){
	Sky& sky = Sky::getInstance();

	sky.setParams(_camera->getEye(),_sunVector, false,true,true);
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
		GUI::getInstance().modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
		_eventTimers[0] += FpsDisplay;
	}
}


void PhysXScene::processInput(){
	Scene::processInput();

	if(_angleLR) _camera->RotateX(_angleLR * Framerate::getInstance().getSpeedfactor());
	if(_angleUD) _camera->RotateY(_angleUD * Framerate::getInstance().getSpeedfactor());
	if(_moveFB)  _camera->MoveForward(_moveFB * (Framerate::getInstance().getSpeedfactor()/5));
	if(_moveLR)	 _camera->MoveStrafe(_moveLR * (Framerate::getInstance().getSpeedfactor()/5));
}

bool PhysXScene::load(const std::string& name){

	setInitialData();
	bool state = false;
	//Adaugam o lumina
	vec2<F32> sunAngle(0.0f, RADIANS(45.0f));
	_sunVector = vec4<F32>(-cosf(sunAngle.x) * sinf(sunAngle.y),-cosf(sunAngle.y),-sinf(sunAngle.x) * sinf(sunAngle.y),0.0f );
	Light* light = addDefaultLight();
	light->setLightProperties(LIGHT_POSITION,_sunVector);
	light->setLightProperties(LIGHT_AMBIENT,vec4<F32>(1.0f,1.0f,1.0f,1.0f));
	light->setLightProperties(LIGHT_DIFFUSE,vec4<F32>(1.0f,1.0f,1.0f,1.0f));
	light->setLightProperties(LIGHT_SPECULAR,vec4<F32>(1.0f,1.0f,1.0f,1.0f));
	//Incarcam resursele scenei
	state = loadResources(true);	
	state = loadEvents(true);
	return state;
}

bool PhysXScene::loadResources(bool continueOnErrors){
	 _mousePressed = false;

	GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec3<F32>(60,20,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3<F32>(0.0f,0.2f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
	GUI::getInstance().addText("RenderBinCount",
								vec3<F32>(60,30,0),
								BITMAP_8_BY_13,
								vec3<F32>(0.6f,0.2f,0.2f),
								"Number of items in Render Bin: %d",0);

	
	_eventTimers.push_back(0.0f); //Fps
	//Add a new physics scene
	_physx = static_cast<PhysXImplementation* >(PHYSICS_DEVICE.NewSceneInterface(this));
	//Initialize the physics scene
	_physx->init();
	_camera->RotateX(RADIANS(-75));
	_camera->RotateY(RADIANS(25));
	_camera->setEye(vec3<F32>(0,30,-40));
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
	vec3<F32> Pos(0, 10 + CubeSize,0);
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
			_moveFB = 0.25f;
			break;
		case OIS::KC_A:
			_moveLR = 0.25f;
			break;
		case OIS::KC_S:
			_moveFB = -0.25f;
			break;
		case OIS::KC_D:
			_moveLR = -0.25f;
			break;
		case OIS::KC_1:
			PHYSICS_DEVICE.createPlane(_physx,vec3<F32>(0,0,0),random(0.5f,2.0f));
			break;
		case OIS::KC_2:
			PHYSICS_DEVICE.createBox(_physx,vec3<F32>(0,random(10,30),0),random(0.5f,2.0f));
			break;
		case OIS::KC_3:
			//create tower of 10 boxes
			for(U8 i = 0 ; i < 10; i++){
				PHYSICS_DEVICE.createBox(_physx,vec3<F32>(0,5.0f+5*i,0),0.5f);
			}
			break;
		case OIS::KC_4:{
			Event_ptr e(New Event(0,true,true,boost::bind(&PhysXScene::createStack, boost::ref(*this))));
			addEvent(e);
			/*F32 stackSize = 50;
			F32 CubeSize = 1.0f;
			F32 Spacing = 0.0001f;
			vec3<F32> Pos(0, 10 + CubeSize,0);
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
			_moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			_moveLR = 0;
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
			_angleLR = -0.15f;
		else if(_prevMouse.x - key.state.X.abs < -1 )
			_angleLR = 0.15f;
		else
			_angleLR = 0;

		if(_prevMouse.y - key.state.Y.abs > 1 )
			_angleUD = -0.1f;
		else if(_prevMouse.y - key.state.Y.abs < -1 )
			_angleUD = 0.1f;
		else
			_angleUD = 0;
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
		_angleUD = 0;
		_angleLR = 0;
	}
}