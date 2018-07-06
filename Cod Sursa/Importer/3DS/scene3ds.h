#ifndef SCENE3DS_H
#define SCENE3DS_H

#include <string>

#include "light/light3ds.h"
#include "material/material3ds.h"
#include "mesh/mesh3ds.h"

/***/
class Scene3DS : public Light3DS , public Material3DS , public Mesh3DS
{
    public:
	/***/
	Scene3DS(const char *src);
	/***/
	Scene3DS(const std::string &src);
	/***/
	~Scene3DS();

	void operator=(const char *src);
    private:
	
	void ParseEditor3D(Model3DSChunk c);
	void ParseEditObject(Model3DSChunk c);
};

#endif
