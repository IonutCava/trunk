#include "GFXDevice.h"
#include "Rendering/common.h"
#include "Geometry/Object3D.h"
#include "Importer/DVDConverter.h"

void GFXDevice::setApi(GraphicsAPI api)
{
	switch(api)
	{
	case 0: //OpenGL 1.0
	case 1: //OpenGL 1.2
	case 2: //OpenGL 2.0
	case 3: //OpenGL 2.1
	case 4: //OpenGL 2.2
	case 5: //OpenGL 3.0
	case 6: //OpenGL 3.2
	default:
		_api = GL_API::getInstance();
		break;

	case 7: //DX 8
	case 8: //DX 9
	case 9: //DX 10
		_api = DX_API::getInstance();
		break;
	//Ionut: OpenGL 4.0 and DX 11 in another life maybe :)
	};
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
		(_iter->second)->onDraw();
		if(!(_iter->second)->getVisibility()) continue;
		switch((_iter->second)->getType())
		{
			case BOX_3D:
				drawBox3D((Box3D*)(_iter->second));
				break;
			case SPHERE_3D:
				drawSphere3D((Sphere3D*)(_iter->second));
				break;
			case QUAD_3D:
				drawQuad3D((Quad3D*)(_iter->second));
				break;
			case TEXT_3D:
				drawText3D((Text3D*)(_iter->second));
				break;
			default:
				break;
		};
	}
}
void GFXDevice::renderElements(vector<DVDFile*>& geometryArray)
{
	vector<DVDFile*>::iterator _iter;
	for(_iter = geometryArray.begin();  _iter != geometryArray.end(); ++ _iter)
	{
		renderModel(*_iter);
	}
}

void GFXDevice::renderElements(unordered_map<string,DVDFile*>& geometryArray)
{
	unordered_map<string,DVDFile*>::iterator _iter;
	for(_iter = geometryArray.begin();  _iter != geometryArray.end(); ++ _iter)
	{
		renderModel(_iter->second);
	}
}