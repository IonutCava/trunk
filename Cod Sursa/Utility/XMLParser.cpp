#include "Headers/XMLParser.h"
#include "Headers/Guardian.h"
#include "Headers/ParamHandler.h"
#include "Managers/SceneManager.h"
#include "Managers/TerrainManager.h"

#include "SceneList.h"

namespace XML
{
	ParamHandler &par = ParamHandler::getInstance();
	using boost::property_tree::ptree;
	ptree pt;

	void loadScripts(const std::string& file)
	{
		cout << "XML: Loading Scripts!" << endl;
		read_xml(file,pt);
		par.setParam("scriptLocation",pt.get("scriptLocation","XML"));
		par.setParam("assetsLocation",pt.get("assets","Assets"));
		par.setParam("scenesLocation",pt.get("scenes","Scenes"));
		par.setParam("serverAdress",pt.get("server","127.0.0.1"));

		loadConfig(par.getParam<string>("scriptLocation") + "/" + pt.get("config","config.xml"));

		read_xml(par.getParam<string>("scriptLocation") + "/" +
			     par.getParam<string>("scenesLocation") + "/Scenes.xml",pt);

		loadScene(pt.get("MainScene","MainScene")); 
	}

	void loadConfig(const std::string& file)
	{
		pt.clear();
		cout << "XML: Loading Configuration settings file: [ " << file << " ] " << endl;
		read_xml(file,pt);
		par.setParam("showPhysXErrors", pt.get("debug.showPhysXErrors",true));
		par.setParam("logFile",pt.get("debug.logFile","none"));
		par.setParam("memFile",pt.get("debug.memFile","none"));
		par.setParam("groundPos", pt.get("runtime.groundPos",-2000.0f));
		par.setParam("simSpeed",pt.get("runtime.simSpeed",1));
		par.setParam("windowWidth",pt.get("runtime.windowWidth",1024));
		par.setParam("windowHeight",pt.get("runtime.windowHeight",768));
	}

	void loadScene(const std::string& sceneName)
	{
		pt.clear();
		cout << "XML: Loading scene [ " << sceneName << " ] " << endl;
		read_xml(par.getParam<string>("scriptLocation") + "/" +
                 par.getParam<string>("scenesLocation") + "/" +
				 sceneName + ".xml", pt);

		Scene* scene = SceneManager::getInstance().findScene(sceneName);

		if(!scene)
		{
			cout << "XML: Trying to load unsupported scene! Defaulting to default scene" << endl;
			scene = new MainScene();
		}

		SceneManager::getInstance().setActiveScene(scene);

		TerrainManager* terMgr = SceneManager::getInstance().getTerrainManager();
		terMgr->getGrassVisibility() = pt.get("vegetation.grassVisibility",1000.0f);
		terMgr->getTreeVisibility()  = pt.get("vegetation.treeVisibility",100.0f);
		terMgr->getGeneralVisibility()  = pt.get("options.visibility",100.0f);

		terMgr->getWindDirX()  = pt.get("wind.windDirX",1.0f);
		terMgr->getWindDirZ()  = pt.get("wind.windDirZ",1.0f);
		terMgr->getWindSpeed() = pt.get("wind.windSpeed",1.0f);

		loadTerrain(par.getParam<string>("scriptLocation") + "/" +
					par.getParam<string>("scenesLocation") + "/" +
					sceneName + "/" + pt.get("terrain","terrain.xml"));

		loadGeometry(par.getParam<string>("scriptLocation") + "/" +
					 par.getParam<string>("scenesLocation") + "/" +
					 sceneName + "/" + pt.get("assets","assets.xml"));
	}



	void loadTerrain(const std::string &file)
	{
		pt.clear();
		cout << "XML: Loading terrain: [ " << file << " ] " << endl;
		read_xml(file,pt);
		ptree::iterator it;
		typedef pair<string,string> item;
		string assetLocation = ParamHandler::getInstance().getParam<string>("assetsLocation") + "/"; 
		for (it = pt.get_child("terrainList").begin(); it != pt.get_child("terrainList").end(); ++it )
		{
			string name = it->second.data(); //The actual terrain name
			string tag = it->first.data();   //The <name> tag for valid terrains or <xmlcomment> for comments
			//Check and skip commented terrain
			if(tag.find("<xmlcomment>") != string::npos) continue;
			//Load the rest of the terrain
			TerrainInfo ter;
			ter.variables.insert(item("terrainName",name));
			ter.variables.insert(item("heightmap",assetLocation + pt.get<string>(name + ".heightmap")));
			ter.variables.insert(item("textureMap",assetLocation + pt.get<string>(name + ".textures.map")));
			ter.variables.insert(item("redTexture",assetLocation + pt.get<string>(name + ".textures.red")));
			ter.variables.insert(item("greenTexture",assetLocation + pt.get<string>(name + ".textures.green")));
			ter.variables.insert(item("blueTexture",assetLocation + pt.get<string>(name + ".textures.blue")));
			//ter.variables.insert(item("alphaTexture",assetLocation + pt.get<string>(name + ".textures.alpha"))); NotUsed
			ter.variables.insert(item("normalMap",assetLocation + pt.get<string>(name + ".textures.normalMap")));
			ter.variables.insert(item("waterCaustics",assetLocation + pt.get<string>(name + ".textures.waterCaustics")));
			ter.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
			ter.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
			ter.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
			ter.scale.x = pt.get<F32>(name + ".scale");
			ter.scale.y = pt.get<F32>(name + ".heightFactor");
			ter.active = pt.get<bool>(name + ".active");
			ter.variables.insert(item("grassMap",assetLocation + pt.get<string>(name + ".vegetation.map")));
			ter.variables.insert(item("grassBillboard1",assetLocation + pt.get<string>(name + ".vegetation.grassBillboard1")));
			ter.variables.insert(item("grassBillboard2",assetLocation + pt.get<string>(name + ".vegetation.grassBillboard2")));
			ter.variables.insert(item("grassBillboard3",assetLocation + pt.get<string>(name + ".vegetation.grassBillboard3")));
			//ter.variables.insert(item("grassBillboard1",pt.get<string>(name + ".vegetation.grassBillboard1")));
			ter.grassDensity = pt.get<U32>(name + ".vegetation.<xmlattr>.grassDensity");
			ter.treeDensity = pt.get<U32>(name + ".vegetation.<xmlattr>.treeDensity");
			ter.grassScale = pt.get<F32>(name + ".vegetation.<xmlattr>.grassScale");
			ter.treeScale = pt.get<F32>(name + ".vegetation.<xmlattr>.treeScale");
			SceneManager::getInstance().addTerrain(ter);
			
		}
		cout << "XML: Number of terrains to load: " << SceneManager::getInstance().getNumberOfTerrains() << endl;
	}

	void loadGeometry(const std::string &file)
	{
		pt.clear();
		cout << "XML: Loading Geometry: [ " << file << " ] " << endl;
		read_xml(file,pt);
		ptree::iterator it;
		string assetLocation = ParamHandler::getInstance().getParam<string>("assetsLocation")+"/";
		for (it = pt.get_child("geometry").begin(); it != pt.get_child("geometry").end(); ++it )
		{
			string name = it->second.data();
			string format = it->first.data();
			if(format.find("<xmlcomment>") != string::npos) continue;
			FileData model;
			model.ModelName  = assetLocation + pt.get<string>(name + ".model");
			model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
			model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
			model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
			model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x");
			model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y");
			model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z");
			model.scale.x    = pt.get<F32>(name + ".scale.<xmlattr>.x"); 
			model.scale.y    = pt.get<F32>(name + ".scale.<xmlattr>.y"); 
			model.scale.z    = pt.get<F32>(name + ".scale.<xmlattr>.z"); 
			model.type = MESH;
			SceneManager::getInstance().addModel(model);
		}
		for (it = pt.get_child("vegetation").begin(); it != pt.get_child("vegetation").end(); ++it )
		{
			string name = it->second.data();
			string format = it->first.data();
			if(format.find("<xmlcomment>") != string::npos) continue;
			FileData model;
			model.ModelName  = assetLocation + pt.get<string>(name + ".model");
			model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
			model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
			model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
			model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x");
			model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y");
			model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z");
			model.scale.x    = pt.get<F32>(name + ".scale.<xmlattr>.x"); 
			model.scale.y    = pt.get<F32>(name + ".scale.<xmlattr>.y"); 
			model.scale.z    = pt.get<F32>(name + ".scale.<xmlattr>.z"); 
			model.type = VEGETATION;
			SceneManager::getInstance().addModel(model);
		}
	}
}