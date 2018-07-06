#include "Sky.h"

#include "Utility/Headers/ParamHandler.h"
#include "Managers/ResourceManager.h"
#include "Rendering/Camera.h"
#include "Hardware/Video/GFXDevice.h"

Sky::Sky()
{
	_init = false;
   	string location = ParamHandler::getInstance().getParam<string>("assetsLocation")+"/misc_images/";
	_sky = new Sphere3D(1,4);
	_sun = new Sphere3D(0.1f, 16);

	_skybox =  ResourceManager::getInstance().LoadResource<TextureCubemap>(
				location+"skybox_2.jpg "+
				location+"skybox_1.jpg "+
				location+"skybox_5.jpg "+
				location+"skybox_6.jpg "+ 
				location+"skybox_3.jpg "+
				location+"skybox_4.jpg");
	_skyShader = ResourceManager::getInstance().LoadResource<Shader>("sky");
	assert(_skyShader);
	_init = true;
}

Sky::~Sky()
{
	delete _sky; _sky = NULL;
	delete _sun; _sun = NULL;
	delete _skyShader; _skyShader = NULL;
	delete _skybox; _skybox = NULL;
}

void Sky::draw() const
{
	if (_drawSky && _drawSun) drawSkyAndSun();
	if (_drawSky && !_drawSun) drawSky();
	if (!_drawSky && _drawSun) drawSun();
}

void Sky::setParams(const vec3& eyePos, const vec3& sunVect, bool invert, bool drawSun, bool drawSky) 
{
	_eyePos = eyePos;	_sunVect = sunVect;
	_invert = invert;	_drawSun = drawSun;
	_drawSky = drawSky;
}

void Sky::drawSkyAndSun() const
{
	if(!_init) return;
	GFXDevice::getInstance().enable_MODELVIEW();
	_sky->getPosition() = vec3(_eyePos.x,_eyePos.y,_eyePos.z);

	if(_invert)
		_sky->getScale() = vec3(1.0f, -1.0f, 1.0f);
	else
		_sky->getScale() = vec3(1.0f, 1.0f, 1.0f);


	glPushAttrib(GL_ENABLE_BIT);

	_skybox->Bind(0);
	_skyShader->bind();
	{
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", true);
		_skyShader->Uniform("sun_vector", _sunVect);
		_skyShader->Uniform("view_vector", Camera::getInstance().getViewDir());

		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);
		_sky->draw();
	}
	_skyShader->unbind();
	_skybox->Unbind(0);
	glPopAttrib();
	glClear(GL_DEPTH_BUFFER_BIT);
}

void Sky::drawSky() const
{

	if(!_init) return;
	_sky->getPosition() = vec3(_eyePos.x,_eyePos.y,_eyePos.z);
	if(_invert)
		_sky->getScale() = vec3(1.0f, -1.0f, 1.0f);
	else
		_sky->getScale() = vec3(1.0f,  1.0f, 1.0f);

	glPushAttrib(GL_ENABLE_BIT);

	_skybox->Bind(0);
	_skyShader->bind();
	{
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", false);
		_skyShader->Uniform("view_vector", Camera::getInstance().getViewDir());

		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);
		_sky->draw();
	}
	_skyShader->unbind();
	_skybox->Unbind(0);
	glPopAttrib();
	glClear(GL_DEPTH_BUFFER_BIT);

}

void Sky::drawSun() const
{
	if(!_init) return;
	
	vec4 color;
	glGetLightfv(GL_LIGHT0, GL_DIFFUSE, color);
	_sun->getColor() = vec3(color.r, color.g, color.b);
	_sun->getPosition() = vec3(_eyePos.x,_eyePos.y,_eyePos.z);

	GFXDevice::getInstance().enable_MODELVIEW();
	GFXDevice::getInstance().pushMatrix();
	GFXDevice::getInstance().translate(vec3(-_sunVect.x, -_sunVect.y, -_sunVect.z));
	glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		_sun->draw();
		glPopAttrib();
	glClear(GL_DEPTH_BUFFER_BIT);
	GFXDevice::getInstance().popMatrix();

}


