#include "Headers/Guardian.h"
#include "Managers/Headers/SceneManager.h"
#include "Headers/XMLParser.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "PhysX/Headers/PhysX.h"

using namespace std;

void Guardian::LoadApplication(const string& entryPoint){
	Application& app = Application::getInstance();
	Framerate::getInstance().Init(60);
	Console::getInstance().printCopyrightNotice();
	Console::getInstance().printfn("Starting the application!");
	//Initialize application window and hardware devices
	XML::loadScripts(entryPoint); //ToDo: This should be moved in each scene constructor! - Ionut Cava
	app.Initialize();
	LoadSettings(); //ToDo: This should be moved up so that it is the first instruction Guardian executes! - Ionut Cava
	Console::getInstance().printfn("Initializing the rendering engine");
	SceneManager::getInstance().load(string(""));
	Console::getInstance().printfn("Initial data loaded ... ");
	Console::getInstance().printfn("Entering main rendering loop ...");
	GFXDevice::getInstance().initDevice();
	
	_closing = false;
}

void Guardian::ReloadSettings(){
	LoadSettings();
}

void Guardian::ReloadEngine(){
/*
*/
}

void Guardian::TerminateApplication(){

	Console::getInstance().printfn("Closing application!");
	PostFX::getInstance().DestroyInstance();
	PhysX::getInstance().exitNx();
	SceneManager::getInstance().DestroyInstance();
	ResourceManager::getInstance().DestroyInstance();
	Console::getInstance().printfn("Closing hardware interface(GFX,SFX,PhysX, input,network) engine ...");
	GFXDevice::getInstance().DestroyInstance();
	GFXDevice::getInstance().closeRenderingApi();
	SFXDevice::getInstance().closeAudioApi();
	SFXDevice::getInstance().DestroyInstance();
	Console::getInstance().printfn("Application shutdown complete!");
}

void Guardian::LoadSettings()
{
	ParamHandler &par = ParamHandler::getInstance();
    
	string mem = par.getParam<string>("memFile");
	string log = par.getParam<string>("logFile");
	XML::loadMaterialXML(par.getParam<string>("scriptLocation")+"/defaultMaterial");
	if(mem.compare("none") != 0) myfile.open(mem.c_str());
	else myfile.open("mem.log");

}
