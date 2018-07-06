#include "material3ds.h"

#include <iostream>
#include <string>
using namespace std;

Material3DS::Material3DS()
{
}
Material3DS::~Material3DS()
{
}

void Material3DS::Parse(Model3DSChunk c)
{
	cout << "edit material\n";
	
	for(Model3DSChunk cc = c.Child() ; cc ; cc = cc.Sibling())
	{
		switch(cc.ID())
		{
			case(0xa000):
				NewMaterial(cc);
			break;
			
			default:
			break;
		}
	}
}

void Material3DS::NewMaterial(Model3DSChunk c)
{
	string mat_name = c.Str();
	cout << mat_name << "\n";

	Material3DSObject material(mat_name);
	materials.push_back(material);
}

const std::vector<Material3DSObject> &Material3DS::Materials()
{
	return materials;
}
