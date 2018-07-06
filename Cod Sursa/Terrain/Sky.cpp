#include "Sky.h"

#include "Managers/ResourceManager.h"
#include "Rendering/Camera.h"
#include "Hardware/Video/GFXDevice.h"

Sky::Sky()
{
	_init = false;

	_skybox = ( ResourceManager::getInstance().LoadResource<TextureCubemap>(
				"skybox_2.jpg skybox_1.jpg skybox_5.jpg skybox_6.jpg skybox_3.jpg skybox_4.jpg"))->getHandle();
	_skyShader = ResourceManager::getInstance().LoadResource<Shader>("sky");
	assert(_skyShader);
	_init = true;
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
	GFXDevice::getInstance().pushMatrix();
	GFXDevice::getInstance().translate(_eyePos.x, _eyePos.y, _eyePos.z);
	if(_invert) GFXDevice::getInstance().scale(1.0f, -1.0f, 1.0f);

	glPushAttrib(GL_ENABLE_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _skybox);
	_skyShader->bind();
	{
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", true);
		_skyShader->Uniform("sun_vector", _sunVect);
		_skyShader->Uniform("view_vector", Camera::getInstance().getViewDir());

		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);
		glutSolidSphere (0.25, 4, 4);
	}
	_skyShader->unbind();

	glPopAttrib();

	GFXDevice::getInstance().popMatrix();

	glClear(GL_DEPTH_BUFFER_BIT);
}

void Sky::drawSky() const
{

	if(!_init) return;

	GFXDevice::getInstance().enable_MODELVIEW();
	GFXDevice::getInstance().pushMatrix();
	GFXDevice::getInstance().translate(_eyePos.x, _eyePos.y, _eyePos.z);
	if(_invert)	GFXDevice::getInstance().scale(1.0f, -1.0f, 1.0f);

	glPushAttrib(GL_ENABLE_BIT);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _skybox);
	_skyShader->bind();
	{
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", false);
		_skyShader->Uniform("view_vector", Camera::getInstance().getViewDir());
		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);
		glutSolidSphere (1.0, 4, 4);
	}
	_skyShader->unbind();
	glDisable(GL_TEXTURE_CUBE_MAP);

	glPopAttrib();

	GFXDevice::getInstance().popMatrix();

	glClear(GL_DEPTH_BUFFER_BIT);

}

void Sky::drawSun() const
{
	if(!_init) return;

	vec4 color;
	glGetLightfv(GL_LIGHT0, GL_DIFFUSE, color);

	GFXDevice::getInstance().enable_MODELVIEW();
	GFXDevice::getInstance().pushMatrix();
	GFXDevice::getInstance().translate(_eyePos.x, _eyePos.y, _eyePos.z);
	GFXDevice::getInstance().translate(-_sunVect.x, -_sunVect.y, -_sunVect.z);

	glPushAttrib(GL_ENABLE_BIT);

	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	GFXDevice::getInstance().setColor(color.r, color.g, color.b);
	glutSolidSphere(0.1, 16, 16);

	glPopAttrib();

	glClear(GL_DEPTH_BUFFER_BIT);
	GFXDevice::getInstance().popMatrix();

	delete color;
}


