/**********************************************************/
/****************** User Interface Stuff ******************/
/**********************************************************/
#ifndef GLUI_MANAGER
#define GLUI_MANAGER

enum CallbackType
{
	QUIT_ID,
	HUGE_CRATE,
	STACK_CRATES,
	TOWER_CRATES,
	SPHERE,
	ACTOR_POSITION,
	TREE_VISIBILITY,
	GRASS_VISIBILITY,
	WIND_SPEED,
	STACK_20,
	STACK_100,
	TOWER_15,
	SPHERE_1,
	BOX_1,
	DEBUG_VIEW
};

void glui_cb(int control);
void processMenuEvents(int option);
#endif