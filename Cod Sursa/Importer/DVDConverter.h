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
	//DVDFile(const DVDFile& old);  //No need for this yet. The original aiScene will always be the same for copies
								    //and all other memebers are default copy-able(??) anyway

	~DVDFile();
    bool load(const string& file);
	bool unload();
	void scheduleDeletion(){_shouldDelete = true;}
	void cancelDeletion(){_shouldDelete = false;}
	bool clean();

private:
	bool load_threaded();

private:
	const aiScene* scene;
	U32   _ppsteps;
	bool _shouldDelete;
};

#endif