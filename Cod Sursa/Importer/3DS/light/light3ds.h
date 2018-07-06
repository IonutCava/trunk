#ifndef LIGHT3DS_H
#define LIGHT3DS_H

#include <string>
#include <vector>

#include "../chunk/chunk.h"

#include "light3dsobject.h"

/***/
class Light3DS
{
    public:
	/***/
	Light3DS();
	/***/
	~Light3DS();

	void Parse(const std::string &name , Model3DSChunk c);
    private:

	std::vector<Light3DSObject> lights;
};

#endif
