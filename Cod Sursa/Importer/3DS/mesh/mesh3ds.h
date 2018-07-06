#ifndef MESH3DS_H
#define MESH3DS_H

#include <string>
#include <vector>

#include "../chunk/chunk.h"

#include "mesh3dsobject.h"

/***/
class Mesh3DS
{
    public:
	/***/
	Mesh3DS();
	/***/
	~Mesh3DS();

	void Parse(const std::string &name , Model3DSChunk c);

	const std::vector<Mesh3DSObject> &Meshes();
    private:

	std::vector<Mesh3DSObject> meshes;
};

#endif
