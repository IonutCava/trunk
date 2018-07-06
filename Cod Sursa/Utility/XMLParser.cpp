#include "Headers/XMLParser.h"
#include "Headers/Guardian.h"
#include "Headers/ParamHandler.h"
#include "Rendering/common.h"
#include "Managers/SceneManager.h"
#include "Managers/TerrainManager.h"

namespace XML
{

	void loadScripts(const std::string &file)
	{
		ParamHandler &par = ParamHandler::getInstance();
		using boost::property_tree::ptree;
		ptree pt;
		read_xml(file,pt);
		string configLocation = pt.get("scriptLocation","XML") + "/" + pt.get("config","config.xml");
		string terrainLocation = pt.get("scriptLocation","XML") + "/" + pt.get("level","terrain.xml");
		loadConfig(configLocation);
		loadTerrain(terrainLocation);
	}

	void loadConfig(const std::string &file)
	{
		ParamHandler &par = ParamHandler::getInstance();
		using boost::property_tree::ptree;
		ptree pt;
		read_xml(file,pt);
		par.setParam("showPhysXErrors", pt.get("debug.showPhysXErrors",true));
		par.setParam("logFile",pt.get("debug.logFile","none"));
		par.setParam("memFile",pt.get("debug.memFile","none"));
		par.setParam("groundPos", pt.get("runtime.groundPos",-2000.0f));
		par.setParam("simSpeed",pt.get("runtime.simSpeed",1));
		//par.setParam("grassVisibility",pt.get("vegetation.grassVisibility",1000.0f));
		//par.setParam("treeVisibility",pt.get("vegetation.treeVisibility",10.0f));
		//par.setParam("windDirX",pt.get("wind.windDirX",1.0f));
		//par.setParam("windDirZ",pt.get("wind.windDirZ",1.0f));
		//par.setParam("windSpeed",pt.get("wind.windSpeed",1.0f));
		par.setParam("windowWidth",pt.get("runtime.windowWidth",1024));
		par.setParam("windowHeight",pt.get("runtime.windowHeight",768));
		TerrainManager::getInstance().setGrassVisibility(pt.get("vegetation.grassVisibility",1000.0f));
		TerrainManager::getInstance().setTreeVisibility(pt.get("vegetation.treeVisibility",10.0f));
		TerrainManager::getInstance().setWindSpeed(pt.get("wind.windSpeed",1.0f));
		TerrainManager::getInstance().setWindDirX(pt.get("wind.windDirX",1.0f));
		TerrainManager::getInstance().setWindDirZ(pt.get("wind.windDirZ",1.0f));
	}

	void loadTerrain(const std::string &file)
	{
		ParamHandler &par = ParamHandler::getInstance();
		typedef pair<string,string> item;
		using boost::property_tree::ptree;
		ptree pt;
		read_xml(file,pt);
		ptree::iterator it;
		for (it = pt.get_child("terrainList").begin(); it != pt.get_child("terrainList").end(); ++it )
		{
			SceneManager::getInstance().incNumberOfTerrains();
			string name = it->second.data(); //The actual terrain name
			string tag = it->first.data();   //The <name> tag for valid terrains or <xmlcomment> for comments
			//Check and skip commented terrain
			if(tag.find("<xmlcomment>") != string::npos) continue;
			//Load the rest of the terrain
			TerrainInfo ter;
			ter.variables.insert(item("terrainName",name));
			ter.variables.insert(item("heightmap",pt.get<string>(name + ".heightmap")));
			ter.variables.insert(item("textureMap",pt.get<string>(name + ".textures.map")));
			ter.variables.insert(item("redTexture",pt.get<string>(name + ".textures.red")));
			ter.variables.insert(item("greenTexture",pt.get<string>(name + ".textures.green")));
			ter.variables.insert(item("blueTexture",pt.get<string>(name + ".textures.blue")));
			//ter.variables.insert(item("alphaTexture",pt.get<string>(name + ".textures.alpha"))); NotUsed
			ter.variables.insert(item("normalMap",pt.get<string>(name + ".textures.normalMap")));
			ter.variables.insert(item("waterCaustics",pt.get<string>(name + ".textures.waterCaustics")));
			ter.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
			ter.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
			ter.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
			ter.scale.x = pt.get<F32>(name + ".scale");
			ter.scale.y = pt.get<F32>(name + ".heightFactor");
			ter.active = pt.get<bool>(name + ".active");
			ter.variables.insert(item("grassMap",pt.get<string>(name + ".vegetation.map")));
			ter.variables.insert(item("grassBillboard1",pt.get<string>(name + ".vegetation.grassBillboard1")));
			ter.variables.insert(item("grassBillboard2",pt.get<string>(name + ".vegetation.grassBillboard2")));
			ter.variables.insert(item("grassBillboard3",pt.get<string>(name + ".vegetation.grassBillboard3")));
			//ter.variables.insert(item("grassBillboard1",pt.get<string>(name + ".vegetation.grassBillboard1")));
			ter.grassDensity = pt.get<U32>(name + ".vegetation.<xmlattr>.grassDensity");
			ter.treeDensity = pt.get<U32>(name + ".vegetation.<xmlattr>.treeDensity");
			ter.grassScale = pt.get<F32>(name + ".vegetation.<xmlattr>.grassScale");
			ter.treeScale = pt.get<F32>(name + ".vegetation.<xmlattr>.grassScale");
			Guardian::getInstance().addTerrain(ter);
			
		}
		cout << "Number of terrains to load: " << SceneManager::getInstance().getNumberOfTerrains() << endl;
	}
}