#include "Headers/MainScene.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Environment/Water/Headers/Water.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

REGISTER_SCENE(MainScene);

bool MainScene::updateLights(){
    Light* light = LightManager::getInstance().getLight(0);
    _sun_cosy = cosf(_sunAngle.y);
    _sunColor = DefaultColors::WHITE().lerp(vec4<F32>(1.0f, 0.5f, 0.0f, 1.0f),
                                            vec4<F32>(1.0f, 1.0f, 0.8f, 1.0f),
                                            0.25f + _sun_cosy * 0.75f);

    light->setDirection(_sunvector);
    light->setLightProperties(LIGHT_PROPERTY_DIFFUSE,_sunColor);
    light->setLightProperties(LIGHT_PROPERTY_SPECULAR,_sunColor);
    getSkySGN(0)->getNode<Sky>()->setSunVector(_sunvector);
    FOR_EACH(Terrain* ter, _visibleTerrains){
        ter->getMaterial()->setAmbient(_sunColor);
    }
    return true;
}

void MainScene::preRender(){
}

void MainScene::postRender(){
    if(GFX_DEVICE.isCurrentRenderStage(FINAL_STAGE)){
        _water->previewReflection();
    }
    Scene::postRender();
}

void MainScene::renderEnvironment(bool waterReflection){
    const vec3<F32>& eyePos = renderState().getCamera().getEye();

    bool underwater = _water->isPointUnderWater(eyePos);

    _paramHandler.setParam("scene.camera.underwater", underwater);

    if(underwater){
        waterReflection = false;
    }

    if(waterReflection){
        renderState().getCamera().renderLookAtReflected(_water->getReflectionPlane());
    }

    FOR_EACH(Terrain* ter, _visibleTerrains){
        ter->toggleReflection(waterReflection);
    }

    SceneManager::getInstance().renderVisibleNodes();
}

void MainScene::processInput(const U64 deltaTime){
    bool update = false;
    Camera& cam = renderState().getCamera();
    if(state()._angleLR){
        cam.rotateYaw(state()._angleLR);
        update = true;
    }
    if(state()._angleUD){
        cam.rotatePitch(state()._angleUD);
        update = true;
    }

    if(state()._moveFB || state()._moveLR){
        if(state()._moveFB) cam.moveForward(state()._moveFB);
        if(state()._moveLR) cam.moveStrafe(state()._moveLR);
        update = true;
    }

    if(update){
        const vec3<F32>& eyePos = cam.getEye();
        const vec3<F32>& euler  = cam.getEuler();
        if(!_freeflyCamera){
            F32 terrainHeight = 0.0f;
            vec3<F32> eyePosition = cam.getEye();
            FOR_EACH(Terrain* ter, _visibleTerrains){
                terrainHeight = ter->getPositionFromGlobal(eyePosition.x,eyePosition.z).y;
                if(!IS_ZERO(terrainHeight)){
                    eyePosition.y = terrainHeight + 0.45f;
                    cam.setEye(eyePosition);
                    break;
                }
            }
            _GUI->modifyText("camPosition","[ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: %5.2f | Yaw: %5.2f] [TerHght: %5.2f ]",
                              eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw,
                              terrainHeight);
        }else{
            _GUI->modifyText("camPosition","[ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: %5.2f | Yaw: %5.2f]",
                              eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw);
        }
        update = false;
    }
}

void MainScene::processTasks(const U64 deltaTime){
    updateLights();
    D32 SunDisplay = getSecToMs(1.50);
    D32 FpsDisplay = getSecToMs(0.5);
    D32 TimeDisplay = getSecToMs(1.0);
    if (_taskTimers[0] >= SunDisplay){
        _sunAngle.y += 0.0005f;
        _sunvector = vec4<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
                                -cosf(_sunAngle.y),
                                -sinf(_sunAngle.x) * sinf(_sunAngle.y),
                                0.0f );
        _taskTimers[0] = 0.0;
    }

    if (_taskTimers[1] >= FpsDisplay){
        _GUI->modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f", ApplicationTimer::getInstance().getFps(), ApplicationTimer::getInstance().getFrameTime());
        _GUI->modifyText("underwater","Underwater [ %s ] | WaterLevel [%f] ]", _paramHandler.getParam<bool>("scene.camera.underwater") ? "true" : "false", state().getWaterLevel());
        _GUI->modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
        _taskTimers[1] = 0.0;
    }

    if (_taskTimers[2] >= TimeDisplay){
        _GUI->modifyText("timeDisplay", "Elapsed time: %5.0f", GETTIME());
        _taskTimers[2] = 0.0;
    }

    Scene::processTasks(deltaTime);
}

bool MainScene::load(const std::string& name, CameraManager* const cameraMgr, GUI* const gui){
    bool computeWaterHeight = false;

    //Load scene resources
    bool loadState = SCENE_LOAD(name,cameraMgr,gui,true,true);

    if(state().getWaterLevel() == RAND_MAX) computeWaterHeight = true;
    addDefaultLight();
    addDefaultSky();

    for(U8 i = 0; i < _terrainInfoArray.size(); i++){
        SceneGraphNode* terrainNode = _sceneGraph->findNode(_terrainInfoArray[i]->getVariable("terrainName"));
        if(terrainNode){ //We might have an unloaded terrain in the Array, and thus, not present in the graph
            Terrain* tempTerrain = terrainNode->getNode<Terrain>();
            if(terrainNode->isActive()){
                _visibleTerrains.push_back(tempTerrain);
                if(computeWaterHeight){
                    F32 tempMin = terrainNode->getBoundingBox().getMin().y;
                    if(state()._waterHeight > tempMin) state()._waterHeight = tempMin;
                }
            }
        }else{
            ERROR_FN(Locale::get("ERROR_MISSING_TERRAIN"), _terrainInfoArray[i]->getVariable("terrainName"));
        }
    }
    ResourceDescriptor infiniteWater("waterEntity");
    _water = CreateResource<WaterPlane>(infiniteWater);
    _water->setParams(200.0f,vec2<F32>(85.0f, 92.5f),vec2<F32>(0.4f,0.35f),0.34f);
    _waterGraphNode = _sceneGraph->getRoot()->addNode(_water);
    _waterGraphNode->useDefaultTransform(false);
    _waterGraphNode->setTransform(nullptr);
    _waterGraphNode->setUsageContext(SceneGraphNode::NODE_STATIC);
    _waterGraphNode->setNavigationContext(SceneGraphNode::NODE_IGNORE);
    ///General rendering callback
    renderCallback(DELEGATE_BIND(&MainScene::renderEnvironment, this, false));
    ///Render the scene for water reflection FBO generation
    _water->setRenderCallback(DELEGATE_BIND(&MainScene::renderEnvironment, this, true));
    return loadState;
}

bool MainScene::unload(){
    SFX_DEVICE.stopMusic();
    SFX_DEVICE.stopAllSounds();
    RemoveResource(_beep);
    return Scene::unload();
}

void MainScene::test(boost::any a, CallbackParam b){
    static bool switchAB = false;
    vec3<F32> pos;
    SceneGraphNode* boxNode = _sceneGraph->findNode("box");
    Object3D* box = nullptr;
    if(boxNode) box = boxNode->getNode<Object3D>();
    if(box) pos = boxNode->getTransform()->getPosition();

    if(!switchAB){
        if(pos.x < 300 && pos.z == 0)		   pos.x++;
        if(pos.x == 300)
        {
            if(pos.y < 800 && pos.z == 0)      pos.y++;
            if(pos.y == 800)
            {
                if(pos.z > -500)   pos.z--;
                if(pos.z == -500)  switchAB = true;
            }
        }
    } else {
        if(pos.x > -300 && pos.z ==  -500)      pos.x--;
        if(pos.x == -300)
        {
            if(pos.y > 100 && pos.z == -500)    pos.y--;
            if(pos.y == 100) {
                if(pos.z < 0)    pos.z++;
                if(pos.z == 0)   switchAB = false;
            }
        }
    }
    if(box)	boxNode->getTransform()->setPosition(pos);
}

bool MainScene::loadResources(bool continueOnErrors){

    _GUI->addText("fpsDisplay",               //Unique ID
                  vec2<I32>(60,60),           //Position
                  Font::DIVIDE_DEFAULT,       //Font
                  vec3<F32>(0.0f,0.2f, 1.0f), //Color
                  "FPS: %s",0);               //Text and arguments

    _GUI->addText("timeDisplay",
                  vec2<I32>(60,80),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.6f,0.2f,0.2f),
                  "Elapsed time: %5.0f",GETTIME());
    _GUI->addText("underwater",
                  vec2<I32>(60,115),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f,0.8f,0.2f),
                  "Underwater [ %s ] | WaterLevel [%f] ]","false", 0);
    _GUI->addText("RenderBinCount",
                  vec2<I32>(60,135),
                  Font::BATANG,
                  vec3<F32>(0.6f,0.2f,0.2f),
                  "Number of items in Render Bin: %d",0);
    _taskTimers.push_back(0.0); //Sun
    _taskTimers.push_back(0.0); //Fps
    _taskTimers.push_back(0.0); //Time

    _sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
    _sunvector = vec4<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
                            -cosf(_sunAngle.y),
                            -sinf(_sunAngle.x) * sinf(_sunAngle.y),
                            0.0f );

    Kernel* kernel = Application::getInstance().getKernel();
    Task_ptr boxMove(New Task(kernel->getThreadPool(),30,true,false,DELEGATE_BIND(&MainScene::test,this,std::string("test"),TYPE_STRING)));
    addTask(boxMove);
    ResourceDescriptor backgroundMusic("background music");
    backgroundMusic.setResourceLocation(_paramHandler.getParam<std::string>("assetsLocation")+"/music/background_music.ogg");
    backgroundMusic.setFlag(true);

    ResourceDescriptor beepSound("beep sound");
    beepSound.setResourceLocation(_paramHandler.getParam<std::string>("assetsLocation")+"/sounds/beep.wav");
    beepSound.setFlag(false);
    state()._backgroundMusic["generalTheme"] = CreateResource<AudioDescriptor>(backgroundMusic);
    _beep = CreateResource<AudioDescriptor>(beepSound);

    const vec3<F32>& eyePos = renderState().getCamera().getEye();
    const vec3<F32>& euler = renderState().getCamera().getEuler();
    _GUI->addText("camPosition",  vec2<I32>(60,100),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f,0.8f,0.2f),
                  "Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | Yaw: %5.2f]",
                  eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw);

    return true;
}

bool MainScene::onKeyDown(const OIS::KeyEvent& key){
    bool keyState = Scene::onKeyDown(key);
    switch(key.key)	{
        default: break;
        case OIS::KC_W: state()._moveFB =  1; break;
        case OIS::KC_A:	state()._moveLR = -1; break;
        case OIS::KC_S:	state()._moveFB = -1; break;
        case OIS::KC_D:	state()._moveLR =  1; break;
    }
    return keyState;
}

bool _playMusic = false;
bool MainScene::onKeyUp(const OIS::KeyEvent& key){
    bool keyState = Scene::onKeyUp(key);
    switch(key.key)	{
        default: break;
        case OIS::KC_W:
        case OIS::KC_S:	state()._moveFB = 0; break;
        case OIS::KC_A:
        case OIS::KC_D: state()._moveLR = 0; break;
        case OIS::KC_X:	SFX_DEVICE.playSound(_beep); break;
        case OIS::KC_M:{
            _playMusic = !_playMusic;
            if(_playMusic){
                SFX_DEVICE.playMusic(state()._backgroundMusic["generalTheme"]);
            }else{
                SFX_DEVICE.stopMusic();
            }
            }break;
        case OIS::KC_R:{
            _water->togglePreviewReflection();
            }break;
        case OIS::KC_F:{
            _freeflyCamera = !_freeflyCamera;
            _FBSpeedFactor = _freeflyCamera ? 1.0f : 20.0f;
            _LRSpeedFactor = _freeflyCamera ? 2.0f : 5.0f;
            }break;
        case OIS::KC_F1:
            _sceneGraph->print();
            break;
        case OIS::KC_T:
            FOR_EACH(Terrain* ter, _visibleTerrains){
                ter->toggleBoundingBoxes();
            }
            break;
    }
    return keyState;
}

bool MainScene::onMouseMove(const OIS::MouseEvent& key){
    if(_mousePressed[OIS::MB_Right]){
        if(_previousMousePos.x - key.state.X.abs > 1 )		 state()._angleLR = -1;
        else if(_previousMousePos.x - key.state.X.abs < -1 ) state()._angleLR =  1;
        else			                                     state()._angleLR =  0;

        if(_previousMousePos.y - key.state.Y.abs > 1 )		 state()._angleUD = -1;
        else if(_previousMousePos.y - key.state.Y.abs < -1 ) state()._angleUD =  1;
        else 			                                     state()._angleUD =  0;
    }

    return Scene::onMouseMove(key);
}

bool MainScene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
    bool keyState = Scene::onMouseClickUp(key,button);
    if(!_mousePressed[OIS::MB_Right]){
        state()._angleUD = 0;
        state()._angleLR = 0;
    }
    return keyState;
}