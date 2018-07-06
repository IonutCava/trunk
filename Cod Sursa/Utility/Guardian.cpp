#include "Headers/Guardian.h"
#include "Managers/SceneManager.h"
#include "Managers/ResourceManager.h"
#include "Headers/ParamHandler.h"
#include "Headers/XMLParser.h"
#include "Rendering/common.h"
using namespace std;

void Guardian::LoadApplication(const string& entryPoint)
{
	Engine& engine = Engine::getInstance();
	ParamHandler& par = ParamHandler::getInstance();
	Framerate::getInstance().Init(60);
	Con::getInstance().printfn("Starting the application!");
	XML::loadScripts(entryPoint); //ToDo: This should be moved in each scene constructor! - Ionut Cava
	
	LoadSettings(); //ToDo: This should be moved up so that it is the first instruction Guardian executes! - Ionut Cava
	Con::getInstance().printfn("Initializing the rendering engine");
	engine.Initialize();
	Con::getInstance().printfn("Initializing the PhysX engine!");
    PhysX::getInstance().setParameters(-9.81f,par.getParam<bool>("showPhysXErrors"),1.0f);
	StartPhysX();
	SceneManager::getInstance().load(string(""));
	Con::getInstance().printfn("Initial data loaded ... ");
	Con::getInstance().printfn("Entering main rendering loop ...");
	GFXDevice::getInstance().initDevice();

	
}

void Guardian::ReloadSettings()
{
	ParamHandler &par = ParamHandler::getInstance();
	LoadSettings();
	PhysX::getInstance().setParameters(-9.81f,par.getParam<bool>("showPhysXErrors"),1.0f);
	RestartPhysX();
}

void Guardian::ReloadEngine()
{
/*
*/
}

void Guardian::RestartPhysX()
{
	PhysX::getInstance().ExitNx();
	StartPhysX();
}

void Guardian::StartPhysX()
{
	PhysX &pxWorld = PhysX::getInstance();
	ParamHandler &par = ParamHandler::getInstance();
	if(pxWorld.InitNx())
	{
		pxWorld.GetPhysicsResults();
		pxWorld.StartPhysics(); // initializam sistemul de fizica
		pxWorld.setGroundPos(par.getParam<F32>("groundPos"));
        pxWorld.setSimSpeed(par.getParam<F32>("simSpeed"));
		pxWorld.setGroundPlane();
	}
	else
		Con::getInstance().errorfn("Please install the PhysX driver in order to run physics simulations!");
	
}

void Guardian::TerminateApplication()
{
	Con::getInstance().printfn("Closing the PhysX engine ...");
	PhysX::getInstance().ExitNx();
	Con::getInstance().printfn("Destroying Terrain ...");
	TerrainManager *terMgr = SceneManager::getInstance().getTerrainManager();
	delete terMgr;
	terMgr = NULL;
	Con::getInstance().printfn("Deleting scene objects ...");
	delete SceneManager::getInstance().getActiveScene();
	SceneManager::getInstance().Destroy();
	SceneManager::getInstance().DestroyInstance();
	//clear everything left in the resourceManager
	ResourceManager::getInstance().Destroy();
	ResourceManager::getInstance().DestroyInstance();
	Con::getInstance().printfn("Closing hardware interface(GFX,SFX,input,network) engine ...");
	GFXDevice::getInstance().closeRenderingApi();
	GFXDevice::getInstance().DestroyInstance();
	SFXDevice::getInstance().closeAudioApi();
	SFXDevice::getInstance().DestroyInstance();
	Con::getInstance().printfn("Closing interface engine ...");
	Engine::getInstance().Quit();
}

void Guardian::LoadSettings()
{
	ParamHandler &par = ParamHandler::getInstance();
    
	string mem = par.getParam<string>("memFile");
	string log = par.getParam<string>("logFile");

	if(mem.compare("none") != 0) myfile.open(mem.c_str());
	else myfile.open("mem.log");

}
