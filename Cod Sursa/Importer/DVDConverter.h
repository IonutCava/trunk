#ifndef _DIVIDE_FORMAT_CONVERTER_H_
#define _DIVIDE_FORMAT_CONVERTER_H_

#include "resource.h"
#include <aiScene.h> 
#include "Geometry/Mesh.h"
#include  "MultiThreading/threadHandler.h"

class DVDFile : public Mesh
{

public:
	DVDFile();
	~DVDFile();
    bool load(const string& file);
	bool unload();
	void scheduleDeletion(){_shouldDelete = true;}
	void cancelDeletion(){_shouldDelete = false;}
	void clean(){if(_shouldDelete) unload(); delete this;};
private:
	bool load_threaded();

private:
	const aiScene* scene;
	U32   _ppsteps;
	bool _shouldDelete;
};

#endif