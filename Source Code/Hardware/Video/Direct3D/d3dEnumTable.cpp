#include "Headers/d3dEnumTable.h"
unsigned int d3dTextureTypeTable[TEXTURE_TYPE_PLACEHOLDER];

namespace D3D_ENUM_TABLE {

	void fill(){
		d3dTextureTypeTable[TEXTURE_1D] = 0;
		d3dTextureTypeTable[TEXTURE_2D] = 1;
		d3dTextureTypeTable[TEXTURE_3D] = 2;
		d3dTextureTypeTable[TEXTURE_CUBE_MAP] = 3;

	}
}
