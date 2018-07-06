#include "GFXDevice.h"
#include "Rendering/common.h"
#include "Geometry/Object3D.h"
#include "Geometry/Object3DFlyWeight.h"
#include "Importer/DVDConverter.h"
#include "Managers/TextureManager.h"
#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Quad3D.h"
#include "Geometry/Predefined/Text3D.h"
using namespace std;

void GFXDevice::setApi(RenderAPI api)
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

void GFXDevice::resizeWindow(U16 w, U16 h)
{
	Engine::getInstance().setWindowWidth(w);
    Engine::getInstance().setWindowHeight(h);
	_api.resizeWindow(w,h);
}

void GFXDevice::renderElements(tr1::unordered_map<string,Object3D*>& primitiveArray)
{
	tr1::unordered_map<string,Object3D*>::iterator _iter;
	for(_iter = primitiveArray.begin();  _iter != primitiveArray.end();  _iter++)
	{
		if(!(_iter->second)->getVisibility()) continue;
		switch((_iter->second)->getType())
		{
			case BOX_3D:
				drawBox3D(dynamic_cast<Box3D*>(_iter->second));
				break;
			case SPHERE_3D:
				drawSphere3D(dynamic_cast<Sphere3D*>(_iter->second));
				break;
			case QUAD_3D:
				drawQuad3D(dynamic_cast<Quad3D*>(_iter->second));
				break;
			case TEXT_3D:
				drawText3D(dynamic_cast<Text3D*>(_iter->second));
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
		Mesh *temp = dynamic_cast<Mesh*>((*_iter)->getObject());
		temp->getTransform()->setPosition((*_iter)->getTransform()->getPosition());
		temp->getTransform()->scale((*_iter)->getTransform()->getScale());
		temp->getTransform()->rotateQuaternion((*_iter)->getTransform()->getOrientation());
		renderModel(temp);
	}
}

void GFXDevice::renderElements(vector<Mesh*>& geometryArray)
{
	vector<Mesh*>::iterator _iter;
	for(_iter = geometryArray.begin();  _iter != geometryArray.end(); ++ _iter)
	{
		renderModel(*_iter);
	}
}

void GFXDevice::renderElements(tr1::unordered_map<string,Mesh*>& geometryArray)
{
	tr1::unordered_map<string,Mesh*>::iterator _iter;
	for(_iter = geometryArray.begin();  _iter != geometryArray.end(); ++ _iter)
	{
		renderModel(_iter->second);
	}
}

void GFXDevice::renderModel(Mesh* const model)
{
	if(!model) return;
	if(model->clean()){
		ResourceManager::getInstance().remove(model);
		return;
	}
	model->onDraw(); //Update BB, shaders etc.
	if(!model->isVisible()) return;
	_api.renderModel(model);
}

void GFXDevice::toggleWireframe(bool state)
{
	_wireframeMode = !_wireframeMode;
	_api.toggleWireframe(_wireframeMode);
}

// takes a screen shot and saves it to a TGA image
void GFXDevice::Screenshot(char *filename, U16 xmin,U16 ymin, U16 xmax, U16 ymax)
{
	// compute width and heidth of the image
	U16 w = xmax - xmin;
	U16 h = ymax - ymin;

	// allocate memory for the pixels
	U8 *imageData = new U8[w * h * 4];

	// read the pixels from the frame buffer
	//glReadPixels(xmin,ymin,xmax,ymax,GL_RGBA,GL_UNSIGNED_BYTE, (void*)imageData);

	// save the image 
	TextureManager::getInstance().SaveSeries(filename,w,h,32,imageData);
	delete[] imageData;
	imageData = NULL;
}