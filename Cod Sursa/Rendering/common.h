#ifndef ENGINE_H_
#define ENGINE_H_

#include "resource.h"
#include "GUI/zpr.h"
#include "Utility/Headers/Singleton.h"
#include "Hardware/Video/GFXDevice.h"
#include "Vegetation/Vegetation.h"

using namespace std;

class PhysX;
class Camera;
class SceneManager;
class GUI;
class Terrain;

SINGLETON_BEGIN( Engine )

private:
	Engine();
	int time, timebase;
	static int status;
	int mainWindowId;
	F32 EpochTime;
	F32 update_time, update_time2, update_time3;
	static int initializat;
	static bool loaded;
	auto_ptr<Terrain> ter;
	auto_ptr<Vegetation> vegetation;
	bool firstPersonCamera;
	bool detailMapState;
	bool m_bWireframe;
	void RenderActors(); // Used when PhysX isn't available
	void ProcessRenderingInput();
	D32 fps;
	int width, height;

	GFXDevice&    _GFX;
    PhysX&        _px;
	SceneManager& _scene;
	Camera&       _camera;
	GUI&          _gui;

public:
	int getWindowWidth(){return width;}
	int getWindowHeight(){return height;}
	void setWindowWidth(int w){width = w;}
	void setWindowHeight(int h){height = h;}
   void toggleFirstPerson(){firstPersonCamera = !firstPersonCamera;}
   F32 moveFB,moveLR,angleUD,angleLR,tip, turn;
   void LoadControls();
   //rendering functions
   void Initialize(int w, int h); //Set up the rendering platform
   static void Pick(GLint name){}
   static void DrawSceneStatic();
   string text;
   void DrawScene();
   void RefreshMetrics();

   void RestoreLightCameraMatrices();
   static void Idle();
   void Quit();
   int  getMainWindowId(){return mainWindowId;}
   void setMainWindowId(int id){mainWindowId = id;}
   void DrawComplexMesh(U32 ListID);

   void Screenshot(char *filename, int xmin, int ymin, int xmax, int ymax);
   void ToggleWireframeRendering();
   bool isWireframeRendering() {return m_bWireframe;}
   
   SINGLETON_END()

#endif