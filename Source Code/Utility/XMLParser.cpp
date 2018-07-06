#include "Headers/XMLParser.h"
#include "Headers/Guardian.h"
#include "Headers/ParamHandler.h"
#include "Managers/SceneManager.h"
#include "Managers/ResourceManager.h"
#include "Rendering/common.h"
#include "SceneList.h"
using namespace std;

namespace XML
{
	ParamHandler &par = ParamHandler::getInstance();
	using boost::property_tree::ptree;
	ptree pt;

	void loadScripts(const string& file)
	{
		Console::getInstance().printfn("XML: Loading Scripts!");
		read_xml(file,pt);
		par.setParam("scriptLocation",pt.get("scriptLocation","XML"));
		par.setParam("assetsLocation",pt.get("assets","Assets"));
		par.setParam("scenesLocation",pt.get("scenes","Scenes"));
		par.setParam("serverAddress",pt.get("server","127.0.0.1"));
		loadConfig(par.getParam<string>("scriptLocation") + "/" + pt.get("config","config.xml"));

		read_xml(par.getParam<string>("scriptLocation") + "/" +
			     par.getParam<string>("scenesLocation") + "/Scenes.xml",pt);
		loadScene(pt.get("MainScene","MainScene")); 
	}

	void loadConfig(const string& file)
	{
		pt.clear();
		Console::getInstance().printf("XML: Loading Configuration settings file: [ %s ]\n", file.c_str());
		read_xml(file,pt);
		par.setParam("showPhysXErrors", pt.get("debug.showPhysXErrors",true));
		par.setParam("logFile",pt.get("debug.logFile","none"));
		par.setParam("memFile",pt.get("debug.memFile","none"));
		par.setParam("groundPos", pt.get("runtime.groundPos",-2000.0f));
		par.setParam("simSpeed",pt.get("runtime.simSpeed",1));
		par.setParam("windowWidth",pt.get("runtime.windowWidth",1024));
		par.setParam("windowHeight",pt.get("runtime.windowHeight",768));
		Engine::getInstance().setWindowHeight(pt.get("runtime.windowHeight",768));
		Engine::getInstance().setWindowWidth(pt.get("runtime.windowWidth",1024));
		bool postProcessing = pt.get("rendering.enablePostFX",false);
		par.setParam("enablePostFX",postProcessing);
		if(postProcessing){
			bool enable3D = pt.get("rendering.enable3D",false);
			par.setParam("enable3D",enable3D);
			if(enable3D){
				par.setParam("anaglyphOffset",pt.get("rendering.anaglyphOffset",0.16f));
			}
			par.setParam("enableNoise",pt.get("rendering.enableNoise",false));
			par.setParam("enableDepthOfField",pt.get("rendering.enableDepthOfField",false));
			par.setParam("enableBloom",pt.get("rendering.enableBloom",false));
			par.setParam("enableBlur",pt.get("rendering.enableBlur",false));
		}
			

	}

	void loadScene(const string& sceneName)
	{
		pt.clear();
		Console::getInstance().printf("XML: Loading scene [ %s ]\n", sceneName.c_str());
		read_xml(par.getParam<string>("scriptLocation") + "/" +
                 par.getParam<string>("scenesLocation") + "/" +
				 sceneName + ".xml", pt);

		Scene* scene = SceneManager::getInstance().findScene(sceneName);

		if(!scene)
		{
			Console::getInstance().errorf("XML: Trying to load unsupported scene! Defaulting to default scene\n");
			scene = new MainScene();
		}

		SceneManager::getInstance().setActiveScene(scene);

		SceneManager::getInstance().getActiveScene()->getGrassVisibility() = pt.get("vegetation.grassVisibility",1000.0f);
		SceneManager::getInstance().getActiveScene()->getTreeVisibility()  = pt.get("vegetation.treeVisibility",1000.0f);
		SceneManager::getInstance().getActiveScene()->getGeneralVisibility()  = pt.get("options.visibility",1000.0f);

		SceneManager::getInstance().getActiveScene()->getWindDirX()  = pt.get("wind.windDirX",1.0f);
		SceneManager::getInstance().getActiveScene()->getWindDirZ()  = pt.get("wind.windDirZ",1.0f);
		SceneManager::getInstance().getActiveScene()->getWindSpeed() = pt.get("wind.windSpeed",1.0f);

		SceneManager::getInstance().getActiveScene()->getWaterLevel() = pt.get("water.waterLevel",RAND_MAX);
		SceneManager::getInstance().getActiveScene()->getWaterDepth() = pt.get("water.waterDepth",-75);
		loadTerrain(par.getParam<string>("scriptLocation") + "/" +
					par.getParam<string>("scenesLocation") + "/" +
					sceneName + "/" + pt.get("terrain","terrain.xml"));

		loadGeometry(par.getParam<string>("scriptLocation") + "/" +
					 par.getParam<string>("scenesLocation") + "/" +
					 sceneName + "/" + pt.get("assets","assets.xml"));
	}



	void loadTerrain(const string &file)
	{
		pt.clear();
		Console::getInstance().printf("XML: Loading terrain: [ %s ]\n",file.c_str());
		read_xml(file,pt);
		ptree::iterator it;
		string assetLocation = ParamHandler::getInstance().getParam<string>("assetsLocation") + "/"; 
		for (it = pt.get_child("terrainList").begin(); it != pt.get_child("terrainList").end(); it++ )
		{
			string name = it->second.data(); //The actual terrain name
			string tag = it->first.data();   //The <name> tag for valid terrains or <xmlcomment> for comments
			//Check and skip commented terrain
			if(tag.find("<xmlcomment>") != string::npos) continue;
			//Load the rest of the terrain
			TerrainDescriptor* ter = ResourceManager::getInstance().LoadResource<TerrainDescriptor>(name+"_descriptor");
			ter->addVariable("terrainName",name);
			ter->addVariable("heightmap",assetLocation + pt.get<string>(name + ".heightmap"));
			ter->addVariable("textureMap",assetLocation + pt.get<string>(name + ".textures.map"));
			ter->addVariable("redTexture",assetLocation + pt.get<string>(name + ".textures.red"));
			ter->addVariable("greenTexture",assetLocation + pt.get<string>(name + ".textures.green"));
			ter->addVariable("blueTexture",assetLocation + pt.get<string>(name + ".textures.blue"));
			ter->addVariable("alphaTexture",assetLocation + pt.get<string>(name + ".textures.alpha","none"));
			ter->addVariable("normalMap",assetLocation + pt.get<string>(name + ".textures.normalMap"));
			ter->addVariable("waterCaustics",assetLocation + pt.get<string>(name + ".textures.waterCaustics"));
			ter->addVariable("grassMap",assetLocation + pt.get<string>(name + ".vegetation.map"));
			ter->addVariable("grassBillboard1",assetLocation + pt.get<string>(name + ".vegetation.grassBillboard1"));
			ter->addVariable("grassBillboard2",assetLocation + pt.get<string>(name + ".vegetation.grassBillboard2"));
			ter->addVariable("grassBillboard3",assetLocation + pt.get<string>(name + ".vegetation.grassBillboard3"));
			//ter->addVariable("grassBillboard1",pt.get<string>(name + ".vegetation.grassBillboard1"));
			ter->setGrassDensity(pt.get<U32>(name + ".vegetation.<xmlattr>.grassDensity"));
			ter->setTreeDensity(pt.get<U16>(name + ".vegetation.<xmlattr>.treeDensity"));
			ter->setGrassScale(pt.get<F32>(name + ".vegetation.<xmlattr>.grassScale"));
			ter->setTreeScale(pt.get<F32>(name + ".vegetation.<xmlattr>.treeScale"));

			ter->setPosition(vec3(pt.get<F32>(name + ".position.<xmlattr>.x"),
								  pt.get<F32>(name + ".position.<xmlattr>.y"),
								  pt.get<F32>(name + ".position.<xmlattr>.z")));
			ter->setScale(vec2(pt.get<F32>(name + ".scale"), //width / length
							   pt.get<F32>(name + ".heightFactor"))); //height
							   

			ter->setActive(pt.get<bool>(name + ".active"));
			
			SceneManager::getInstance().addTerrain(ter);
			
		}
		Console::getInstance().printf("XML: Number of terrains to load: %d\n",SceneManager::getInstance().getNumberOfTerrains());
	}

	void loadGeometry(const string &file)
	{
		pt.clear();
		Console::getInstance().printf("XML: Loading Geometry: [ %s ]\n",file.c_str());
		read_xml(file,pt);
		ptree::iterator it;
		string assetLocation = ParamHandler::getInstance().getParam<string>("assetsLocation")+"/";
		if(boost::optional<ptree &> geometry = pt.get_child_optional("geometry"))
		for (it = pt.get_child("geometry").begin(); it != pt.get_child("geometry").end(); ++it )
		{
			string name = it->second.data();
			string format = it->first.data();
			if(format.find("<xmlcomment>") != string::npos) continue;
			FileData model;
			model.ItemName = name;
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
			model.type = GEOMETRY;
			model.version = pt.get<F32>(name + ".version");
			SceneManager::getInstance().addModel(model);
		}
		if(boost::optional<ptree &> vegetation = pt.get_child_optional("vegetation"))
		for (it = pt.get_child("vegetation").begin(); it != pt.get_child("vegetation").end(); ++it )
		{
			string name = it->second.data();
			string format = it->first.data();
			if(format.find("<xmlcomment>") != string::npos) continue;
			FileData model;
			model.ItemName = name;
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
			model.version = pt.get<F32>(name + ".version");
			SceneManager::getInstance().addModel(model);
		}
		if(boost::optional<ptree &> primitives = pt.get_child_optional("primitives"))
		for (it = pt.get_child("primitives").begin(); it != pt.get_child("primitives").end(); ++it )
		{
			string name = it->second.data();
			string format = it->first.data();
			if(format.find("<xmlcomment>") != string::npos) continue;

			FileData model;
			model.ItemName = name;
			model.ModelName = pt.get<string>(name + ".model");
			model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
			model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
			model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
			model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x");
			model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y");
			model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z");
			model.scale.x    = pt.get<F32>(name + ".scale.<xmlattr>.x"); 
			model.scale.y    = pt.get<F32>(name + ".scale.<xmlattr>.y"); 
			model.scale.z    = pt.get<F32>(name + ".scale.<xmlattr>.z"); 
			model.color.r    = pt.get<F32>(name + ".color.<xmlattr>.r"); 
			model.color.g    = pt.get<F32>(name + ".color.<xmlattr>.g"); 
			model.color.b    = pt.get<F32>(name + ".color.<xmlattr>.b");
			/*The data variable stores a float variable (not void*) that can represent anything you want*/
			/*For Text3D, it's the line width and for Box3D it's the edge length*/
			if(model.ModelName.compare("Text3D") == 0)
			{
				model.data = pt.get<F32>(name + ".lineWidth");
				model.data2 = pt.get<string>(name + ".text");
			}
			else if(model.ModelName.compare("Box3D") == 0)
				model.data = pt.get<F32>(name + ".size");

			else if(model.ModelName.compare("Sphere3D") == 0)
				model.data = pt.get<F32>(name + ".radius");
			else
				model.data = 0;
			
			model.type = PRIMITIVE;
			model.version = pt.get<F32>(name + ".version");
			SceneManager::getInstance().addModel(model);
		}
	}
}