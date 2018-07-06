#include "GFXDevice.h"
#include "Rendering/common.h"
#include "Geometry/Object3D.h"
#include "Geometry/Object3DFlyWeight.h"
#include "Importer/DVDConverter.h"
#include "Managers/TextureManager.h"

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

void GFXDevice::renderElements(vector<Object3DFlyWeight*>& geometryArray)
{
	vector<Object3DFlyWeight*>::iterator _iter;
	for(_iter = geometryArray.begin();  _iter != geometryArray.end(); ++ _iter)
	{
		DVDFile *temp = dynamic_cast<DVDFile*>((*_iter)->getObject());
		temp->getTransform()->setPosition((*_iter)->getTransform()->getPosition());
		temp->getTransform()->scale((*_iter)->getTransform()->getScale());
		temp->getTransform()->rotateQuaternion((*_iter)->getTransform()->getOrientation());
		renderModel(temp);
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

void GFXDevice::toggleWireframe(bool state)
{
	_wireframeMode = !_wireframeMode;
	_api.toggleWireframe(_wireframeMode);
}

// takes a screen shot and saves it to a TGA image
void GFXDevice::Screenshot(char *filename, int xmin,int ymin, int xmax, int ymax)
{
	// compute width and heidth of the image
	int w = xmax - xmin;
	int h = ymax - ymin;

	// allocate memory for the pixels
	U8 *imageData = new U8[w * h * 4];

	// read the pixels from the frame buffer
	//glReadPixels(xmin,ymin,xmax,ymax,GL_RGBA,GL_UNSIGNED_BYTE, (void*)imageData);

	// save the image 
	TextureManager::getInstance().SaveSeries(filename,w,h,32,imageData);
	delete[] imageData;
}