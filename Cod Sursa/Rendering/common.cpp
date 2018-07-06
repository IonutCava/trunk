#include "Hardware/Video/OpenGL/glResources.h" //ToDo: Remove this from here -Ionut

#include "common.h"

#include "GUI/GLUIManager.h"
#include "Utility/Headers/Guardian.h"
#include "Managers/TextureManager.h"
#include "Managers/ResourceManager.h"
#include "Managers/TerrainManager.h"
#include "Managers/SceneManager.h"
#include "Terrain/Sky.h"
#include "Camera.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Frustum.h"
#include "Utility/Headers/ParamHandler.h"
#include "GUI/GUI.h"
#include "Framerate.h"

#include "Utility/Headers/Event.h"
#include "Geometry/Predefined/Text3D.h"

void Engine::Idle()
{
	SceneManager::getInstance().clean();
	GFXDevice::getInstance().idle();
}

Engine::Engine() : 
	_GFX(GFXDevice::getInstance()),
    _px(PhysX::getInstance()),
	_scene(SceneManager::getInstance()),
	_camera(Camera::getInstance()),
	_gui(GUI::getInstance())
{
	//BEGIN CONSTRUCTOR
	 time = 0;
	 timebase = 0;
	 angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;
	 m_bWireframe = false;
 
	 tip = 0, turn = 0;
	 mainWindowId = -1;
	 //END CONSTRUCTOR
}


void Engine::DrawSceneStatic()
{
	GFXDevice::getInstance().clearBuffers(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	Engine::getInstance().DrawScene();
	Framerate::getInstance().SetSpeedFactor();
	GFXDevice::getInstance().swapBuffers();
}

void Engine::DrawScene()
{

	RefreshMetrics();
	
	_GFX.enable_MODELVIEW();
	_GFX.loadIdentityMatrix();
	_camera.RenderLookAt();
	
	_px.GetPhysicsResults();
	if(_px.getScene() != NULL)
	{
		if (_px.getWireFrameData())
			_px.getDebugRenderer()->renderData(*(_px.getScene()->getDebugRenderable()));
	}
	_px.StartPhysics();

	_scene.preRender();
	_scene.render();
	_scene.processInput();
	_scene.processEvents(abs(GETTIME()));
}

void Engine::ToggleWireframeRendering()
{
	m_bWireframe = !m_bWireframe;
	if(m_bWireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


void Engine::RefreshMetrics()
{
	time=(int)(GETTIME()*1000);
	if (time - timebase > 1000) timebase = time;		
}

void Engine::Initialize(int w, int h)
{    
	ResourceManager& res = ResourceManager::getInstance();
	width = w;	height = h;
	_GFX.setApi(OpenGL32);
	_GFX.initHardware();
	ZPR::Init();
    glutDisplayFunc(DrawSceneStatic);
	glutIdleFunc(Idle);
	
	_camera.setEye(vec3(200,150,200));

	F32 fogColor[4] = {0.5, 0.5, 0.5, 1.0}; 
	//_GFX.enableFog(0.3f,fogColor);
}

void Engine::Quit()
{
	cout << "Destroying Terrain ...\n";
	SceneManager::getInstance().getTerrainManager()->destroy();
	cout << "Closing the rendering engine ...\n";
	_GFX.closeRenderingApi();
}


// takes a screen shot and saves it to a TGA image
void Engine::Screenshot(char *filename, int xmin,int ymin, int xmax, int ymax)
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