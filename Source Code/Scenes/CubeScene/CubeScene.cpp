#include "Headers/CubeScene.h"

#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Managers/Headers/SceneManager.h"
#include "GUI/Headers/GUI.h"

REGISTER_SCENE(CubeScene);

void CubeScene::render(){

	GUI& gui = GUI::getInstance();
	//Pass 2
	//Draw a 2D fullscreen quad that has the lighting shader applied to it and all generated textures bound to that shader
	_GFX.toggle2D(true);

	_deferredShader->bind();
		_deferredBuffer->Bind(0,0);
		_deferredBuffer->Bind(1,1);
		_deferredBuffer->Bind(2,2);
		_lightTexture->Bind(3);

			_deferredShader->UniformTexture("tImage0",0);
			_deferredShader->UniformTexture("tImage1",1);
			_deferredShader->UniformTexture("tImage2",2);
			_deferredShader->UniformTexture("lightTexture",3);
			_deferredShader->Uniform("lightCount",(I32)LightManager::getInstance().getLights().size());
			_deferredShader->Uniform("cameraPosition",_camera->getEye());

			_GFX.renderModel(_renderQuad);

		_lightTexture->Unbind(3);
		_deferredBuffer->Unbind(2);
		_deferredBuffer->Unbind(1);
		_deferredBuffer->Unbind(0);
	_deferredShader->unbind();

	if(_showTextures){

		_lightTexture->Bind(0);
			_GFX.renderModel(_renderQuad2);
		_lightTexture->Unbind(0);

		_deferredBuffer->Bind(0,1);
			_GFX.renderModel(_renderQuad3);
		_deferredBuffer->Unbind(0);

		_deferredBuffer->Bind(0,2);
			_GFX.renderModel(_renderQuad4);
		_deferredBuffer->Unbind(0);

	}
	_GFX.toggle2D(false);

	GUIElement* guiElement = gui.getGuiElement("FinalImage");
	if(guiElement){
		guiElement->setVisible(_showTextures);
	}
	guiElement = gui.getGuiElement("LightTexture");
	if(guiElement){
		guiElement->setVisible(_showTextures);
	}
	guiElement = gui.getGuiElement("PositionData");
	if(guiElement){
		guiElement->setVisible(_showTextures);
	}
	guiElement = gui.getGuiElement("NormalData");
	if(guiElement){
		guiElement->setVisible(_showTextures);
	}
		
}

void CubeScene::processEvents(F32 time){
	LightManager::LightMap& lights = LightManager::getInstance().getLights();
	F32 updateLights = 0.005f;

	if(time - _eventTimers[0] >= updateLights){
		for(U8 row=0; row<3; row++)
			for(U8 col=0; col < lights.size()/3.0f; col++){
				F32 x = col * 150.0f - 5.0f + cos(GETMSTIME()*(col-row+2)*0.008f)*200.0f;
				F32 y = cos(GETTIME()*(col-row+2)*0.01f)*200.0f+20;;
				F32 z = row * 500.0f -500.0f - cos(GETMSTIME()*(col-row+2)*0.009f)*200.0f+10;
				F32 r = 1;
				F32 g = 1.0f-(row/3.0f);
				F32 b = col/(lights.size()/3.0f);
	
				lights[row*10+col]->setLightProperties(LIGHT_POSITION,vec4<F32>(x,y,z,1));
				lights[row*10+col]->setLightProperties(LIGHT_DIFFUSE,vec4<F32>(r,g,b,1));
			}

		_eventTimers[0] += updateLights;
	}

}

I8 j = 1;
F32 i = 0;
bool _switch = false;
void CubeScene::preRender() {
	GFX_DEVICE.setRenderStage(DEFERRED_STAGE);
	LightManager::LightMap& lights = LightManager::getInstance().getLights();

	if(i >= 360) _switch = true;
	else if(i <= 0) _switch = false;

	if(!_switch) i += 0.1f;
	else i -= 0.1f;

	i >= 180 ? j = -1 : j = 1;

	SceneGraphNode* cutia1 = _sceneGraph->findNode("Cutia1");
	SceneGraphNode* hellotext = _sceneGraph->findNode("HelloText");
	SceneGraphNode* bila = _sceneGraph->findNode("Bila");
	SceneGraphNode* dwarf = _sceneGraph->findNode("dwarf");
	cutia1->getTransform()->rotateEuler(vec3<F32>(0.3f*i, 0.6f*i,0));
	hellotext->getTransform()->rotate(vec3<F32>(0.6f,0.2f,0.4f),i);
	bila->getTransform()->translateY(j*0.25f);
	dwarf->getTransform()->rotate(vec3<F32>(0,1,0),i);

	U8 index = 0;
	F32* pixels = (F32*)_lightTexture->Begin();
	for(U8 row=0; row<3; row++){
		for(U8 col=0; col < lights.size()/3; col++){
			U8 i = row*10+col;
			//Light Position																										
			pixels[index + 0]  = lights[i]->getPosition().x;		
			pixels[index + 1]  = lights[i]->getPosition().y;		
			pixels[index + 2]  = lights[i]->getPosition().z;		
			//Light Color
			pixels[index + 3]  = lights[i]->getDiffuseColor().r;					
			pixels[index + 4]  = lights[i]->getDiffuseColor().g;											
			pixels[index + 5]  = lights[i]->getDiffuseColor().b;
			index += 6;
		}
	}
	_lightTexture->End();
	//Pass 1
	//Draw the geometry, saving parameters into the buffer
	_deferredBuffer->Begin();
		_sceneGraph->render();
	_deferredBuffer->End();
} 

void CubeScene::processInput(){

	if(_angleLR) _camera->RotateX(_angleLR * Framerate::getInstance().getSpeedfactor());
	if(_angleUD) _camera->RotateY(_angleUD * Framerate::getInstance().getSpeedfactor());
	if(_moveFB)  _camera->MoveForward(_moveFB * (Framerate::getInstance().getSpeedfactor()/5));
	if(_moveLR)	 _camera->MoveStrafe(_moveLR * (Framerate::getInstance().getSpeedfactor()/5));

}

bool CubeScene::preLoad(){
	_GFX.setDeferredRendering(true);
	return Scene::preLoad();
}

bool CubeScene::load(const std::string& name){	

	bool state = loadResources(true);	
	if(state){
		state = loadEvents(true);
	}
	return state;
}

bool CubeScene::loadResources(bool continueOnErrors){

	_deferredBuffer = _GFX.newFBO();
	_lightTexture = _GFX.newPBO();
	//30 lights? >:)
	
	for(U8 row=0; row<3; row++)
		for(U8 col=0; col < 10; col++){
			U8 lightId = (U8)(row*10+col);
			std::stringstream ss; ss << (U32)lightId;
			ResourceDescriptor tempLight("Light Deferred " + ss.str());
			tempLight.setId(lightId);
			Light* light = CreateResource<Light>(tempLight);
			light->setDrawImpostor(true);
			light->setRadius(30);
			light->setCastShadows(false); //ToDo: Shadows are ... for another time -Ionut
			_sceneGraph->getRoot()->addNode(light);
			addLight(light);
	}
	ResourceDescriptor deferred("DeferredShadingPass2");
	_deferredShader = CreateResource<ShaderProgram>(deferred);

	_deferredBuffer->Create(FBO_2D_DEFERRED,_cachedResolution.width,_cachedResolution.height);
	_lightTexture->Create(2,LightManager::getInstance().getLights().size());

	F32 width = _cachedResolution.width;
	F32 height = _cachedResolution.height;
	ResourceDescriptor mrt("MRT RenderQuad");
	mrt.setFlag(true); //no default material
	_renderQuad = CreateResource<Quad3D>(mrt);
	assert(_renderQuad);
	//In 2D mode, Quad's are flipped!!!!
	_renderQuad->setDimensions(vec4<F32>(0,0,width,height));

	ResourceDescriptor mrt2("MRT RenderQuad2");
	mrt2.setFlag(true); //no default material
	_renderQuad2 = CreateResource<Quad3D>(mrt2);
	assert(_renderQuad2);
	_renderQuad2->setDimensions(vec4<F32>(width/2,0,width,height/2));

	ResourceDescriptor mrt3("MRT RenderQuad3");
	mrt3.setFlag(true); //no default material
	_renderQuad3 = CreateResource<Quad3D>(mrt3);
	assert(_renderQuad3);
	_renderQuad3->setCorner(Quad3D::TOP_LEFT, vec3<F32>(0, height, 0));
	_renderQuad3->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(width/2, height, 0));
	_renderQuad3->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(0,height/2,0));
	_renderQuad3->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(width/2, height/2, 0));

	ResourceDescriptor mrt4("MRT RenderQuad4");
	mrt4.setFlag(true); //no default material
	_renderQuad4 = CreateResource<Quad3D>(mrt4);
	assert(_renderQuad4);
	_renderQuad4->setCorner(Quad3D::TOP_LEFT, vec3<F32>(width/2, height, 0));
	_renderQuad4->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(width, height, 0));
	_renderQuad4->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(width/2,height/2,0));
	_renderQuad4->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(width, height/2, 0));

	_eventTimers.push_back(0.0f);
	GUI& gui = GUI::getInstance();
	gui.addText("FinalImage",           //Unique ID
		        vec3<F32>(60,60,0),          //Position
				BITMAP_9_BY_15,		    //Font
				vec3<F32>(0.0f,0.2f, 1.0f),  //Color
				"FINAL IMAGE",0);    //Text and arguments
	gui.addText("LightTexture",           //Unique ID
		        vec3<F32>(60+width/2,60,0),          //Position
				BITMAP_9_BY_15,		    //Font
				vec3<F32>(0.0f,0.2f, 1.0f),  //Color
				"LIGHT TEXTURE",0);    //Text and arguments
	gui.addText("PositionData",           //Unique ID
				vec3<F32>(60,60+height/2,0),  //Position
				BITMAP_9_BY_15,		    //Font
				vec3<F32>(0.0f,0.2f, 1.0f),  //Color
				"POSITION DATA",0);    //Text and arguments
	gui.addText("NormalData",           //Unique ID
	        vec3<F32>(60+width/2,60+height/2,0),          //Position
			BITMAP_9_BY_15,		    //Font
			vec3<F32>(0.0f,0.2f, 1.0f),  //Color
			"NORMAL DATA",0);    //Text and arguments
	return true;
}

bool CubeScene::unload(){

	PRINT_FN(Locale::get("DEFERRED_RT_DELETE"));
	RemoveResource(_renderQuad);
	return Scene::unload();
}

void CubeScene::onKeyDown(const OIS::KeyEvent& key)
{
	Scene::onKeyDown(key);
	switch(key.key)
	{
		case OIS::KC_W:
			_moveFB = 0.75f;
			break;
		case OIS::KC_A:
			_moveLR = 0.75f;
			break;
		case OIS::KC_S:
			_moveFB = -0.75f;
			break;
		case OIS::KC_D:
			_moveLR = -0.75f;
			break;
		case OIS::KC_T:{
			F32 width = _cachedResolution.width;
			F32 height = _cachedResolution.height;
			_showTextures = !_showTextures;
			if(!_showTextures){
				_renderQuad->setCorner(Quad3D::TOP_LEFT, vec3<F32>(0, height, 0));
				_renderQuad->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(width, height, 0));
				_renderQuad->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(0,0,0));
				_renderQuad->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(width, 0, 0));
			}else{
				_renderQuad->setCorner(Quad3D::TOP_LEFT, vec3<F32>(0, height/2, 0));
				_renderQuad->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(width/2, height/2, 0));
				_renderQuad->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(0,0,0));
				_renderQuad->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(width/2, 0, 0));
			}
			}break;
		default:
			break;
	}
}

void CubeScene::onKeyUp(const OIS::KeyEvent& key)
{
	Scene::onKeyUp(key);
	switch(key.key)
	{
		case OIS::KC_W:
		case OIS::KC_S:
			_moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			_moveLR = 0;
			break;
		default:
			break;
	}

}
