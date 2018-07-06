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
	bool m_bWireframe;
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

   F32 moveFB,moveLR,angleUD,angleLR,tip, turn;
   void LoadControls();
   //rendering functions
   void Initialize(int w, int h); //Set up the rendering platform
   static void Pick(GLint name){}
   static void DrawSceneStatic();
   void DrawScene();
   void RefreshMetrics();

   static void Idle();
   void Quit();
   int  getMainWindowId(){return mainWindowId;}
   void setMainWindowId(U32 id){mainWindowId = id;}
   void Screenshot(char *filename, int xmin, int ymin, int xmax, int ymax);
   void ToggleWireframeRendering();
   bool isWireframeRendering() {return m_bWireframe;}
   
   SINGLETON_END()

#endif