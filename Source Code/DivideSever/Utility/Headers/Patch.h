#ifndef _PATCH_H_
#define _PATCH_H_

#include <DivideNetworking/Utility/Singleton.h>
#include "DataTypes.h"

struct PatchData
{
	string sceneName;
	U32 size;
	std::vector<string> name, modelName;
	std::vector<F32> version;
};

DEFINE_SINGLETON(Patch)

public:
	bool compareData(const PatchData& data);
	void addGeometry(const FileData& data);
	const vector<FileData>& updateClient();
	void reset() {ModelData.clear();};

private:
	vector<FileData> ModelData;

END_SINGLETON

#endif