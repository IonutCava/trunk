#include "common.h"

#include "GUI/GLUIManager.h"
#include "Utility/Headers/Guardian.h"
#include "Managers/TextureManager.h"
#include "Managers/ResourceManager.h"
#include "Managers/SceneManager.h"
#include "Managers/TerrainManager.h"
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
	glutSetWindow(Engine::getInstance().getMainWindowId()); 
	glutPostRedisplay();
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
	
	
	ProcessRenderingInput();
	
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

	

}


void Engine::RefreshMetrics()
{
	time=(int)(GETTIME()*1000);
	if (time - timebase > 1000) timebase = time;		
}

void printHello(boost::any data, CallbackParam type)
{
	/*if(type == TYPE_CHAR)
		cout << "Hello from: " << boost::any_cast<char*>(data) << endl;
	else if(type == TYPE_STRING)
		cout << "Hello from: " << boost::any_cast<string>(data) << endl;
	else if(type == TYPE_FLOAT)
		cout << "Hello from: " << boost::any_cast<F32>(data) << endl;
	else if(type == TYPE_INTEGER)
		cout << "Hello from: " << boost::any_cast<int>(data) << endl;
	else*/
	cout << GETTIME() << "Hello " << boost::this_thread::get_id() << endl;
}

/*Test Button function*/
void TheButtonCallback()
{
	cout << "I have been called" << endl;
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
	

	_camera.setEye(vec3(200,150,200));
	_gui.addText("fpsDisplay",           //Unique ID
		                       vec3(60,60,0),          //Position
							   GLUT_BITMAP_8_BY_13,    //Font
							   vec3(0.0f,0.2f, 1.0f),  //Color
							   "HELLO! FPS: %s",0);    //Text and arguments
	_gui.addText("timeDisplay",
								vec3(60,70,0),
								GLUT_BITMAP_8_BY_13,
								vec3(0.6f,0.2f,0.2f),
								"Elapsed time: %5.0f",GETTIME());
	F32 fogColor[4] = {0.5, 0.5, 0.5, 1.0}; 
	_GFX.enableFog(0.3f,fogColor);
	_gui.addButton("testButton","click me",vec2(20,20),vec2(40,40),vec3(0.3f,0.2f,0.6f),TheButtonCallback);
/*
	int ev = 1;
	char* ev2 = "aha";
	Event *_event = new Event(30,true,100,&printHello,string("Hello from stupid thread"),TYPE_STRING);
	Event *_event2 = new Event(2000,true,200,&printHello,ev,TYPE_INTEGER);
	Event *_event3 = new Event(300,true,10,&printHello,ev2,TYPE_CHAR);
	Event *_event4 = new Event(200,true,20,&printHello,1.0f,TYPE_FLOAT);
	Text3D* t = new Text3D();
	t->getName() = "test text";
	t->getText() = "Test text to display";
	cout << "Test: " << t->getName() <<  " has text: " << t->getText() << endl;
	*/
	
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
	UBYTE *imageData = new UBYTE[w * h * 4];

// read the pixels from the frame buffer
	glReadPixels(xmin,ymin,xmax,ymax,GL_RGBA,GL_UNSIGNED_BYTE, (void*)imageData);

// save the image 
	TextureManager::getInstance().SaveSeries(filename,w,h,32,imageData);
  delete[] imageData;
}