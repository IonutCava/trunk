#include "Water.h"
#include "Managers/ResourceManager.h"
#include "Rendering/common.h"
#include "Hardware/Video/GFXDevice.h"

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

void WaterPlane::draw(FrameBufferObject& _fbo)
{
	RenderState s(false,true,true,true);
	GFXDevice::getInstance().setRenderState(s);


	_fbo.Bind(0);
	_texture->Bind(1);
	_shader->bind();
		_shader->UniformTexture("texWaterReflection", 0);
		_shader->UniformTexture("texWaterNoiseNM", 1);
		_shader->Uniform("time", GETTIME());
		_shader->Uniform("water_shininess",_shininess );
		_shader->Uniform("noise_tile", _noiseTile );
		_shader->Uniform("noise_factor", _noiseFactor);
		_shader->Uniform("win_width",  Engine::getInstance().getWindowWidth());
		_shader->Uniform("win_height", Engine::getInstance().getWindowHeight());
		_shader->Uniform("bbox_min",_plane->_tl);
		_shader->Uniform("bbox_max",_plane->_br);

	GFXDevice::getInstance().drawQuad3D(_plane);

	_shader->unbind();
	_texture->Unbind(1);
	_fbo.Unbind(0);
}