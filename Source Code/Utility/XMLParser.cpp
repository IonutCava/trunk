#include "Headers/XMLParser.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

namespace XML {

	using boost::property_tree::ptree;
	ptree pt;

	std::string loadScripts(const std::string& file){

		ParamHandler &par = ParamHandler::getInstance();
		PRINT_FN(Locale::get("XML_LOAD_SCRIPTS"));
		read_xml(file,pt);
		std::string activeScene("MainScene");
		par.setParam("scriptLocation",pt.get("scriptLocation","XML"));
		par.setParam("assetsLocation",pt.get("assets","Assets"));
		par.setParam("scenesLocation",pt.get("scenes","Scenes"));
		par.setParam("serverAddress",pt.get("server","127.0.0.1"));
		loadConfig(par.getParam<std::string>("scriptLocation") + "/" + pt.get("config","config.xml"));

		read_xml(par.getParam<std::string>("scriptLocation") + "/" +
			     par.getParam<std::string>("scenesLocation") + "/Scenes.xml",pt);
		activeScene = pt.get("MainScene",activeScene);
		return activeScene;
	}

	void loadConfig(const std::string& file) {
		ParamHandler &par = ParamHandler::getInstance();
		pt.clear();
		PRINT_FN(Locale::get("XML_LOAD_CONFIG"), file.c_str());
		read_xml(file,pt);
		par.setParam("locale",pt.get("language","enGB"));
		par.setParam("logFile",pt.get("debug.logFile","none"));
		par.setParam("memFile",pt.get("debug.memFile","none"));
		par.setParam("groundPos", pt.get("runtime.groundPos",-2000.0f));
		par.setParam("simSpeed",pt.get("runtime.simSpeed",1));
		par.setParam("appTitle",pt.get("title","DIVIDE Framework"));
		par.setParam("detailLevel",pt.get<U8>("rendering.detailLevel",HIGH));
		
		U8 shadowDetailLevel = pt.get<U8>("rendering.shadowDetailLevel",HIGH);
		U8 shadowResolutionFactor = 1;
		switch(shadowDetailLevel){
			default:
			case HIGH:
				shadowResolutionFactor = 1;
				break;
			case MEDIUM:
				shadowResolutionFactor = 2;
				break;
			case LOW:
				shadowResolutionFactor = 4;
				break;
		};
		par.setParam("shadowDetailLevel",shadowDetailLevel);
		par.setParam("shadowResolutionFactor", shadowResolutionFactor);
		par.setParam("enableShadows",pt.get("rendering.enableShadows", true));
		par.setParam("defaultTextureLocation",pt.get("defaultTextureLocation","textures/"));
		par.setParam("shaderLocation",pt.get("defaultShadersLocation","shaders/"));
		I32 resWidth = pt.get("runtime.resolutionWidth",1024.0f);
		I32 resHeight = pt.get("runtime.resolutionHeight",768.0f);
		par.setParam("zNear",(F32)pt.get("runtime.zNear",0.1f));
		par.setParam("zFar",(F32)pt.get("runtime.zFar",1200.0f));
		par.setParam("verticalFOV",(F32)pt.get("runtime.verticalFOV",60));
		par.setParam("aspectRatio",1.0f * resWidth / resHeight);
		par.setParam("resolutionWidth",resWidth);
		par.setParam("resolutionHeight",resHeight);
		par.setParam("rendering.enableFog", pt.get("rendering.enableFog",true));

		Application::getInstance().setResolutionHeight(resHeight);
		Application::getInstance().setResolutionWidth(resWidth);
		bool postProcessing = pt.get("rendering.enablePostFX",false);
		par.setParam("postProcessing.enablePostFX",postProcessing);
		if(postProcessing){
			bool enable3D = pt.get("rendering.enable3D",false);
			par.setParam("postProcessing.enable3D",enable3D);
			if(enable3D){
				par.setParam("postProcessing.anaglyphOffset",pt.get("rendering.anaglyphOffset",0.16f));
			}
			par.setParam("postProcessing.enableNoise",pt.get("rendering.enableNoise",false));
			par.setParam("postProcessing.enableDepthOfField",pt.get("rendering.enableDepthOfField",false));
			par.setParam("postProcessing.enableBloom",pt.get("rendering.enableBloom",false));
			par.setParam("postProcessing.enableSSAO",pt.get("rendering.enableSSAO",false));
		}
		par.setParam("mesh.playAnimations",pt.get("mesh.playAnimations",true));
	}

	void loadScene(const std::string& sceneName, SceneManager& sceneMgr) {
		ParamHandler &par = ParamHandler::getInstance();
		pt.clear();
		PRINT_FN(Locale::get("XML_LOAD_SCENE"), sceneName.c_str());
		std::string sceneLocation = par.getParam<std::string>("scriptLocation") + "/" + 
				                    par.getParam<std::string>("scenesLocation") + "/" + sceneName;
		try{
			read_xml(sceneLocation + ".xml", pt);
		}catch(	boost::property_tree::xml_parser_error & e ){
			ERROR_FN(Locale::get("ERROR_XML_INVALID_SCENE"),sceneName.c_str());
			std::string error = e.what();
			error += " [check error log!]";
			throw error.c_str();
		}
		par.setParam("currentScene",sceneName);
		Scene* scene = sceneMgr.createScene(sceneName);

		if(!scene)	{
			ERROR_FN(Locale::get("ERROR_XML_LOAD_INVALID_SCENE"));
			return;
		}

		sceneMgr.setActiveScene(scene);
		scene->setName(sceneName);

		scene->getGrassVisibility() = pt.get("vegetation.grassVisibility",1000.0f);
		scene->getTreeVisibility()  = pt.get("vegetation.treeVisibility",1000.0f);
		scene->getGeneralVisibility()  = pt.get("options.visibility",1000.0f);

		scene->getWindDirX()  = pt.get("wind.windDirX",1.0f);
		scene->getWindDirZ()  = pt.get("wind.windDirZ",1.0f);
		scene->getWindSpeed() = pt.get("wind.windSpeed",1.0f);

		scene->getWaterLevel() = pt.get("water.waterLevel",RAND_MAX);
		scene->getWaterDepth() = pt.get("water.waterDepth",-75);

		loadTerrain(sceneLocation + "/" + pt.get("terrain","terrain.xml"),scene);
		loadGeometry(sceneLocation + "/" + pt.get("assets","assets.xml"),scene);
	}



	void loadTerrain(const std::string &file, Scene* const scene) {
		U8 count = 0;
		pt.clear();
		PRINT_FN(Locale::get("XML_LOAD_TERRAIN"),file.c_str());
		read_xml(file,pt);
		ptree::iterator it;
		std::string assetLocation = ParamHandler::getInstance().getParam<std::string>("assetsLocation") + "/"; 
		for (it = pt.get_child("terrainList").begin(); it != pt.get_child("terrainList").end(); ++it )	{
			std::string name = it->second.data(); //The actual terrain name
			std::string tag = it->first.data();   //The <name> tag for valid terrains or <xmlcomment> for comments
			//Check and skip commented terrain
			if(tag.find("<xmlcomment>") != std::string::npos) continue;
			//Load the rest of the terrain
			TerrainDescriptor* ter = CreateResource<TerrainDescriptor>(name+"_descriptor");
			ter->addVariable("terrainName",name);
			ter->addVariable("heightmap",assetLocation + pt.get<std::string>(name + ".heightmap"));
			ter->addVariable("textureMap",assetLocation + pt.get<std::string>(name + ".textures.map"));
			ter->addVariable("redTexture",assetLocation + pt.get<std::string>(name + ".textures.red"));
			ter->addVariable("greenTexture",assetLocation + pt.get<std::string>(name + ".textures.green"));
			ter->addVariable("blueTexture",assetLocation + pt.get<std::string>(name + ".textures.blue"));
			ter->addVariable("alphaTexture",assetLocation + pt.get<std::string>(name + ".textures.alpha","none"));
			ter->addVariable("normalMap",assetLocation + pt.get<std::string>(name + ".textures.normalMap"));
			ter->addVariable("waterCaustics",assetLocation + pt.get<std::string>(name + ".textures.waterCaustics"));
			ter->addVariable("grassMap",assetLocation + pt.get<std::string>(name + ".vegetation.map"));
			ter->addVariable("grassBillboard1",assetLocation + pt.get<std::string>(name + ".vegetation.grassBillboard1"));
			ter->addVariable("grassBillboard2",assetLocation + pt.get<std::string>(name + ".vegetation.grassBillboard2"));
			ter->addVariable("grassBillboard3",assetLocation + pt.get<std::string>(name + ".vegetation.grassBillboard3"));
			//ter->addVariable("grassBillboard1",pt.get<std::string>(name + ".vegetation.grassBillboard1"));
			ter->setGrassDensity(pt.get<U32>(name + ".vegetation.<xmlattr>.grassDensity"));
			ter->setTreeDensity(pt.get<U16>(name + ".vegetation.<xmlattr>.treeDensity"));
			ter->setGrassScale(pt.get<F32>(name + ".vegetation.<xmlattr>.grassScale"));
			ter->setTreeScale(pt.get<F32>(name + ".vegetation.<xmlattr>.treeScale"));

			ter->setPosition(vec3<F32>(pt.get<F32>(name + ".position.<xmlattr>.x"),
								       pt.get<F32>(name + ".position.<xmlattr>.y"),
								       pt.get<F32>(name + ".position.<xmlattr>.z")));
			ter->setScale(vec2<F32>(pt.get<F32>(name + ".scale"), //width / length
							        pt.get<F32>(name + ".heightFactor"))); //height
							   

			ter->setActive(pt.get<bool>(name + ".active"));
			
			scene->addTerrain(ter);
			count++;
			
		}
		PRINT_FN(Locale::get("XML_TERRAIN_COUNT"),count);
	}

	void loadGeometry(const std::string &file, Scene* const scene) 	{

		pt.clear();
		PRINT_FN(Locale::get("XML_LOAD_GEOMETRY"),file.c_str());
		read_xml(file,pt);
		ptree::iterator it;
		std::string assetLocation = ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/";

		if(boost::optional<ptree &> geometry = pt.get_child_optional("geometry"))
		for (it = pt.get_child("geometry").begin(); it != pt.get_child("geometry").end(); ++it ) 	{
			std::string name = it->second.data();
			std::string format = it->first.data();
			if(format.find("<xmlcomment>") != std::string::npos) continue;
			FileData model;
			model.ItemName = name;
			model.ModelName  = assetLocation + pt.get<std::string>(name + ".model");
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
			scene->addModel(model);
		}

		if(boost::optional<ptree &> vegetation = pt.get_child_optional("vegetation"))
		for (it = pt.get_child("vegetation").begin(); it != pt.get_child("vegetation").end(); ++it ) {

			std::string name = it->second.data();
			std::string format = it->first.data();
			if(format.find("<xmlcomment>") != std::string::npos) continue;
			FileData model;
			model.ItemName = name;
			model.ModelName  = assetLocation + pt.get<std::string>(name + ".model");
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
			scene->addModel(model);
		}

		if(boost::optional<ptree &> primitives = pt.get_child_optional("primitives"))
		for (it = pt.get_child("primitives").begin(); it != pt.get_child("primitives").end(); ++it ) {

			std::string name = it->second.data();
			std::string format = it->first.data();
			if(format.find("<xmlcomment>") != std::string::npos) continue;

			FileData model;
			model.ItemName = name;
			model.ModelName = pt.get<std::string>(name + ".model");
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
			if(model.ModelName.compare("Text3D") == 0){
				model.data = pt.get<F32>(name + ".lineWidth");
				model.data2 = pt.get<std::string>(name + ".text");
			}else if(model.ModelName.compare("Box3D") == 0)
				model.data = pt.get<F32>(name + ".size");
			else if(model.ModelName.compare("Sphere3D") == 0)
				model.data = pt.get<F32>(name + ".radius");
			else model.data = 0;
			
			model.type = PRIMITIVE;
			model.version = pt.get<F32>(name + ".version");
			scene->addModel(model);
		}
	}

	Material* loadMaterial(const std::string &file){
		ParamHandler &par = ParamHandler::getInstance();
		std::string location = par.getParam<std::string>("scriptLocation") + "/" + 
				                    par.getParam<std::string>("scenesLocation") + "/" +
									par.getParam<std::string>("currentScene") + "/materials/";

		return loadMaterialXML(location+file);
		
	}

	Material* loadMaterialXML(const std::string &matName){
		pt.clear();
		std::ifstream inp;
		inp.open(std::string(matName+".xml").c_str(), std::ifstream::in);
		inp.close();
		if(inp.fail()){
			inp.clear(std::ios::failbit);
			return NULL;
		}
		bool skip = false;
		PRINT_FN(Locale::get("XML_LOAD_MATERIAL"),matName.c_str());
		read_xml(matName+".xml",pt);
		std::string materialName = matName.substr(matName.rfind("/")+1,matName.length());
		if(FindResource(materialName)) skip = true;
		Material* mat = CreateResource<Material>(ResourceDescriptor(materialName));
		if(skip) return mat;
		mat->setDiffuse(vec4<F32>(pt.get<F32>("material.diffuse.<xmlattr>.r",0.6f),
								  pt.get<F32>("material.diffuse.<xmlattr>.g",0.6f),
								  pt.get<F32>("material.diffuse.<xmlattr>.b",0.6f),
								  pt.get<F32>("material.diffuse.<xmlattr>.a",1.f)));
		mat->setAmbient(vec4<F32>(pt.get<F32>("material.ambient.<xmlattr>.r",0.6f),
								  pt.get<F32>("material.ambient.<xmlattr>.g",0.6f),
								  pt.get<F32>("material.ambient.<xmlattr>.b",0.6f),
								  pt.get<F32>("material.ambient.<xmlattr>.a",1.f)));
		mat->setSpecular(vec4<F32>(pt.get<F32>("material.specular.<xmlattr>.r",1.f),
								   pt.get<F32>("material.specular.<xmlattr>.g",1.f),
								   pt.get<F32>("material.specular.<xmlattr>.b",1.f),
								   pt.get<F32>("material.specular.<xmlattr>.a",1.f)));
		mat->setEmissive(vec3<F32>(pt.get<F32>("material.emissive.<xmlattr>.r",1.f),
			                       pt.get<F32>("material.emissive.<xmlattr>.g",1.f),
							       pt.get<F32>("material.emissive.<xmlattr>.b",1.f)));
		mat->setShininess(pt.get<F32>("material.shininess.<xmlattr>.v",50.f));
		mat->setDoubleSided(pt.get<bool>("material.doubleSided",false));
		if(boost::optional<ptree &> child = pt.get_child_optional("diffuseTexture1")){
			mat->setTexture(Material::TEXTURE_BASE,loadTextureXML(pt.get("diffuseTexture1.file","none")),
							mat->getTextureOperation(pt.get("diffuseTexture1.operation",0)));
		}
		if(boost::optional<ptree &> child = pt.get_child_optional("diffuseTexture2")){
			mat->setTexture(Material::TEXTURE_SECOND,loadTextureXML(pt.get("diffuseTexture2.file","none")),
							mat->getTextureOperation(pt.get("diffuseTexture2.operation",0)));
		}
		if(boost::optional<ptree &> child = pt.get_child_optional("bumpMap")){
			mat->setTexture(Material::TEXTURE_BUMP,loadTextureXML(pt.get("bumpMap.file","none")),
							mat->getTextureOperation(pt.get("bumpMap.operation",0)));
		}
		if(boost::optional<ptree &> child = pt.get_child_optional("opacityMap")){
			mat->setTexture(Material::TEXTURE_OPACITY,loadTextureXML(pt.get("opacityMap.file","none")),
							mat->getTextureOperation(pt.get("opacityMap.operation",0)));
		}
		if(boost::optional<ptree &> child = pt.get_child_optional("specularMap")){
			mat->setTexture(Material::TEXTURE_SPECULAR,loadTextureXML(pt.get("specularMap.file","none")),
							mat->getTextureOperation(pt.get("specularMap.operation",0)));
		}
		if(boost::optional<ptree &> child = pt.get_child_optional("shaderProgram")){
			mat->setShaderProgram(pt.get("shaderProgram.effect","NULL_SHADER"));
		}
		if(boost::optional<ptree &> child = pt.get_child_optional("shadows")){
			mat->setCastsShadows(pt.get<bool>("shadows.castsShadows", true));
			mat->setReceivesShadows(pt.get<bool>("shadows.receiveShadows", true));
		}
		return mat;

	}

	void dumpMaterial(Material* const mat){
		ParamHandler &par = ParamHandler::getInstance();
		std::string file = mat->getName();
		file = file.substr(file.rfind("/")+1,file.length());

		std::string location = par.getParam<std::string>("scriptLocation") + "/" + 
				                    par.getParam<std::string>("scenesLocation") + "/" +
									par.getParam<std::string>("currentScene") + "/materials/";
		
		std::string fileLocation = location +  file + ".xml";
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
		pt.put("material.doubleSided", mat->isDoubleSided());
		Texture* baseTexture = mat->getTexture(Material::TEXTURE_BASE);

		if(baseTexture){
			pt.put("diffuseTexture1.file",baseTexture->getResourceLocation());
			U32 wrap[3] = {baseTexture->getTextureWrap(0),
						   baseTexture->getTextureWrap(1),
						   baseTexture->getTextureWrap(2)};
			if(wrap[0] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapU","WRAP");
			}else if (wrap[0] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapU","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapU","REPEAT");
			}
			if(wrap[1] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapV","WRAP");
			}else if (wrap[1] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapV","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapV","REPEAT");
			}
			if(wrap[2] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapW","WRAP");
			}else if (wrap[2] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapW","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapW","REPEAT");
			}
			pt.put("diffuseTexture1.minFilter","LINEAR");
			pt.put("diffuseTexture1.magFilter","LINEAR");
			pt.put("diffuseTexture1.mipFilter",true);
			pt.put("diffuseTexture1.operation",mat->getTextureOperation(Material::TEXTURE_BASE));
		}
		Texture* secondTexture = mat->getTexture(Material::TEXTURE_SECOND);
		if(secondTexture){
			pt.put("diffuseTexture2.file",secondTexture->getResourceLocation());
			U32 wrap[3] = {secondTexture->getTextureWrap(0),
						   secondTexture->getTextureWrap(1),
						   secondTexture->getTextureWrap(2)};
			if(wrap[0] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapU","WRAP");
			}else if (wrap[0] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapU","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapU","REPEAT");
			}
			if(wrap[1] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapV","WRAP");
			}else if (wrap[1] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapV","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapV","REPEAT");
			}
			if(wrap[2] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapW","WRAP");
			}else if (wrap[2] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapW","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapW","REPEAT");
			}
			pt.put("diffuseTexture2.minFilter","LINEAR");
			pt.put("diffuseTexture2.magFilter","LINEAR");
			pt.put("diffuseTexture2.mipFilter",true);
			pt.put("diffuseTexture2.operation",mat->getTextureOperation(Material::TEXTURE_SECOND));
		}
		Texture* bumpTexture = mat->getTexture(Material::TEXTURE_BUMP);
		if(bumpTexture){
			pt.put("bumpMap.file",bumpTexture->getResourceLocation());
			U32 wrap[3] = {bumpTexture->getTextureWrap(0),
						   bumpTexture->getTextureWrap(1),
						   bumpTexture->getTextureWrap(2)};
			if(wrap[0] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapU","WRAP");
			}else if (wrap[0] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapU","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapU","REPEAT");
			}
			if(wrap[1] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapV","WRAP");
			}else if (wrap[1] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapV","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapV","REPEAT");
			}
			if(wrap[2] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapW","WRAP");
			}else if (wrap[2] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapW","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapW","REPEAT");
			}
			pt.put("bumpMap.minFilter","LINEAR");
			pt.put("bumpMap.magFilter","LINEAR");
			pt.put("bumpMap.mipFilter",true);
			pt.put("bumpMap.operation",mat->getTextureOperation(Material::TEXTURE_BUMP));
		}
		Texture* opacityMap = mat->getTexture(Material::TEXTURE_OPACITY);
		if(opacityMap){
			pt.put("opacityMap.file",opacityMap->getResourceLocation());
			U32 wrap[3] = {opacityMap->getTextureWrap(0),
						   opacityMap->getTextureWrap(1),
						   opacityMap->getTextureWrap(2)};
			if(wrap[0] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapU","WRAP");
			}else if (wrap[0] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapU","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapU","REPEAT");
			}
			if(wrap[1] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapV","WRAP");
			}else if (wrap[1] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapV","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapV","REPEAT");
			}
			if(wrap[2] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapW","WRAP");
			}else if (wrap[2] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapW","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapW","REPEAT");
			}
			pt.put("opacityMap.minFilter","LINEAR");
			pt.put("opacityMap.magFilter","LINEAR");
			pt.put("opacityMap.mipFilter",true);
			pt.put("opacityMap.operation",mat->getTextureOperation(Material::TEXTURE_OPACITY));
		}
		Texture* specularMap = mat->getTexture(Material::TEXTURE_SPECULAR);
		if(specularMap){
			pt.put("specularMap.file",specularMap->getResourceLocation());
			U32 wrap[3] = {specularMap->getTextureWrap(0),
						   specularMap->getTextureWrap(1),
						   specularMap->getTextureWrap(2)};
			if(wrap[0] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapU","WRAP");
			}else if (wrap[0] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapU","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapU","REPEAT");
			}
			if(wrap[1] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapV","WRAP");
			}else if (wrap[1] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapV","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapV","REPEAT");
			}
			if(wrap[2] ==  Texture::TextureWrap_Wrap){
				pt.put("diffuseTexture1.MapW","WRAP");
			}else if (wrap[2] ==  Texture::TextureWrap_Clamp){
				pt.put("diffuseTexture1.MapW","CLAMP");
			}else {
				pt.put("diffuseTexture1.MapW","REPEAT");
			}
			pt.put("specularMap.minFilter","LINEAR");
			pt.put("specularMap.magFilter","LINEAR");
			pt.put("specularMap.mipFilter",true);
			pt.put("specularMap.operation",mat->getTextureOperation(Material::TEXTURE_SPECULAR));
		}
		
		ShaderProgram* s = mat->getShaderProgram();
		if(s){
			pt.put("shaderProgram.effect",s->getName());
		}

		pt.put("shadows.castsShadows", mat->getCastsShadows());
		pt.put("shadows.receiveShadows", mat->getReceivesShadows());
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
		Texture* tex1 = CreateResource<Texture2D>(texture);

		std::string clampU = pt.get("diffuseTexture1.MapU","CLAMP");
		std::string clampV = pt.get("diffuseTexture1.MapV","CLAMP");
		std::string clampW = pt.get("diffuseTexture1.MapW","CLAMP");
		Texture::TextureWrap wrapU = Texture::TextureWrap_Repeat;
		Texture::TextureWrap wrapV = Texture::TextureWrap_Repeat;
		Texture::TextureWrap wrapW = Texture::TextureWrap_Repeat;

		if(clampU.compare("WRAP")){
			wrapU = Texture::TextureWrap_Wrap;
		}else if (clampU.compare("CLAMP")){
			wrapU = Texture::TextureWrap_Clamp;
		}else if (clampU.compare("REPEAT")){
			wrapU = Texture::TextureWrap_Repeat;
		}else{
			ERROR_FN(Locale::get("ERROR_XML_INVALID_U_PARAM"),textureName.c_str());
		}
		if(clampV.compare("WRAP")){
			wrapV = Texture::TextureWrap_Wrap;
		}else if (clampV.compare("CLAMP")){
			wrapV = Texture::TextureWrap_Clamp;
		}else if (clampV.compare("REPEAT")){
			wrapV = Texture::TextureWrap_Repeat;
		}else{
			ERROR_FN(Locale::get("ERROR_XML_INVALID_V_PARAM"),textureName.c_str());
		}
		if(clampW.compare("WRAP")){
			wrapW = Texture::TextureWrap_Wrap;
		}else if (clampW.compare("CLAMP")){
			wrapW = Texture::TextureWrap_Clamp;
		}else if (clampW.compare("REPEAT")){
			wrapW = Texture::TextureWrap_Repeat;
		}else{
			ERROR_FN(Locale::get("ERROR_XML_INVALID_W_PARAM"),textureName.c_str());
		}

		tex1->setTextureWrap(wrapU,wrapV,wrapW);

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
				ERROR_FN(Locale::get("ERROR_XML_INVALID_MIN_FILTER"),textureName.c_str());
			}
			if(magFilter.compare("LINEAR")){
				magFilterValue = Texture::LINEAR_MIPMAP_LINEAR;
			}else if(magFilter.compare("NEAREST")){
				magFilterValue= Texture::NEAREST_MIPMAP_NEAREST;
			}else{
				ERROR_FN(Locale::get("ERROR_XML_INVALID_MAG_FILTER"),textureName.c_str());
			}

		}else{
			if(minFilter.compare("LINEAR")){
				minFilterValue = Texture::LINEAR;
			}else if(minFilter.compare("NEAREST")){
				minFilterValue = Texture::NEAREST;
			}else{
				ERROR_FN(Locale::get("ERROR_XML_INVALID_MIN_FILTER"),textureName.c_str());
			}
			if(magFilter.compare("LINEAR")){
				magFilterValue = Texture::LINEAR;
			}else if(magFilter.compare("NEAREST")){
				magFilterValue = Texture::NEAREST;
			}else{
				ERROR_FN(Locale::get("ERROR_XML_INVALID_MAG_FILTER"),textureName.c_str());
			}
		}
		tex1->setTextureFilters(minFilterValue,magFilterValue);
		return tex1;
	}
}
