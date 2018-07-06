#include "GFXDevice.h"

void GFXDevice::setApi(GraphicsAPI api)
{
	if(api <= 2)
	{
		_api = GL_API::getInstance();
	}
	else
	{
		_api = DX_API::getInstance();
	}
	_api.setId(api);
}
