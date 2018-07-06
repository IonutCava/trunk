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

void GFXDevice::renderElements(tr1::unordered_map<string,Object3D*>& geometryArray)
{
	tr1::unordered_map<string,Object3D*>::iterator _iter;
	for(_iter = geometryArray.begin();  _iter != geometryArray.end();  _iter++){
		renderModel(_iter->second);
	}
}

void GFXDevice::renderElements(vector<Object3DFlyWeight*>& geometryArray)
{
	vector<Object3DFlyWeight*>::iterator _iter;
	Mesh *temp = NULL;
	for(_iter = geometryArray.begin();  _iter != geometryArray.end(); ++ _iter)
	{
		temp = dynamic_cast<Mesh*>((*_iter)->getObject());
		temp->getTransform()->setPosition((*_iter)->getTransform()->getPosition());
		temp->getTransform()->scale((*_iter)->getTransform()->getScale());
		temp->getTransform()->rotateQuaternion((*_iter)->getTransform()->getOrientation());
		renderModel(temp);
	}
}

void GFXDevice::renderModel(Object3D* const model)
{
		if(!model) return;
		if(!model->getVisibility()) return;
		if(model->shouldDelete()){
			ResourceManager::getInstance().remove(model);
			return;;
		}
		model->onDraw();
		if(!model->getChildren().empty()){
			for(U8 i = 0; i < model->getChildren().size(); i++){
				model->getChildren()[i]->setParentMatrix(model->getParentMatrix());
				renderModel(model->getChildren()[i]);
			}
		}
		switch(model->getType())
		{
			case BOX_3D:
				drawBox3D(dynamic_cast<Box3D*>(model));
				break;
			case SPHERE_3D:
				drawSphere3D(dynamic_cast<Sphere3D*>(model));
				break;
			case QUAD_3D:
				drawQuad3D(dynamic_cast<Quad3D*>(model));
				break;
			case TEXT_3D:
				drawText3D(dynamic_cast<Text3D*>(model));
				break;
			case MESH:
				_api.renderModel(dynamic_cast<Mesh*>(model));
				break;
			default:
				break;
		};
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