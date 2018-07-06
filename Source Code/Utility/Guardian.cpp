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
	Console::getInstance().printCopyrightNotice();
	Console::getInstance().printfn("Starting the application!");
	XML::loadScripts(entryPoint); //ToDo: This should be moved in each scene constructor! - Ionut Cava
	
	LoadSettings(); //ToDo: This should be moved up so that it is the first instruction Guardian executes! - Ionut Cava
	Console::getInstance().printfn("Initializing the rendering engine");
	engine.Initialize();
	Console::getInstance().printfn("Initializing the PhysX engine!");
    PhysX::getInstance().setParameters(-9.81f,par.getParam<bool>("showPhysXErrors"),1.0f);
	StartPhysX();
	SceneManager::getInstance().load(string(""));
	Console::getInstance().printfn("Initial data loaded ... ");
	Console::getInstance().printfn("Entering main rendering loop ...");
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
		Console::getInstance().errorfn("Please install the PhysX driver in order to run physics simulations!");
	
}

void Guardian::TerminateApplication()
{
	GFXDevice::getInstance().closeRenderingApi();	
	Console::getInstance().printfn("Closing the PhysX engine ...");
	PhysX::getInstance().ExitNx();
	Console::getInstance().printfn("Deleting running scene ...");
	Scene* activeScene = SceneManager::getInstance().getActiveScene();
	activeScene->unload();
	delete activeScene;
	activeScene = NULL;
	Console::getInstance().printfn("Destroying scene manager ...");
	SceneManager::getInstance().DestroyInstance();
	Console::getInstance().printfn("Deleting resource manager ...");
	ResourceManager::getInstance().DestroyInstance();
	Console::getInstance().printfn("Closing hardware interface(GFX,SFX,input,network) engine ...");
	GFXDevice::getInstance().DestroyInstance();
	SFXDevice::getInstance().closeAudioApi();
	SFXDevice::getInstance().DestroyInstance();
	Console::getInstance().printfn("Closing interface engine ...");
	Engine::getInstance().Quit();
	Console::getInstance().printfn("Application shutdown complete!");
}

void Guardian::LoadSettings()
{
	ParamHandler &par = ParamHandler::getInstance();
    
	string mem = par.getParam<string>("memFile");
	string log = par.getParam<string>("logFile");

	if(mem.compare("none") != 0) myfile.open(mem.c_str());
	else myfile.open("mem.log");

}