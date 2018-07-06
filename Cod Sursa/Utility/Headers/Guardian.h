#include "resource.h"
#include "PhysX/PhysX.h"

SINGLETON_BEGIN( Guardian )


public:
	void LoadSettings();
	void LoadApplication(const std::string& entryPoint);
	void ReloadSettings();
	void RestartPhysX();
	static void TerminateApplication();
	void StartPhysX();
	void ReloadEngine();
	std::ofstream myfile;
	
	
 SINGLETON_END()