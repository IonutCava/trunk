#include "Water.h"
#include "Managers/ResourceManager.h"
#include "Managers/SceneManager.h"
#include "Rendering/common.h"
#include "Hardware/Video/GFXDevice.h"
using namespace std;

WaterPlane::WaterPlane() : _texture(ResourceManager::getInstance().LoadResource<Texture2D>(ParamHandler::getInstance().getParam<string>("assetsLocation")+"/misc_images/terrain_water_NM.jpg")),
						   _shader(ResourceManager::getInstance().LoadResource<Shader>("water"))

{
	_shininess =  50.0f;
	_noiseTile = 10.0f;
	_noiseFactor = 0.1f;
	_plane = new Quad3D(vec3(-1,0,1),vec3(1,0,1),vec3(-1,0,-1),vec3(1,0,-1));
}

bool WaterPlane::unload()
{
	bool state = false;
	delete _plane;
	state = _shader->unload();
	state = _texture->unload();
	return state;
}

void WaterPlane::draw(FrameBufferObject* fbo[])
{
	TerrainManager* terMgr = SceneManager::getInstance().getTerrainManager();
	RenderState s(false,true,true,true);
	GFXDevice::getInstance().setRenderState(s);

	vec3 fogColor(0.7f, 0.7f, 0.9f);
	_texture->SetMatrix(0,terMgr->getSunModelviewProjMatrix());

	_shader->bind();
		fbo[0]->Bind(0);
		_shader->UniformTexture("texWaterReflection", 0);

		_texture->Bind(1);
		_shader->UniformTexture("texWaterNoiseNM", 1);

		fbo[1]->Bind(2);
		fbo[2]->Bind(3);
		_shader->UniformTexture("texDepthMapFromLight0", 2);
		_shader->UniformTexture("texDepthMapFromLight1", 3);
		_shader->Uniform("depth_map_size", 512);

		_shader->Uniform("time", GETTIME());
		_shader->Uniform("water_shininess",_shininess );
		_shader->Uniform("noise_tile", _noiseTile );
		_shader->Uniform("noise_factor", _noiseFactor);
		_shader->Uniform("win_width",  (I32)Engine::getInstance().getWindowDimensions().width);
		_shader->Uniform("win_height", (I32)Engine::getInstance().getWindowDimensions().height);
		_shader->Uniform("bbox_min",_plane->_tl);
		_shader->Uniform("bbox_max",_plane->_br);
		
		_shader->Uniform("fog_color",fogColor);

		GFXDevice::getInstance().drawQuad3D(_plane);

		fbo[2]->Unbind(3);
		fbo[1]->Unbind(2);
		_texture->Unbind(1);
		fbo[0]->Unbind(0);
	_shader->unbind();

	_texture->RestoreMatrix(0);
}