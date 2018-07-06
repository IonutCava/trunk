#ifndef _DIVIDE_FORMAT_CONVERTER_H_
#define _DIVIDE_FORMAT_CONVERTER_H_

#include "resource.h"
#include "Geometry/Mesh.h"

struct aiScene;
SINGLETON_BEGIN( DVDConverter )

public:
    Mesh* load(const std::string& file);

private:
	const aiScene* scene;
	U32   _ppsteps;

SINGLETON_END()

#endif