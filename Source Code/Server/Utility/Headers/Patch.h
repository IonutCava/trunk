#ifndef _PATCH_H_
#define _PATCH_H_

#include "Core/Headers/Singleton.h"
#include "Utility/Headers/Vector.h"
#include "DataTypes.h"

namespace Divide {

struct PatchData
{
	stringImpl sceneName;
	U32 size;
	vectorImpl<stringImpl> name, modelName;
	vectorImpl<F32> version;
};

DEFINE_SINGLETON(Patch)

public:
	bool compareData(const PatchData& data);
	void addGeometry(const FileData& data);
	const vectorImpl<FileData>& updateClient();
	void reset() {ModelData.clear();};

private:
	vectorImpl<FileData> ModelData;

END_SINGLETON

}; //namespace Divide

#endif