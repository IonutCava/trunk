#include "Headers/PingPongScene.h"

#include "GUI/Headers/GUI.h"
#include "Rendering/Headers/Frustum.h"
#include "Environment/Sky/Headers/Sky.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

//vec4<F32> _lightPosition(0,16,6,0.0);

//begin copy-paste: randarea scenei
void PingPongScene::render(){
	Sky& sky = Sky::getInstance();

	sky.setParams(_camera->getEye(),vec3<F32>(_sunVector),false,true,true);
	sky.draw();

	_sceneGraph->render();
}
//end copy-paste

//begin copy-paste: Desenam un cer standard
void PingPongScene::preRender(){
	vec2<F32> _sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
	_sunVector = vec4<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );

	LightManager::getInstance().getLight(0)->setLightProperties(LIGHT_POSITION,_sunVector);
}
//<<end copy-paste

void PingPongScene::processEvents(F32 time){

	F32 FpsDisplay = 0.3f;
	if (time - _eventTimers[0] >= FpsDisplay){
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		_eventTimers[0] += FpsDisplay;
	}
}

void PingPongScene::resetGame(){
	_directionTowardsAdversary = true;
	_upwardsDirection = false;
	_touchedAdversaryTableHalf = false;
	_touchedOwnTableHalf = false;
	_lost = false;
	_sideDrift = 0;
	getEvents().clear();
	_ballSGN->getTransform()->setPosition(vec3<F32>(0, 2 ,2));
}

void PingPongScene::serveBall(){
	GUI::getInstance().modifyText("insults","");
	resetGame();

	if(getEvents().empty()){///A maximum of 1 events allowed
		Event_ptr newGame(New Event(30,true,false,boost::bind(&PingPongScene::test,this,rand() % 5,TYPE_INTEGER)));
		addEvent(newGame);
	}
}

void PingPongScene::test(boost::any a, CallbackParam b){
	if(getEvents().empty()) return;
	bool updated = false;
	std::string message;
	Transform* ballTransform = _ballSGN->getTransform();
	vec3<F32> ballPosition  = ballTransform->getPosition();

	
	SceneGraphNode* table = _sceneGraph->findNode("table");
	SceneGraphNode* net = _sceneGraph->findNode("net");
	SceneGraphNode* opponent = _sceneGraph->findNode("opponent");
	SceneGraphNode* paddle = _sceneGraph->findNode("paddle");
	vec3<F32> paddlePosition   = paddle->getTransform()->getPosition();
	vec3<F32> opponentPosition = opponent->getTransform()->getPosition();
	vec3<F32> tablePosition     = table->getTransform()->getPosition();

	///Is the ball coming towards us or towards the opponent?
	_directionTowardsAdversary ? ballPosition.z -= 0.11f : ballPosition.z += 0.11f;
	///Up or down?
	_upwardsDirection ? 	ballPosition.y += 0.084f : 	ballPosition.y -= 0.084f;

	///Is the ball moving to the right or to the left?
	ballPosition.x += _sideDrift*0.15f;
	if(opponentPosition.x != ballPosition.x)
		opponent->getTransform()->translateX(ballPosition.x - opponentPosition.x);

	ballTransform->translate(ballPosition - ballTransform->getPosition());

	///Did we hit the table? Bounce then ...
	if(table->getBoundingBox().Collision(_ballSGN->getBoundingBox())){
		if(ballPosition.z > tablePosition.z){
			_touchedOwnTableHalf = true;
			_touchedAdversaryTableHalf = false;
		}else{
			_touchedOwnTableHalf = false;
			_touchedAdversaryTableHalf = true;
		}
		_upwardsDirection = true;
	}
	///Kinetic  energy depletion
	if(ballPosition.y > 2.1f) _upwardsDirection = false;

	///Did we hit the paddle?
	if(_ballSGN->getBoundingBox().Collision(paddle->getBoundingBox())){
		_sideDrift = ballPosition.x - paddlePosition.x;
		///If we hit the ball with the upper margin of the paddle, add a slight impuls to the ball
		if(ballPosition.y >= paddlePosition.y) ballPosition.z -= 0.12f;

		_directionTowardsAdversary = true;
	}

	if(ballPosition.y + 0.75f < table->getBoundingBox().getMax().y){
		///If we hit the ball and it landed on the opponent's table half
		///Or if the opponent hit the ball and it landed on our table half
		if((_touchedAdversaryTableHalf && _directionTowardsAdversary) || 
		   (!_directionTowardsAdversary && !_touchedOwnTableHalf))		
			_lost = false;
		else
			_lost = true;
		
		updated = true;
	}
	///Did we win or lose?
	if(ballPosition.z >= paddlePosition.z){
		_lost = true;
		updated = true;
	}
	if(ballPosition.z <= opponentPosition.z){
		_lost = false;
		updated = true;
	}

	if(_ballSGN->getBoundingBox().Collision(net->getBoundingBox())){
		if(_directionTowardsAdversary){
			///Did we hit the net?
			_lost = true;
		}else{
			///Did the opponent hit the net?
			_lost = false;
		}
		updated = true;
		
    }
	
	///Did we hit the opponent? Then change ball direction ... BUT ...
	///Add a small chance that we win
	if(random(30) != 2)
	if(_ballSGN->getBoundingBox().Collision(opponent->getBoundingBox())){
		_sideDrift = ballPosition.x - opponent->getTransform()->getPosition().x;
		_directionTowardsAdversary = false;
	}
	///Add a spin effect to the ball
	ballTransform->rotateEuler(vec3<F32>(ballPosition.z,1,1));

	if(updated){
		if(_lost){
			message = "You lost!";
			_scor--;

			if(b == TYPE_INTEGER){
				I32 quote = boost::any_cast<I32>(a);
				if(_scor % 3 == 0 ) GUI::getInstance().modifyText("insults",(char*)_quotes[quote].c_str());
			}
		}else{
			message = "You won!";
			_scor++;
		}
		
		GUI::getInstance().modifyText("Score","Score: %d",_scor);
		GUI::getInstance().modifyText("Message",(char*)message.c_str());
		resetGame();
	}
}

void PingPongScene::processInput(){

	///Move FB = Forward/Back = up/down
	///Move LR = Left/Right

	///Camera controls
	if(_angleLR) _camera->RotateX(_angleLR * Framerate::getInstance().getSpeedfactor());
	if(_angleUD) _camera->RotateY(_angleUD * Framerate::getInstance().getSpeedfactor());

	SceneGraphNode* paddle = _sceneGraph->findNode("paddle");

	vec3<F32> pos = paddle->getTransform()->getPosition();

	///Paddle movement is limited to the [-3,3] range except for Y-descent
	if(_moveFB){
		if((_moveFB > 0 && pos.y >= 3) || (_moveFB < 0 && pos.y <= 0.5f)) return;
		paddle->getTransform()->translateY((_moveFB * Framerate::getInstance().getSpeedfactor())/6);
	}

	if(_moveLR){
		///Left/right movement is flipped for proper control
		if((_moveLR < 0 && pos.x >= 3) || (_moveLR > 0 && pos.x <= -3)) return;
		paddle->getTransform()->translateX((-_moveLR * Framerate::getInstance().getSpeedfactor())/6);
	}
}

bool PingPongScene::load(const std::string& name){

	setInitialData();
	bool state = false;
	///Add a light
	Light* light = addDefaultLight();
	///Load scene resources
	state = loadResources(true);	
	state = loadEvents(true);
	///Position the camera
	_camera->setAngleX(RADIANS(-90));
	_camera->setEye(vec3<F32>(0,2.5f,6.5f));
	
	return state;
}

bool PingPongScene::loadResources(bool continueOnErrors){

	///Create a ball
	ResourceDescriptor minge("Ping Pong Ball");
	_ball = CreateResource<Sphere3D>(minge);
	_ballSGN = addGeometry(_ball);
	_ball->setResolution(16);
	_ball->setRadius(0.1f);
	_ballSGN->getTransform()->translate(vec3<F32>(0, 2 ,2));
	_ball->getMaterial()->setDiffuse(vec4<F32>(0.4f,0.4f,0.4f,1.0f));
	_ball->getMaterial()->setAmbient(vec4<F32>(0.25f,0.25f,0.25f,1.0f));
	_ball->getMaterial()->setShininess(36.8f);
	_ball->getMaterial()->setSpecular(vec4<F32>(0.774597f,0.774597f,0.774597f,1.0f));

	///Buttons and text labels
	GUI::getInstance().addButton("Serve", "Serve", vec2<F32>(_cachedResolution.width-120 ,
															 _cachedResolution.height/1.1f),
													    	 vec2<U16>(100,25),vec3<F32>(0.65f,0.65f,0.65f),
															 boost::bind(&PingPongScene::serveBall,this));

	GUI::getInstance().addText("Score",vec3<F32>(_cachedResolution.width - 120, _cachedResolution.height/1.3f, 0),
							   BITMAP_8_BY_13,vec3<F32>(1,0,0), "Score: %d",0);

	GUI::getInstance().addText("Message",vec3<F32>(_cachedResolution.width - 120, _cachedResolution.height/1.5f, 0),
							   BITMAP_8_BY_13,vec3<F32>(1,0,0), "");
	GUI::getInstance().addText("insults",vec3<F32>(_cachedResolution.width/4, _cachedResolution.height/3, 0),
							   BITMAP_TIMES_ROMAN_24,vec3<F32>(0,1,0), "");
	GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec3<F32>(60,60,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3<F32>(0.0f,0.2f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
	///Add some taunts
	_quotes.push_back("Ha ha ... even Odin's laughin'!");
	_quotes.push_back("If you're a ping-pong player, I'm Jimmy Page");
	_quotes.push_back("Ooolee, ole ole ole, see the ball? ... It's past your end");
	_quotes.push_back("You're lucky the room's empty. I'd be so ashamed otherwise if I were you");
	_quotes.push_back("It's not the hard. Even a monkey can do it.");
	
	_eventTimers.push_back(0.0f); //Fps
	_eventTimers.push_back(0.0f); //Light
	return true;
}

bool PingPongScene::unload(){
	Sky::getInstance().DestroyInstance();
	return Scene::unload();
}

void PingPongScene::onKeyDown(const OIS::KeyEvent& key){

	Scene::onKeyDown(key);
	switch(key.key){

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
		default:
			break;
	}
}

void PingPongScene::onKeyUp(const OIS::KeyEvent& key){

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
		default:
			break;
	}

}

void PingPongScene::OnJoystickMovePOV(const OIS::JoyStickEvent& key,I8 pov){

	Scene::OnJoystickMovePOV(key,pov);
	if( key.state.mPOV[pov].direction & OIS::Pov::North ) //Going up
		_moveFB = 0.25f;
	else if( key.state.mPOV[pov].direction & OIS::Pov::South ) //Going down
		_moveFB = -0.25f;

	if( key.state.mPOV[pov].direction & OIS::Pov::East ) //Going right
		_moveLR = -0.25f;

	else if( key.state.mPOV[pov].direction & OIS::Pov::West ) //Going left
		_moveLR = 0.25f;

	if( key.state.mPOV[pov].direction == OIS::Pov::Centered ){ //stopped/centered out
		_moveLR = 0;
		_moveFB = 0;
	}
}

void PingPongScene::OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis){

	Scene::OnJoystickMoveAxis(key,axis);
}

void PingPongScene::OnJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button){

	if(button == 0)  serveBall();
}