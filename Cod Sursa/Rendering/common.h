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
	vec2 _dimensions;

	GFXDevice&    _GFX;
    PhysX&        _px;
	SceneManager& _scene;
	Camera*       _camera;
	GUI&          _gui;

public:
	const vec2& getWindowDimensions() const {return _dimensions;}
	void setWindowWidth(int w){_dimensions.x = w;}
	void setWindowHeight(int h){_dimensions.y = h;}

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