#include "GLUIManager.h"
#include "Utility/Headers/Guardian.h"
#include "Utility/Headers/ParamHandler.h"
#include "Managers/SceneManager.h"

F32 x = 0, y = 20, z = 0;
F32 treeviz= 100, grassviz = 100, windspeed = 1;

void glui_cb(int control)
{
   ParamHandler &par = ParamHandler::getInstance();
   switch (control)
   {
    case QUIT_ID:
		Guardian::getInstance().TerminateApplication();
    break;
	case STACK_20:
		PhysX::getInstance().CreateStack(20);
	break;
	case STACK_100:
		PhysX::getInstance().CreateStack(20);
	break;
	case TOWER_15:
		PhysX::getInstance().CreateTower(15);
	break;
	case SPHERE_1:
		PhysX::getInstance().CreateSphere(8);
	break;
	case BOX_1:
		PhysX::getInstance().CreateCube(20);
	break;
	case DEBUG_VIEW:
		PhysX::getInstance().setDebugRender(!PhysX::getInstance().getDebugRender());
	break;
	case ACTOR_POSITION:
		PhysX::getInstance().setActorDefaultPos(vec3(x,y,z));
	break;
	case GRASS_VISIBILITY:
		SceneManager::getInstance().getTerrainManager()->getGrassVisibility() = grassviz;
	break;
	case TREE_VISIBILITY:
		SceneManager::getInstance().getTerrainManager()->getTreeVisibility() = treeviz;
	break;
	case WIND_SPEED:
		SceneManager::getInstance().getTerrainManager()->getWindSpeed() = windspeed;
	break;
   }
}



