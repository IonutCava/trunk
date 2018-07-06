#ifndef _PHYSX_PROCESSOR_H_
#define _PHYSX_PROCESSPR_H_

#include "PhysX/PhysX.h"
#include "PhysX/PhysxSceneInterface.h"

class PhysXImplementation : public PhysXSceneInterface {
public:
	PhysXImplementation(Scene* currentScene) : PhysXSceneInterface(currentScene){init();}
	~PhysXImplementation(){exit();}
	bool exit() {return true;}
	void idle() {}
};

#endif