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
	std::string activeScene("MainScene");
	void loadScripts(const string& file){
		Console::getInstance().printfn("XML: Loading Scripts!");
		read_xml(file,pt);
		par.setParam("scriptLocation",pt.get("scriptLocation","XML"));
		par.setParam("assetsLocation",pt.get("assets","Assets"));
		par.setParam("scenesLocation",pt.get("scenes","Scenes"));
		par.setParam("serverAddress",pt.get("server","127.0.0.1"));
		loadConfig(par.getParam<string>("scriptLocation") + "/" + pt.get("config","config.xml"));

		read_xml(par.getParam<string>("scriptLocation") + "/" +
			     par.getParam<string>("scenesLocation") + "/Scenes.xml",pt);
		activeScene = pt.get("MainScene",activeScene);
		loadScene(activeScene); 
		loadMaterialXML(par.getParam<string>("scriptLocation")+"/defaultMaterial");
	}

	void loadConfig(const string& file){
		pt.clear();
		Console::getInstance().printf("XML: Loading Configuration settings file: [ %s ]\n", file.c_str());
		read_xml(file,pt);
		par.setParam("showPhysXErrors", pt.get("debug.showPhysXErrors",true));
		par.setParam("logFile",pt.get("debug.logFile","none"));
		par.setParam("memFile",pt.get("debug.memFile","none"));
		par.setParam("groundPos", pt.get("runtime.groundPos",-2000.0f));
		par.setParam("simSpeed",pt.get("runtime.simSpeed",1));
		I32 winWidth = pt.get("runtime.windowWidth",1024);
		I32 winHeight = pt.get("runtime.windowHeight",768);
		par.setParam("zNear",(F32)pt.get("runtime.zNear",0.1f));
		par.setParam("zFar",(F32)pt.get("runtime.zFar",1200.0f));
		par.setParam("verticalFOV",(F32)pt.get("runtime.verticalFOV",60));
		par.setParam("aspectRatio",1.0f * winWidth / winHeight);
		par.setParam("windowWidth",winWidth);
		par.setParam("windowHeight",winHeight);
		Engine::getInstance().setWindowHeight(winHeight);
		Engine::getInstance().setWindowWidth(winWidth);
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
		}
	}

	void loadScene(const string& sceneName)
	{
		pt.clear();
		Console::getInstance().printf("XML: Loading scene [ %s ]\n", sceneName.c_str());
		read_xml(par.getParam<string>("scriptLocation") + "/" +
                 par.getParam<string>("scenesLocation") + "/" +
				 sceneName + ".xml", pt);
		par.setParam("currentScene",sceneName);
		Scene* scene = SceneManager::getInstance().findScene(sceneName);

		if(!scene)
		{
			Console::getInstance().errorf("XML: Trying to load unsupported scene! Defaulting to default scene\n");
			scene = New MainScene();
		}

		SceneManager::getInstance().setActiveScene(scene);
		scene->setName(sceneName);

		scene->getGrassVisibility() = pt.get("vegetation.grassVisibility",1000.0f);
		scene->getTreeVisibility()  = pt.get("vegetation.treeVisibility",1000.0f);
		scene->getGeneralVisibility()  = pt.get("options.visibility",1000.0f);

		scene->getWindDirX()  = pt.get("wind.windDirX",1.0f);
		scene->getWindDirZ()  = pt.get("wind.windDirZ",1.0f);
		scene->getWindSpeed() = pt.get("wind.windSpeed",1.0f);

		scene->getWaterLevel() = pt.get("water.waterLevel",RAND_MAX);
		scene->getWaterDepth() = pt.get("water.waterDepth",-75);
		std::string sceneLocation = par.getParam<string>("scriptLocation") + "/" + 
				                    par.getParam<string>("scenesLocation") + "/" + sceneName + "/";

		loadTerrain(sceneLocation   + pt.get("terrain","terrain.xml"));
		loadGeometry(sceneLocation  + pt.get("assets","assets.xml"));
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
			TerrainDescriptor* ter = ResourceManager::getInstance().loadResource<TerrainDescriptor>(name+"_descriptor");
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
			model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x",1);
			model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y",1);
			model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z",1);
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

	Material* loadMaterial(const string &file){
		Material* mat = NULL;
	
		string location = par.getParam<string>("scriptLocation") + "/" + 
				                    par.getParam<string>("scenesLocation") + "/" +
									par.getParam<string>("currentScene") + "/materials/";

		return loadMaterialXML(location+file);
		
	}

	Material* loadMaterialXML(const string &matName){
		pt.clear();
		ifstream inp;
		inp.open(string(matName+".xml").c_str(), ifstream::in);
		inp.close();
		if(inp.fail()){
			inp.clear(ios::failbit);
			return NULL;
		}
		bool skip = false;
		Console::getInstance().printfn("Loading material from XML file [ %s ]",matName.c_str());
		read_xml(matName+".xml",pt);
		string materialName = matName.substr(matName.rfind("/")+1,matName.length());
		if(ResourceManager::getInstance().find(materialName)) skip = true;
		Material* mat = ResourceManager::getInstance().loadResource<Material>(ResourceDescriptor(materialName));
		if(skip) return mat;
		mat->setDiffuse(vec4(pt.get<F32>("material.diffuse.<xmlattr>.r",0.6f),
							 pt.get<F32>("material.diffuse.<xmlattr>.g",0.6f),
							 pt.get<F32>("material.diffuse.<xmlattr>.b",0.6f),
							 pt.get<F32>("material.diffuse.<xmlattr>.a",1.f)));
		mat->setAmbient(vec4(pt.get<F32>("material.ambient.<xmlattr>.r",0.6f),
						     pt.get<F32>("material.ambient.<xmlattr>.g",0.6f),
							 pt.get<F32>("material.ambient.<xmlattr>.b",0.6f),
							 pt.get<F32>("material.ambient.<xmlattr>.a",1.f)));
		mat->setSpecular(vec4(pt.get<F32>("material.specular.<xmlattr>.r",1.f),
			                  pt.get<F32>("material.specular.<xmlattr>.g",1.f),
							  pt.get<F32>("material.specular.<xmlattr>.b",1.f),
							  pt.get<F32>("material.specular.<xmlattr>.a",1.f)));
		mat->setEmissive(vec3(pt.get<F32>("material.emissive.<xmlattr>.r",1.f),
			                  pt.get<F32>("material.emissive.<xmlattr>.g",1.f),
							  pt.get<F32>("material.emissive.<xmlattr>.b",1.f)));
		mat->setShininess(pt.get<F32>("material.shininess.<xmlattr>.v",50.f));
		if(boost::optional<ptree &> child = pt.get_child_optional("diffuseTexture1")){
			mat->setTexture(Material::TEXTURE_BASE,loadTextureXML(pt.get("diffuseTexture1.file","none")));
		}
		if(boost::optional<ptree &> child = pt.get_child_optional("diffuseTexture2")){
			mat->setTexture(Material::TEXTURE_SECOND,loadTextureXML(pt.get("diffuseTexture2.file","none")));
		}
		if(boost::optional<ptree &> child = pt.get_child_optional("bumpMap")){
			mat->setTexture(Material::TEXTURE_BUMP,loadTextureXML(pt.get("bumpMap.file","none")));
		}
		if(boost::optional<ptree &> child = pt.get_child_optional("shader")){
			std::string fragShader = pt.get<string>("shader.pixelShader","none");
			std::string vertShader = pt.get<string>("shader.vertexShader","none");
			if(fragShader.compare("none") != 0 && vertShader.compare("none") != 0){
				mat->setShader(fragShader+","+vertShader);
			}else if(fragShader.compare("none") != 0 && vertShader.compare("none") == 0){
				mat->setShader(fragShader);
			}else if(fragShader.compare("none") == 0 && vertShader.compare("none") != 0){
				mat->setShader(vertShader);
			}else{
				mat->setShader("");
			}
		}
	
		return mat;

	}

	void dumpMaterial(Material* const mat){
		std::string file = mat->getName();
		file = file.substr(file.rfind("/")+1,file.length());

		std::string location = par.getParam<string>("scriptLocation") + "/" + 
				                    par.getParam<string>("scenesLocation") + "/" +
									par.getParam<string>("currentScene") + "/materials/";
		
		string fileLocation = location +  file + ".xml";
		if(!mat->isDirty()) return;
		pt.clear();
		pt.put("material.name",file);
		pt.put("material.ambient.<xmlattr>.r",mat->getMaterialMatrix().getCol(0).x);
		pt.put("material.ambient.<xmlattr>.g",mat->getMaterialMatrix().getCol(0).y);
		pt.put("material.ambient.<xmlattr>.b",mat->getMaterialMatrix().getCol(0).z);
		pt.put("material.ambient.<xmlattr>.a",mat->getMaterialMatrix().getCol(0).w);
		pt.put("material.diffuse.<xmlattr>.r",mat->getMaterialMatrix().getCol(1).x);
		pt.put("material.diffuse.<xmlattr>.g",mat->getMaterialMatrix().getCol(1).y);
		pt.put("material.diffuse.<xmlattr>.b",mat->getMaterialMatrix().getCol(1).z);
		pt.put("material.diffuse.<xmlattr>.a",mat->getMaterialMatrix().getCol(1).w);
		pt.put("material.specular.<xmlattr>.r",mat->getMaterialMatrix().getCol(2).x);
		pt.put("material.specular.<xmlattr>.g",mat->getMaterialMatrix().getCol(2).y);
		pt.put("material.specular.<xmlattr>.b",mat->getMaterialMatrix().getCol(2).z);
		pt.put("material.specular.<xmlattr>.a",mat->getMaterialMatrix().getCol(2).w);
		pt.put("material.shininess.<xmlattr>.v",mat->getMaterialMatrix().getCol(3).x);
		pt.put("material.emissive.<xmlattr>.r",mat->getMaterialMatrix().getCol(3).y);
		pt.put("material.emissive.<xmlattr>.g",mat->getMaterialMatrix().getCol(3).z);
		pt.put("material.emissive.<xmlattr>.b",mat->getMaterialMatrix().getCol(3).w);
		Texture* baseTexture = mat->getTexture(mat->TEXTURE_BASE);
		if(baseTexture){
			pt.put("diffuseTexture1.file",baseTexture->getResourceLocation());
			pt.put("diffuseTexture1.MapU","CLAMP");
			pt.put("diffuseTexture1.MapV","CLAMP");
			pt.put("diffuseTexture1.minFilter","LINEAR");
			pt.put("diffuseTexture1.magFilter","LINEAR");
			pt.put("diffuseTexture1.mipFilter",true);
		}
		Texture* secondTexture = mat->getTexture(mat->TEXTURE_SECOND);
		if(secondTexture){
			pt.put("diffuseTexture2.file",secondTexture->getResourceLocation());
			pt.put("diffuseTexture2.MapU","CLAMP");
			pt.put("diffuseTexture2.MapV","CLAMP");
			pt.put("diffuseTexture2.minFilter","LINEAR");
			pt.put("diffuseTexture2.magFilter","LINEAR");
			pt.put("diffuseTexture2.mipFilter",true);
		}
		Texture* bumpTexture = mat->getTexture(mat->TEXTURE_BUMP);
		if(bumpTexture){
			pt.put("bumpMap.file",bumpTexture->getResourceLocation());
			pt.put("bumpMap.MapU","CLAMP");
			pt.put("bumpMap.MapV","CLAMP");
			pt.put("bumpMap.minFilter","LINEAR");
			pt.put("bumpMap.magFilter","LINEAR");
			pt.put("bumpMap.mipFilter",true);
		}
		Shader* s = mat->getShader();
		if(s){
			pt.put("shader.pixelShader",s->getFragName());
			pt.put("shader.vertexShader",s->getVertName());
		}
		
		boost::property_tree::xml_writer_settings<char> settings('\t', 1);
		FILE * xml = fopen(fileLocation.c_str(), "w");
		write_xml(fileLocation, pt,std::locale(),settings);
		fclose(xml);
	}


	Texture*  loadTextureXML(const std::string& textureName){
		std::string img_name = textureName.substr( textureName.find_last_of( '/' ) + 1 );
		std::string pathName = textureName.substr( 0, textureName.rfind("/")+1 );
		ResourceDescriptor texture(img_name);
		texture.setResourceLocation(pathName + img_name);
		texture.setFlag(true);
		Texture* tex1 = ResourceManager::getInstance().loadResource<Texture2D>(texture);

		std::string clampU = pt.get("diffuseTexture1.MapU","CLAMP");
		std::string clampV = pt.get("diffuseTexture1.MapV","CLAMP");
		bool wrapU = false;
		bool wrapV = false;
		if(clampU.compare("WRAP")){
			wrapU = true;
		}else if (clampU.compare("CLAMP")){
			wrapU = false;
		}else{
			Console::getInstance().errorfn("Invalid U-map parameter for [ %s ]",textureName.c_str());
		}
		if(clampV.compare("WRAP")){
			wrapV = true;
		}else if (clampV.compare("CLAMP")){
			wrapV = false;
		}else{
			Console::getInstance().errorfn("Invalid V-map parameter for [ %s ]",textureName.c_str());
		}
		tex1->setTextureWrap(wrapU,wrapV);

		std::string minFilter = pt.get("diffuseTexture1.minFilter","LINEAR");
		std::string magFilter = pt.get("diffuseTexture1.magFilter","LINEAR");
		bool mipFilter = pt.get("diffuseTexture1.mipFilter",true);
		U8 minFilterValue = Texture::LINEAR_MIPMAP_LINEAR;
		U8 magFilterValue = Texture::LINEAR_MIPMAP_LINEAR;
		if(mipFilter){
			if(minFilter.compare("LINEAR")){
				minFilterValue = Texture::LINEAR_MIPMAP_LINEAR;
			}else if(minFilter.compare("NEAREST")){
				minFilterValue = Texture::NEAREST_MIPMAP_NEAREST;
			}else{
			Console::getInstance().errorfn("Invalid min filter parameter for [ %s ]",textureName.c_str());
			}
			if(magFilter.compare("LINEAR")){
				magFilterValue = Texture::LINEAR_MIPMAP_LINEAR;
			}else if(magFilter.compare("NEAREST")){
				magFilterValue= Texture::NEAREST_MIPMAP_NEAREST;
			}else{
				Console::getInstance().errorfn("Invalid mag filter parameter for [ %s ]",textureName.c_str());
			}

		}else{
			if(minFilter.compare("LINEAR")){
				minFilterValue = Texture::LINEAR;
			}else if(minFilter.compare("NEAREST")){
				minFilterValue = Texture::NEAREST;
			}else{
				Console::getInstance().errorfn("Invalid min filter parameter for [ %s ]",textureName.c_str());
			}
			if(magFilter.compare("LINEAR")){
				magFilterValue = Texture::LINEAR;
			}else if(magFilter.compare("NEAREST")){
				magFilterValue = Texture::NEAREST;
			}else{
				Console::getInstance().errorfn("Invalid mag filter parameter for [ %s ]",textureName.c_str());
			}
		}
		tex1->setTextureFilters(minFilterValue,magFilterValue);
		return tex1;
	}
}
