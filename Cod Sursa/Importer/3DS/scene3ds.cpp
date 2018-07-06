#include "scene3ds.h"

#include <iostream>
using namespace std;

#include "chunk/file.h"
#include "chunk/chunk.h"

Scene3DS::Scene3DS(const char *src)
{
	operator=(src);
}
Scene3DS::Scene3DS(const string &src)
{
	operator=(src.c_str());
}
Scene3DS::~Scene3DS()
{
}

void Scene3DS::operator=(const char *src)
{
	Model3DSFile file(src);

	Model3DSChunk root = file.Child();

	for(Model3DSChunk cc = root.Child() ; cc ; cc = cc.Sibling())
	{
		switch(cc.ID())
		{
			case(0x0002):
				// parse int which has version info
			break;
			
			case(0x3d3d):
				ParseEditor3D(cc);
			break;

			default:
			break;
		}
	}
}

void Scene3DS::ParseEditor3D(Model3DSChunk c)
{
	for(Model3DSChunk cc = c.Child() ; cc ; cc = cc.Sibling())
	{
		switch(cc.ID())
		{
			case(0x3d3e):
				// mesh version
			break;

			case(0xafff):
				Material3DS::Parse(cc);
			break;
			
			case(0x4000):
				ParseEditObject(cc);
			break;
			
			default:
			break;
		}
	}
}

void Scene3DS::ParseEditObject(Model3DSChunk c)
{
	string obj_name = c.Str();
	
	for(Model3DSChunk cc = c.Child() ; cc ; cc = cc.Sibling())
	{
		switch(cc.ID())
		{
			case(0x4100):
				Mesh3DS::Parse(obj_name , cc);
			break;
			
			case(0x4600):
				Light3DS::Parse(obj_name , cc);
			break;
			
			case(0x4700):
				cout << "\tobject camera\n";
			break;
			
			default:
				cout << "\terror : unknown object "
					<< hex << cc.ID() << dec << "\n";
			break;
		}
	}
}
