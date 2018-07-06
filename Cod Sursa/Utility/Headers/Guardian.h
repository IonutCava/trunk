#include "resource.h"
#include "PhysX/PhysX.h"
#include "Rendering/common.h"
#include "Utility/Headers/Singleton.h"

SINGLETON_BEGIN( Guardian )

private:
	vector<FileData> ModelDataArray;
	vector<TerrainInfo> TerrainInfoArray;
	int nModelIndex;

public:
	void LoadSettings();
	void LoadApplication(string entryPoint);
	vector<FileData>& getModelDataArray() {return ModelDataArray;}
	void ReloadSettings();
	void RestartPhysX();
	static void TerminateApplication();
	void StartPhysX();
	void ReloadEngine();
	std::ofstream myfile;
	void addTerrain(const TerrainInfo& ter) {TerrainInfoArray.push_back(ter);}
	
 SINGLETON_END()