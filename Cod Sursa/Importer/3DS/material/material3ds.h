#ifndef MATERIAL3DS_H
#define MATERIAL3DS_H

#include <vector>

#include "../chunk/chunk.h"

#include "material3dsobject.h"

/***/
class Material3DS
{
    public:
	/***/
	Material3DS();
	/***/
	~Material3DS();

	void Parse(Model3DSChunk c);

	const std::vector<Material3DSObject> &Materials();
    private:
	void NewMaterial(Model3DSChunk c);
	
	std::vector<Material3DSObject> materials;
};

#endif
