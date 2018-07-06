#ifndef MESH3DSOBJECT_H
#define MESH3DSOBJECT_H

#include <map>
#include <string>
#include <vector>
#include "../chunk/chunk.h"

struct Mesh3DSVertex
{
	F32 x , y , z;
};
struct Mesh3DSTextureCoord
{
	F32 u , v;
};
struct Mesh3DSFace
{
	int a , b , c;
};
struct Mesh3DSMatrix
{
	F32 m[4][4];
};

/***/
class Mesh3DSObject
{
    public:
	/***/
	Mesh3DSObject(const std::string &nname , Model3DSChunk c);
	/***/
	Mesh3DSObject(const Mesh3DSObject &mesh);
	/***/
	~Mesh3DSObject();
	
	void operator=(const Mesh3DSObject &mesh);

	const std::string &Name();
	Mesh3DSMatrix Matrix();	// adds last row (0,0,0,1)

	const std::vector<Mesh3DSVertex> &Vertices();
	const std::vector<Mesh3DSTextureCoord> &TextureCoords();
	const std::vector<Mesh3DSFace> &Faces();
	const std::map<std::string , std::vector<int> > &Materials();

    private:
	
	void ParseLocalCoordinateSystem(Model3DSChunk c);
	void ParseVertices(Model3DSChunk c);
	void ParseTextureCoords(Model3DSChunk c);
	void ParseFaces(Model3DSChunk c);

	void ParseFacesMaterials(Model3DSChunk c);

	std::string name;
	Mesh3DSMatrix matrix;

	std::vector<Mesh3DSVertex> vertices;
	std::vector<Mesh3DSTextureCoord> texturecoords;
	std::vector<Mesh3DSFace> faces;
	std::map<std::string , std::vector<int> > material_faces;
};

#endif
