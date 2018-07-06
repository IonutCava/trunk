#include "Headers/TenisScene.h"
#include "Headers/TenisSceneAISceneImpl.h"

#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Geometry/Material/Headers/Material.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/AIManager.h"
#include "GUI/Headers/GUIButton.h"
#include "GUI/Headers/GUI.h"

REGISTER_SCENE(TenisScene);

static boost::atomic_bool s_gameStarted;

void TenisScene::preRender(){
    vec2<F32> _sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
    _sunvector = vec3<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
                            -cosf(_sunAngle.y),
                            -sinf(_sunAngle.x) * sinf(_sunAngle.y));

    //LightManager::getInstance().getLight(0)->setDirection(_sunvector);
    getSkySGN(0)->getNode<Sky>()->setSunVector(_sunvector);
}

void TenisScene::processGUI(const U64 deltaTime){
    D32 FpsDisplay = 0.7;
    if (_guiTimers[0] >= FpsDisplay){
        _GUI->modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f", ApplicationTimer::getInstance().getFps(), ApplicationTimer::getInstance().getFrameTime());
        _GUI->modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
        _guiTimers[0] = 0.0;
    }
    Scene::processGUI(deltaTime);
}

void TenisScene::processTasks(const U64 deltaTime){


    Scene::processTasks(deltaTime);

    checkCollisions();

    if(s_gameStarted && !_gamePlaying && _scoreTeam1 < 10 && _scoreTeam2 < 10)
        startGame();

    if(_scoreTeam1 == 10 || _scoreTeam2 == 10){
        s_gameStarted = false;
        _GUI->modifyText("Message", "Team %d won!", _scoreTeam1 == 10 ? 1 : 2);
    }
}

void TenisScene::resetGame(){
    removeTask(_gameGUID);
    _collisionPlayer1 = false;
    _collisionPlayer2 = false;
    _collisionPlayer3 = false;
    _collisionPlayer4 = false;
    _collisionNet = false;
    _collisionFloor = false;
    _gamePlaying = true;
    _upwardsDirection = true;
    _touchedTerrainTeam1 = false;
    _touchedTerrainTeam2 = false;
    _applySideImpulse = false;
    _sideImpulseFactor = 0;
    WriteLock w_lock(_gameLock);
    _ballSGN->getComponent<PhysicsComponent>()->setPosition(vec3<F32>((random(0,10) >= 5) ? 3.0f : -3.0f, 0.2f, _lostTeam1 ? -7.0f : 7.0f));
    _directionTeam1ToTeam2 = !_lostTeam1;
    _lostTeam1 = false;
    s_gameStarted = true;
}

void TenisScene::startGame(){
    resetGame();
    Kernel* kernel = Application::getInstance().getKernel();
    Task_ptr newGame(kernel->AddTask(15, true, false, DELEGATE_BIND(&TenisScene::playGame, this, rand() % 5, TYPE_INTEGER)));
    addTask(newGame);
    _gameGUID = newGame->getGUID();
}

void TenisScene::checkCollisions(){
    SceneGraphNode* Player1 = _aiPlayer1->getUnitRef()->getBoundNode();
    SceneGraphNode* Player2 = _aiPlayer2->getUnitRef()->getBoundNode();
    SceneGraphNode* Player3 = _aiPlayer3->getUnitRef()->getBoundNode();
    SceneGraphNode* Player4 = _aiPlayer4->getUnitRef()->getBoundNode();
    BoundingBox ballBB      = _ballSGN->getBoundingBoxConst();
    BoundingBox floorBB     = _floor->getBoundingBoxConst();
    WriteLock w_lock(_gameLock);
    _collisionPlayer1 = ballBB.Collision(Player1->getBoundingBox());
    _collisionPlayer2 = ballBB.Collision(Player2->getBoundingBox());
    _collisionPlayer3 = ballBB.Collision(Player3->getBoundingBox());
    _collisionPlayer4 = ballBB.Collision(Player4->getBoundingBox());
    _collisionNet     = ballBB.Collision(_net->getBoundingBox());
    _collisionFloor   = floorBB.Collision(ballBB);
}

//Team 1: Player1 + Player2
//Team 2: Player3 + Player4
void TenisScene::playGame(cdiggins::any a, CallbackParam b){
    bool updated = false;
    std::string message;
    if(!_gamePlaying){
        return;
    }

    UpgradableReadLock ur_lock(_gameLock);
    //Shortcut to the scene graph nodes containing our agents
    SceneGraphNode* Player1 = _aiPlayer1->getUnitRef()->getBoundNode();
    SceneGraphNode* Player2 = _aiPlayer2->getUnitRef()->getBoundNode();
    SceneGraphNode* Player3 = _aiPlayer3->getUnitRef()->getBoundNode();
    SceneGraphNode* Player4 = _aiPlayer4->getUnitRef()->getBoundNode();
    //Store by copy (thread-safe) current ball position (getPosition()) should be threadsafe
    vec3<F32> netPosition  = _net->getComponent<PhysicsComponent>()->getConstTransform()->getPosition();
    vec3<F32> ballPosition = _ballSGN->getComponent<PhysicsComponent>()->getConstTransform()->getPosition();
    vec3<F32> player1Pos   = Player1->getComponent<PhysicsComponent>()->getConstTransform()->getPosition();
    vec3<F32> player2Pos   = Player2->getComponent<PhysicsComponent>()->getConstTransform()->getPosition();
    vec3<F32> player3Pos   = Player3->getComponent<PhysicsComponent>()->getConstTransform()->getPosition();
    vec3<F32> player4Pos   = Player4->getComponent<PhysicsComponent>()->getConstTransform()->getPosition();
    vec3<F32> netBBMax     = _net->getBoundingBox().getMax();
    vec3<F32> netBBMin     = _net->getBoundingBox().getMin();

    UpgradeToWriteLock uw_lock(ur_lock);
    //Is the ball traveling from team 1 to team 2?
    _directionTeam1ToTeam2 ? ballPosition.z -= 0.123f : ballPosition.z += 0.123f;
    //Is the ball traveling upwards or is it falling?
    _upwardsDirection ? ballPosition.y += 0.066f : ballPosition.y -= 0.066f;
    //In case of a side drift, update accordingly
    if(_applySideImpulse) ballPosition.x += _sideImpulseFactor;

    //After we finish our computations, apply the new transform
    //setPosition/getPosition should be thread-safe
    _ballSGN->getComponent<PhysicsComponent>()->setPosition(ballPosition);
    //Add a spin to the ball just for fun ...
    _ballSGN->getComponent<PhysicsComponent>()->rotate(vec3<F32>(ballPosition.z,1,1));

    //----------------------COLLISIONS------------------------------//
    //z = depth. Descending to the horizon

    if(_collisionFloor){
        if(ballPosition.z > netPosition.z){
            _touchedTerrainTeam1 = true;
            _touchedTerrainTeam2 = false;
        }else{
            _touchedTerrainTeam1 = false;
            _touchedTerrainTeam2 = true;
        }
        _upwardsDirection = true;
    }

    //Where does the Kinetic  energy of the ball run out?
    if(ballPosition.y > 3.5) _upwardsDirection = false;

    //Did we hit a player?

    bool collisionTeam1 = _collisionPlayer1 || _collisionPlayer2;
    bool collisionTeam2 = _collisionPlayer3 || _collisionPlayer4;

    if(collisionTeam1 || collisionTeam2){
        //Something went very wrong. Very.
        //assert(collisionTeam1 != collisionTeam2);
        if(collisionTeam1 == collisionTeam2){
            collisionTeam1 = !_directionTeam1ToTeam2;
            collisionTeam2 = !collisionTeam1;
            D_ERROR_FN("COLLISION ERROR");
        }

        F32 sideDrift = 0;

        if(collisionTeam1){
            _collisionPlayer1 ? sideDrift = player1Pos.x : sideDrift = player2Pos.x;
        }else if(collisionTeam2){
            _collisionPlayer3 ? sideDrift = player3Pos.x : sideDrift = player4Pos.x;
        }

        _sideImpulseFactor = (ballPosition.x - sideDrift);
        _applySideImpulse = !IS_ZERO(_sideImpulseFactor);
        if(_applySideImpulse) _sideImpulseFactor *= 0.025f;

        _directionTeam1ToTeam2 = collisionTeam1;
    }

    //-----------------VALIDATING THE RESULTS----------------------//
    //Which team won?
    if(ballPosition.z >= player1Pos.z && ballPosition.z >= player2Pos.z){
        _lostTeam1 = true;
        updated = true;
    }

    if(ballPosition.z <= player3Pos.z &&  ballPosition.z <= player4Pos.z){
        _lostTeam1 = false;
        updated = true;
    }
    //Which team kicked the ball in the net?
    if(_collisionNet){
        if(ballPosition.y < netBBMax.y - 0.25){
            if(_directionTeam1ToTeam2){
                _lostTeam1 = true;
            }else{
                _lostTeam1 = false;
            }
            updated = true;
        }
    }

    if(ballPosition.x + 0.5f < netBBMin.x || ballPosition.x + 0.5f > netBBMax.x){
        //If we hit the ball and it touched the opposing team's terrain
        //Or if the opposing team hit the ball but it didn't land in our terrain
        if(_collisionFloor){
            if((_touchedTerrainTeam2 && _directionTeam1ToTeam2) || (!_directionTeam1ToTeam2 && !_touchedTerrainTeam1)){
                _lostTeam1 = false;
            }else{
                _lostTeam1 = true;
            }
            updated = true;
        }
    }

    //-----------------DISPLAY RESULTS---------------------//
    if(updated){
        if(_lostTeam1)	{
            message = "Team 2 scored!";
            _scoreTeam2++;
        }else{
            message = "Team 1 scored!";
            _scoreTeam1++;
        }

        _GUI->modifyText("Team1Score","Team 1 Score: %d",_scoreTeam1);
        _GUI->modifyText("Team2Score","Team 2 Score: %d",_scoreTeam2);
        _GUI->modifyText("Message",(char*)message.c_str());
        _gamePlaying = false;
    }
}

void TenisScene::processInput(const U64 deltaTime){
}

bool TenisScene::load(const std::string& name, CameraManager* const cameraMgr, GUI* const gui){
    s_gameStarted = false;
    //Load scene resources
    bool loadState = SCENE_LOAD(name,cameraMgr,gui,true,true);

    //Add a light
    addDefaultLight();
    addDefaultSky();

//	ResourceDescriptor tempLight1("Light omni");
//	tempLight1.setId(0);
//	tempLight1.setEnumValue(LIGHT_TYPE_POINT);
//	light1 = CreateResource<Light>(tempLight1);
//	addLight(light1);

    //Position camera
    //renderState().getCamera().setEye(vec3<F32>(14,5.5f,11.5f));
    //renderState().getCamera().setRotation(10/*yaw*/,-45/*pitch*/);

    //------------------------ Load up game elements -----------------------------///
    _net = _sceneGraph->findNode("Net");
    //FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, _net->getChildren()){
        //it.second->setReceivesShadows(false);
    //}
    _floor = _sceneGraph->findNode("Floor");
    _floor->castsShadows(false);

    AI::AIManager::getInstance().pauseUpdate(false);
    return loadState;
}

bool TenisScene::initializeAI(bool continueOnErrors){
    bool state = false;
    SceneGraphNode* player[4];
    //----------------------------ARTIFICIAL INTELLIGENCE------------------------------//
    player[0] = _sceneGraph->findNode("Player1");
    player[1] = _sceneGraph->findNode("Player2");
    player[2] = _sceneGraph->findNode("Player3");
    player[3] = _sceneGraph->findNode("Player4");
    player[0]->setSelectable(true);
    player[1]->setSelectable(true);
    player[2]->setSelectable(true);
    player[3]->setSelectable(true);

    _aiPlayer1 = New AI::AIEntity(player[0]->getComponent<PhysicsComponent>()->getConstTransform()->getPosition(), "Player1");
    _aiPlayer2 = New AI::AIEntity(player[1]->getComponent<PhysicsComponent>()->getConstTransform()->getPosition(), "Player2");
    _aiPlayer3 = New AI::AIEntity(player[2]->getComponent<PhysicsComponent>()->getConstTransform()->getPosition(), "Player3");
    _aiPlayer4 = New AI::AIEntity(player[3]->getComponent<PhysicsComponent>()->getConstTransform()->getPosition(), "Player4");
    _aiPlayer1->addSensor(AI::VISUAL_SENSOR);
    _aiPlayer2->addSensor(AI::VISUAL_SENSOR);
    _aiPlayer3->addSensor(AI::VISUAL_SENSOR);
    _aiPlayer4->addSensor(AI::VISUAL_SENSOR);

    _aiPlayer1->addAISceneImpl(New AI::TenisSceneAISceneImpl(_ballSGN));
    _aiPlayer2->addAISceneImpl(New AI::TenisSceneAISceneImpl(_ballSGN));
    _aiPlayer3->addAISceneImpl(New AI::TenisSceneAISceneImpl(_ballSGN));
    _aiPlayer4->addAISceneImpl(New AI::TenisSceneAISceneImpl(_ballSGN));

    _team1 = New AI::AITeam(1);
    _team2 = New AI::AITeam(2);

    _aiPlayer1->setTeam(_team1);
    state = _aiPlayer2->addFriend(_aiPlayer1);
    if(state || continueOnErrors){
        _aiPlayer3->setTeam(_team2);
        state = _aiPlayer4->addFriend(_aiPlayer3);
    }
    if(state || continueOnErrors){
        state = AI::AIManager::getInstance().addEntity(_aiPlayer1);
    }
    if(state || continueOnErrors) {
        state = AI::AIManager::getInstance().addEntity(_aiPlayer2);
    }
    if(state || continueOnErrors) {
        state = AI::AIManager::getInstance().addEntity(_aiPlayer3);
    }
    if(state || continueOnErrors) {
        state = AI::AIManager::getInstance().addEntity(_aiPlayer4);
    }
    if(state || continueOnErrors){
    //----------------------- AI controlled units (NPC's) ---------------------//
        _player1 = New NPC(player[0], _aiPlayer1);
        _player2 = New NPC(player[1], _aiPlayer2);
        _player3 = New NPC(player[2], _aiPlayer3);
        _player4 = New NPC(player[3], _aiPlayer4);

        _player1->setMovementSpeed(1.45f);
        _player2->setMovementSpeed(1.5f);
        _player3->setMovementSpeed(1.475f);
        _player4->setMovementSpeed(1.45f);
    }

    if(state || continueOnErrors) Scene::initializeAI(continueOnErrors);
    return state;
}

bool TenisScene::deinitializeAI(bool continueOnErrors){
    AI::AIManager::getInstance().pauseUpdate(true);
    AI::AIManager::getInstance().destroyEntity(_aiPlayer1->getGUID());
    AI::AIManager::getInstance().destroyEntity(_aiPlayer2->getGUID());
    AI::AIManager::getInstance().destroyEntity(_aiPlayer3->getGUID());
    AI::AIManager::getInstance().destroyEntity(_aiPlayer4->getGUID());
    SAFE_DELETE(_player1);
    SAFE_DELETE(_player2);
    SAFE_DELETE(_player3);
    SAFE_DELETE(_player4);
    SAFE_DELETE(_team1);
    SAFE_DELETE(_team2);
    return Scene::deinitializeAI(continueOnErrors);
}

bool TenisScene::loadResources(bool continueOnErrors){
    //Create our ball
    ResourceDescriptor ballDescriptor("Tenis Ball");
    _ball = CreateResource<Sphere3D>(ballDescriptor);
    _ballSGN = addGeometry(_ball,"TenisBallSGN");
    _ball->setResolution(16);
    _ball->setRadius(0.3f);
    _ballSGN->getComponent<PhysicsComponent>()->translate(vec3<F32>(3.0f, 0.2f ,7.0f));
    _ball->getMaterial()->setDiffuse(vec4<F32>(0.4f,0.5f,0.5f,1.0f));
    _ball->getMaterial()->setAmbient(vec4<F32>(0.5f,0.5f,0.5f,1.0f));
    _ball->getMaterial()->setShininess(0.2f);
    _ball->getMaterial()->setSpecular(vec4<F32>(0.7f,0.7f,0.7f,1.0f));
    _ballSGN->setSelectable(true);
    GUIElement* btn = _GUI->addButton("Serve", "Serve",
                                      vec2<I32>(renderState().cachedResolution().width-220,60),
                                      vec2<U32>(100,25),
                                      vec3<F32>(0.65f,0.65f,0.65f),
                                      DELEGATE_BIND(&TenisScene::startGame,this));
    btn->setTooltip("Start a new game!");

    _GUI->addText("Team1Score",vec2<I32>(renderState().cachedResolution().width - 250,
                  renderState().cachedResolution().height/1.3f),
                  Font::DIVIDE_DEFAULT,vec3<F32>(0,0.8f,0.8f), "Team 1 Score: %d",0);

    _GUI->addText("Team2Score",vec2<I32>(renderState().cachedResolution().width - 250,
                  renderState().cachedResolution().height/1.5f),
                  Font::DIVIDE_DEFAULT,vec3<F32>(0.2f,0.8f,0), "Team 2 Score: %d",0);

    _GUI->addText("Message",vec2<I32>(renderState().cachedResolution().width - 250,
                  renderState().cachedResolution().height/1.7f),
                  Font::DIVIDE_DEFAULT,vec3<F32>(0,1,0), "");

    _GUI->addText("fpsDisplay",              //Unique ID
                  vec2<I32>(60,60),          //Position
                  Font::DIVIDE_DEFAULT,      //Font
                  vec3<F32>(0.0f,0.2f, 1.0f),//Color
                  "FPS: %s",0);              //Text and arguments

    _GUI->addText("RenderBinCount",
                  vec2<I32>(60,70),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.6f,0.2f,0.2f),
                  "Number of items in Render Bin: %d",0);
    _guiTimers.push_back(0.0); //Fps
    return true;
}

bool TenisScene::onKeyUp(const OIS::KeyEvent& key){
    return Scene::onKeyUp(key);
}

bool TenisScene::mouseMoved(const OIS::MouseEvent& key){
    if(_mousePressed[OIS::MB_Right]){
        if(_previousMousePos.x - key.state.X.abs > 1 )		 state()._angleLR = -1;
        else if(_previousMousePos.x - key.state.X.abs < -1 ) state()._angleLR =  1;
        else 			                                     state()._angleLR =  0;

        if(_previousMousePos.y - key.state.Y.abs > 1 )		 state()._angleUD = -1;
        else if(_previousMousePos.y - key.state.Y.abs < -1 ) state()._angleUD =  1;
        else 			                                     state()._angleUD =  0;
    }

    return Scene::mouseMoved(key);
}

bool TenisScene::mouseButtonReleased(const OIS::MouseEvent& key,OIS::MouseButtonID button){
    bool keyState = Scene::mouseButtonReleased(key,button);
    if(!_mousePressed[OIS::MB_Right]){
        state()._angleUD = 0;
        state()._angleLR = 0;
    }
    return keyState;
}