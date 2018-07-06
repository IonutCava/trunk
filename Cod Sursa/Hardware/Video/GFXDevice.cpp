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