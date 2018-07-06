#include "Headers/Guardian.h"
#include "Importer/objImporter.h"
#include "Importer/3DS/3ds.h"
#include "GUI/GLUIManager.h"
#include "Managers/TerrainManager.h"
#include "Managers/TextureManager.h"
#include "Managers/SceneManager.h"
#include "PhysX/PhysX.h"
#include "Headers/ParamHandler.h"
#include "Headers/XMLParser.h"
#include "Scenes/MainScene.h"
#include "Scenes/CubeScene.h"
#include "Rendering/common.h"

void Guardian::LoadApplication(string entryPoint)
{
	Engine& engine = Engine::getInstance();
	ParamHandler& par = ParamHandler::getInstance();

	cout << "Starting the application!" << endl;
	XML::loadScripts(entryPoint); //ToDo: This should be moved in each scene constructor! - Ionut Cava
	
	LoadSettings(); //ToDo: This should be moved up so that it is the first instruction Guardian executes! - Ionut Cava
	cout << "Initializing the rendering engine" << endl;
	engine.Initialize(par.getParam<U32>("windowWidth"),par.getParam<U32>("windowHeight"));
	cout << "Initializing the PhysX engine!" << endl;
    PhysX::getInstance().setParameters(-9.81f,par.getParam<bool>("showPhysXErrors"),1.0f);
	StartPhysX();
	SceneManager::getInstance().load(string(""));
	cout << "Initial data loaded ... " << endl;
	cout << "Entering main rendering loop ..." << endl;
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
		cout << "Please install the PhysX driver in order to run physics simulations!\n";
	
}

void Guardian::TerminateApplication()
{
	cout << "Closing the PhysX engine ...\n";
	PhysX::getInstance().ExitNx();
	cout << "Deleting imported objects ...\n";
	//CleanUpOBJ();
	cout << "Closing interface engine ...\n";
	Engine::getInstance().Quit();
	cout << "Engine shutdown complete...\n";
	//myfile.close();
	exit(0);
}

void Guardian::LoadSettings()
{
	ParamHandler &par = ParamHandler::getInstance();
    
	string mem = par.getParam<string>("memFile");
	string log = par.getParam<string>("logFile");

	if(mem.compare("none") != 0) myfile.open(mem.c_str());

}
