#include "GLUIManager.h"
#include "Rendering/common.h"
#include "Utility/Headers/Guardian.h"
#include "Utility/Headers/ParamHandler.h"

using namespace std;

F32 x = 0, y = 20, z = 0;
F32 treeviz= 100, grassviz = 100,windspeed = 1;

void glui_cb(int control)
{
   ParamHandler &par = ParamHandler::getInstance();
   switch (control)
   {
    //  Color Listbox item changed
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
		PhysX::getInstance().setActorDefaultPos(x,y,z);
	break;
	case GRASS_VISIBILITY:
		//par.setParam("grassVisibility",grassviz);
		TerrainManager::getInstance().setGrassVisibility(grassviz);
	break;
	case TREE_VISIBILITY:
		//par.setParam("treeVisibility",treeviz);
		TerrainManager::getInstance().setTreeVisibility(treeviz);
	break;
	case WIND_SPEED:
		//par.setParam("windSpeed",windspeed);
		TerrainManager::getInstance().setWindSpeed(windspeed);
	break;
   }
}



