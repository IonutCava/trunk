#include <Hardware/Network/Headers/ASIOImpl.h>
#include "Headers/NetworkScene.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"

REGISTER_SCENE(NetworkScene);

void NetworkScene::preRender(){
    Light* light = LightManager::getInstance().getLight(0);
    vec4<F32> vSunColor = DefaultColors::WHITE().lerp(vec4<F32>(1.0f, 0.5f, 0.0f, 1.0f),
                                                      vec4<F32>(1.0f, 1.0f, 0.8f, 1.0f),
                                                      0.25f + cosf(_sunAngle.y) * 0.75f);

    light->setDirection(_sunvector);
    light->setDiffuseColor(vSunColor);
    light->setSpecularColor(vSunColor);

    getSkySGN(0)->getNode<Sky>()->setSunVector(_sunvector);
}

void NetworkScene::processInput(const U64 deltaTime){
}

void NetworkScene::processGUI(const U64 deltaTime){
    D32 FpsDisplay = getSecToMs(0.3);
    D32 TimeDisplay = getSecToMs(0.01);
    D32 ServerPing = getSecToMs(1.0);
    if (_guiTimers[0] >= FpsDisplay){
        _GUI->modifyText("fpsDisplay", "FPS: %5.2f", ApplicationTimer::getInstance().getFps());
        _guiTimers[0] = 0.0;
    }

    if (_guiTimers[1] >= TimeDisplay){
        _GUI->modifyText("timeDisplay", "Elapsed time: %5.0f", time);
        _guiTimers[1] = 0.0;
    }

    if (_taskTimers[2] >= ServerPing){
        _GUI->modifyText("statusText", (char*)_paramHandler.getParam<std::string>("asioStatus").c_str());
        _GUI->modifyText("serverMessage", (char*)_paramHandler.getParam<std::string>("serverResponse").c_str());
        _guiTimers[2] = 0.0;
    }
}

void NetworkScene::checkPatches(){
    if(_modelDataArray.empty()) return;
    WorldPacket p(CMSG_GEOMETRY_LIST);
    p << std::string("NetworkScene");
    p << _modelDataArray.size();

    /*for(vectorImpl<FileData>::iterator _iter = _modelDataArray.begin(); _iter != _modelDataArray.end(); ++_iter)	{
        p << (*_iter).ItemName;
        p << (*_iter).ModelName;
        p << (*_iter).version;
    }*/
    ASIOImpl::getInstance().sendPacket(p);
}

bool NetworkScene::preLoad(){
    _GFX.changeResolution(640,384);
    return Scene::preLoad();
}

bool NetworkScene::load(const std::string& name, CameraManager* const cameraMgr, GUI* const gui){
    std::string ipAdress = _paramHandler.getParam<std::string>("serverAddress");
    std::string port = "443";
    ASIOImpl::getInstance().init(ipAdress,port);
    //Load scene resources
    bool loadState = SCENE_LOAD(name,cameraMgr,gui,true,true);

    _paramHandler.setParam("serverResponse",std::string("waiting"));
    addDefaultLight();
    addDefaultSky();
    renderState().getCamera().setEye(vec3<F32>(0,30,-30));

    return loadState;
}

void NetworkScene::test(){
    WorldPacket p(CMSG_PING);
    p << GETMSTIME();
    ASIOImpl::getInstance().sendPacket(p);
}

void NetworkScene::connect(){
    std::string ipAdress = _paramHandler.getParam<std::string>("serverAddress");
    std::string port = "443";
    _GUI->modifyText("statusText",(char*)std::string("Connecting to server ...").c_str());
    ASIOImpl::getInstance().connect(ipAdress,port);
}

void NetworkScene::disconnect()
{
    if(!ASIOImpl::getInstance().isConnected())
        _GUI->modifyText("statusText",(char*)std::string("Disconnecting to server ...").c_str());
    ASIOImpl::getInstance().disconnect();
}

bool NetworkScene::loadResources(bool continueOnErrors)
{
    _sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
    _sunvector = vec4<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
                            -cosf(_sunAngle.y),
                            -sinf(_sunAngle.x) * sinf(_sunAngle.y),
                            0.0f );

    _GUI->addText("fpsDisplay",               //Unique ID
                  vec2<I32>(60,60),           //Position
                  Font::DIVIDE_DEFAULT,       //Font
                  vec3<F32>(0.0f,0.6f, 1.0f), //Color
                  "FPS: %s",0);               //Text and arguments
    _GUI->addText("timeDisplay",
                  vec2<I32>(60,70),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.6f,0.2f,0.2f),
                  "Elapsed time: %5.0f",GETTIME());

    _GUI->addText("serverMessage", vec2<I32>(renderState().cachedResolution().width / 4.0f,
                  renderState().cachedResolution().height / 1.6f),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.5f,0.5f,0.2f),
                  "Server says: %s", "<< nothing yet >>");
    _GUI->addText("statusText",
                  vec2<I32>(renderState().cachedResolution().width / 3.0f,
                  renderState().cachedResolution().height / 1.2f),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f,0.5f,0.2f),
                  "");

    _GUI->addButton("getPing", "ping me", vec2<I32>(60 , renderState().cachedResolution().height/1.1f),
                    vec2<U32>(100,25),vec3<F32>(0.6f,0.6f,0.6f),
                    DELEGATE_BIND(&NetworkScene::test,this));
    _GUI->addButton("disconnect", "disconnect", vec2<I32>(180 , renderState().cachedResolution().height/1.1f),
                    vec2<U32>(100,25),vec3<F32>(0.5f,0.5f,0.5f),
                    DELEGATE_BIND(&NetworkScene::disconnect,this));
    _GUI->addButton("connect", "connect", vec2<I32>(300 , renderState().cachedResolution().height/1.1f),
                    vec2<U32>(100,25),vec3<F32>(0.65f,0.65f,0.65f),
                    DELEGATE_BIND(&NetworkScene::connect,this));
    _GUI->addButton("patch", "patch", vec2<I32>(420 , renderState().cachedResolution().height/1.1f),
                    vec2<U32>(100,25),vec3<F32>(0.65f,0.65f,0.65f),
                    DELEGATE_BIND(&NetworkScene::checkPatches,this));

    _guiTimers.push_back(0.0f); //Fps
    _guiTimers.push_back(0.0f); //Time
    _guiTimers.push_back(0.0f); //Server Ping
    return true;
}