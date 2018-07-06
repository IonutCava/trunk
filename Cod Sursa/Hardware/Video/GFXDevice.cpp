#include "GFXDevice.h"
#include "Rendering/common.h"

void GFXDevice::setApi(GraphicsAPI api)
{
	if(api <= 6)
	{
		_api = GL_API::getInstance();
	}
	else
	{
		_api = DX_API::getInstance();
	}
	_api.setId(api);
}

void GFXDevice::resizeWindow(U32 w, U32 h)
{
	Engine::getInstance().setWindowWidth(w);
    Engine::getInstance().setWindowHeight(h);
	_api.resizeWindow(w,h);
}

void GFXDevice::renderElements(unordered_map<string,Object3D*>& primitiveArray)
{

	unordered_map<string,Object3D*>::iterator _iter;
	for(_iter = primitiveArray.begin();  _iter != primitiveArray.end();  _iter++)
	{
		if((_iter->first).compare("Box3D") == 0)	      drawBox3D((Box3D*)(_iter->second));
		else if((_iter->first).compare("Quad3D") == 0)	  drawQuad3D((Quad3D*)(_iter->second));
		else if((_iter->first).compare("Sphere3D") == 0)  drawSphere3D((Sphere3D*)(_iter->second));
		else if((_iter->first).compare("Text3D") == 0)    drawText3D((Text3D*)(_iter->second));
		else continue;
	}
}

void GFXDevice::renderElements(unordered_map<string,DVDFile*>& geometryArray)
{
	unordered_map<string,DVDFile*>::iterator _iter;
	for(_iter = geometryArray.begin();  _iter != geometryArray.end(); ++ _iter)
		renderModel(_iter->second);
}