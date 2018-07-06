#include "chunk.h"

#include <iostream>
using namespace std;

Model3DSChunk::Model3DSChunk(ifstream &infile , int csend)
: file(infile) , chunkset_end(csend)
{
	//cout << "Model3DSChunk::Model3DSChunk()\n";
	begin = file.tellg();

	id = 0;		// zero out upper bytes
	file.read((char*)&id , 2);

	file.read((char*)&end , 4);
	end += begin;	// compute absolute position

	/*cout << "id = (" << hex << id << ")\t" << dec
		<< "begin = (" << begin << ")\t"
		<< "end = (" << end << ")\t"
		<< "chunkset_end = (" << chunkset_end << ")\n";*/
}
Model3DSChunk::Model3DSChunk(const Model3DSChunk &chunk)
: file(chunk.file) ,
  chunkset_end(chunk.chunkset_end) ,
  id(chunk.id) , begin(chunk.begin) , end(chunk.end)
{
	//cout << "Model3DSChunk::Model3DSChunk(const Model3DSChunk &chunk)\n";
}
Model3DSChunk::~Model3DSChunk()
{
}

void Model3DSChunk::operator=(const Model3DSChunk &chunk)
{
	//cout << "Model3DSChunk::operator=(const Model3DSChunk &chunk)\n";

	file.seekg(chunk.file.tellg());

	chunkset_end = chunk.chunkset_end;
	id = chunk.id;
	begin = chunk.begin;
	end = chunk.end;
}

Model3DSChunk::operator bool()
{
	int curr_pos = file.tellg();
	/*cout << "Model3DSChunk::operator bool() == "
		<< ((begin < chunkset_end) && (begin >= 0)) << "\n"
		<< "(" << begin << " < " << chunkset_end << ")\n";*/
	return (begin < chunkset_end) && (begin >= 0);
}

int Model3DSChunk::ID()
{
	return id;
}

short Model3DSChunk::Short()
{
	short s = 0;
	file.read((char*)&s , 2);
	return s;
}
int Model3DSChunk::Int()
{
	int i;
	file.read((char*)&i , 4);
	return i;
}
F32 Model3DSChunk::Float()
{
	F32 f;
	file.read((char*)&f , 4);
	return f;
}
string Model3DSChunk::Str()
{
	string s;
	char c;

	do
	{
		file.read(&c , 1);
		s += c;
	}while(c != '\0');

	return s;
}

Model3DSChunk Model3DSChunk::Child()
{
	//cout << "end = (" << end << ")\n";
	return Model3DSChunk(file , end);
}
Model3DSChunk Model3DSChunk::Sibling()
{
	file.seekg(end);// travel to next chunk
	//cout << "seek = (" << end << ")\n";
	return Model3DSChunk(file , chunkset_end);
}
