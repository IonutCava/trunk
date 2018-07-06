#ifndef _DIVIDE_FORMAT_CONVERTER_H_
#define _DIVIDE_FORMAT_CONVERTER_H_

#include "resource.h"
#include <aiScene.h> 
#include "Geometry/Mesh.h"

class DVDFile : public Mesh
{

public:
	DVDFile();
	~DVDFile();
    bool load(const string& file);
	bool unload();

private:
	string name;
	const aiScene* scene;
};

#endif