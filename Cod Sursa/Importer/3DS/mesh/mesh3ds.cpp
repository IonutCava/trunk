#include "mesh3ds.h"

#include <iostream>
using namespace std;

Mesh3DS::Mesh3DS()
{
}
Mesh3DS::~Mesh3DS()
{
}

void Mesh3DS::Parse(const string &name , Model3DSChunk c)
{
	cout << "Mesh3DS::Parse(" << name << ",c)\n";

	Mesh3DSObject mesh(name , c);
	meshes.push_back(mesh);
}

const std::vector<Mesh3DSObject> &Mesh3DS::Meshes()
{
	return meshes;
}
