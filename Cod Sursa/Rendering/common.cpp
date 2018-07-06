#include "common.h"

#include "GUI/GLUIManager.h"
#include "Utility/Headers/Guardian.h"
#include "Managers/TextureManager.h"
#include "Managers/ResourceManager.h"
#include "Managers/SceneManager.h"
#include "Terrain/Sky.h"
#include "Camera.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Frustum.h"
#include "Utility/Headers/ParamHandler.h"
#include "GUI/GUI.h"
#include "Framerate.h"

void Engine::Idle()
{
	glutSetWindow(Engine::getInstance().getMainWindowId()); 
	glutPostRedisplay();
}

Engine::Engine() : 
	_GFX(GFXDevice::getInstance())
	
{
	//BEGIN CONSTRUCTOR
	 time = 0;
	 timebase = 0;
	 angleLR=0.0f,angleUD=0.0f,moveFB=0.0f;
	 firstPersonCamera = true;
	 m_bWireframe = false;
 
	 tip = 0, turn = 0;
	 mainWindowId = -1;
	 EpochTime = 0;
	 update_time = 0.0f;
	 update_time2 = 0.0f;
	 update_time3 = 0.0f;

	 //END CONSTRUCTOR
}


void Engine::DrawSceneStatic()
{
	Engine::getInstance().DrawScene();
	Framerate::getInstance().SetSpeedFactor();
	GFXDevice::getInstance().swapBuffers();
}

void Engine::DrawScene()
{

	PhysX &px = PhysX::getInstance();
	RefreshMetrics();

	_GFX.clearBuffers(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	_GFX.enable_MODELVIEW();
	_GFX.loadIdentityMatrix();
	Camera::getInstance().RenderLookAt();
	
	px.GetPhysicsResults();
	ProcessRenderingInput();
	px.StartPhysics();

	SceneManager::getInstance().processEvents(abs(GETTIME()));
	SceneManager::getInstance().preRender();
	SceneManager::getInstance().render();

}

void Engine::ToggleWireframeRendering()
{
	m_bWireframe = !m_bWireframe;
	if(m_bWireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Engine::ProcessRenderingInput()
{
	PhysX &px = PhysX::getInstance();

	SceneManager::getInstance().processInput();

	if(px.getScene() != NULL)
	{
		if (px.getWireFrameData())
			px.getDebugRenderer()->renderData(*(px.getScene()->getDebugRenderable()));
	}
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
	ZPR::SelectionFunc(DrawSceneStatic);    /* Selection mode draw function */
	ZPR::PickFunc(Pick);              /* Pick event client callback   */
    glutDisplayFunc(DrawSceneStatic);
	glutIdleFunc(Idle);
	

	Camera::getInstance().setEye(vec3(200,300,200));
	GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec3(60,60,0),          //Position
							   GLUT_BITMAP_8_BY_13,    //Font
							   vec3(0.0f,0.2f, 1.0f),  //Color
							   "HELLO! FPS: %s",0);    //Text and arguments
	GUI::getInstance().addText("timeDisplay",
								vec3(60,70,0),
								GLUT_BITMAP_8_BY_13,
								vec3(0.6f,0.2f,0.2f),
								"Elapsed time: %5.0f",GETTIME());
	F32 fogColor[4] = {0.5, 0.5, 0.5, 1.0}; 
	_GFX.enableFog(0.3f,fogColor);

	/*int ev = 1;
	char* ev2 = "aha";
	Event *_event = new Event(3,true,100,&printHello,string("Hello1"),TYPE_STRING);
	Event *_event2 = new Event(2000,true,200,&printHello,ev,TYPE_INTEGER);
	Event *_event3 = new Event(300,true,10,&printHello,ev2,TYPE_CHAR);
	Event *_event4 = new Event(200,true,20,&printHello,1.0f,TYPE_FLOAT);
	Text3D* t = new Text3D;
	t->getName() = "test text";
	cout << "Test: " << t->getName() << endl;
	*/
}

void Engine::Quit()
{
	cout << "Destroying Terrain ...\n";
	TerrainManager::getInstance().destroy();
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
	UBYTE *imageData = new UBYTE[w * h * 4];

// read the pixels from the frame buffer
	glReadPixels(xmin,ymin,xmax,ymax,GL_RGBA,GL_UNSIGNED_BYTE, (void*)imageData);

// save the image 
	TextureManager::getInstance().SaveSeries(filename,w,h,32,imageData);
  delete imageData;
}