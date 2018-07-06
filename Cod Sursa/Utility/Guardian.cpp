#include "Headers/Guardian.h"
#include "Importer/objImporter.h"
#include "Importer/3DS/3ds.h"
#include "GUI/GLUIManager.h"
#include "Managers/TextureManager.h"
#include "Managers/SceneManager.h"
#include "PhysX/PhysX.h"
#include "Headers/ParamHandler.h"
#include "Headers/XMLParser.h"
#include "Scenes/MainScene.h"
#include "Scenes/CubeScene.h"

void Guardian::LoadApplication(string entryPoint)
{
	
	nModelIndex = 0;
	SceneManager::getInstance().setActiveScene(new MainScene("MainScene","mainScene.xml"));
	//SceneManager::getInstance().setActiveScene(new CubeScene("CubeScene","cubeScene.xml"));
	cout << "Starting the application!" << endl;
	XML::loadScripts(entryPoint);
	LoadSettings();
	ParamHandler &par = ParamHandler::getInstance();
	Engine& engine = Engine::getInstance();
	cout << "Initializing the rendering engine" << endl;
	engine.Initialize(par.getParam<U32>("windowWidth"),par.getParam<U32>("windowHeight"));
	cout << "Initializing the PhysX engine!" << endl;
    PhysX::getInstance().setParameters(-9.81f,par.getParam<bool>("showPhysXErrors"),1.0f);
	StartPhysX();
	cout << "Initial data loaded ... " << endl;
	SceneManager::getInstance().setInitialData(ModelDataArray);
	glutSetWindow(engine.getMainWindowId());
	glutCloseFunc(TerminateApplication);
	TerrainManager::getInstance().createTerrains(TerrainInfoArray);
	ModelDataArray.clear();
	cout << "Entering main rendering loop ..." << endl;
	SceneManager::getInstance().load(string(""));
	glutMainLoop();
	
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
	Engine::getInstance().Quit();
	cout << "Closing interface engine ...\n";
	TEXMANAGER.FreeAll();
	cout << "Engine shutdown complete...\n";
	//myfile.close();
	exit(0);
}

void Guardian::LoadSettings()
{
	string compare_line;
	ParamHandler &par = ParamHandler::getInstance();
    
	string mem = par.getParam<string>("memFile");
	string log = par.getParam<string>("logFile");

	if(mem.compare("none") != 0) myfile.open(mem.c_str());
	
	ifstream config ("config.cfg");
	assert(config);
	Engine &engine = Engine::getInstance();
	SceneManager& sceneMgr = SceneManager::getInstance();

	while(getline(config,compare_line))
	{
		if(compare_line.substr(0,2).compare("m ") == 0 || compare_line.substr(0,2).compare("v ") == 0)
		{
			sceneMgr.incNumberOfObjects();
		}
   	}

	cout << "Number of models to load: " << sceneMgr.getNumberOfObjects() << endl;

	config.close();
	config.open("config.cfg");

	int i = 0;
	while(getline(config,compare_line))
	{
		
		if(strncmp("m ",compare_line.c_str(),2) == 0 || strncmp("v ",compare_line.c_str(),2) == 0)
		{
			FileData f;
			istringstream iss (compare_line.c_str()+2);
			iss >> f.ModelName;
			iss >> f.ScaleFactor;
		    iss >> f.position.x;
			iss >> f.position.y;
			iss >> f.position.z;
			iss >> f.rotation.x;
			iss >> f.rotation.y;
			iss >> f.rotation.z;
			iss >> f.NormalMap;
			f.Vegetation = (strncmp("v ",compare_line.c_str(),2) == 0) ? true : false;
			ModelDataArray.push_back(f);
			cout << "Added model info: " << f.ModelName << endl;
			i++;
			iss.clear();
		}

		
	}
	config.close();
	
}
