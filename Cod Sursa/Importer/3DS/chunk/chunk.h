#ifndef CHUNK_H
#define CHUNK_H

#include <fstream>
#include <string>
#include "Utility/Headers/DataTypes.h"
/***/
class Model3DSChunk
{
    public:
	/***/
	Model3DSChunk(std::ifstream &infile , int csend);
	/***/
	Model3DSChunk(const Model3DSChunk &chunk);
	/***/
	~Model3DSChunk();

	/**bug : making 2 seperate file stream chunks = each other*/
	void operator=(const Model3DSChunk &chunk);

	operator bool();

	int ID();

	short Short();
	int Int();
	float Float();
	std::string Str();

	Model3DSChunk Child();
	Model3DSChunk Sibling();
    private:
	std::ifstream &file;
	
	int chunkset_end;
	int id , begin , end;
};

#endif
