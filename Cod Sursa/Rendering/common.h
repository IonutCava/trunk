#ifndef ENGINE_H_
#define ENGINE_H_

#include "resource.h"
#include "Utility/Headers/Singleton.h"

class PhysX;
class Camera;
class SceneManager;
class GUI;
class Terrain;
class GFXDevice;

SINGLETON_BEGIN( Engine )

private:
	Engine();
	int time, timebase;
	static int status;
	int mainWindowId;
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

   F32 moveFB,moveLR,angleUD,angleLR;
   void LoadControls();
   //rendering functions
   void Initialize(); //Set up the rendering platform
   static void Pick(int name){}
   static void DrawSceneStatic();
   void DrawScene();
   void RefreshMetrics();

   static void Idle();
   void Quit();
   int  getMainWindowId(){return mainWindowId;}
   void setMainWindowId(U32 id){mainWindowId = id;}
 
   SINGLETON_END()

#endif