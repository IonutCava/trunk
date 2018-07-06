#ifndef BASE_CLASS_H_
#define BASE_CLASS_H_

#include "resource.h"

class Resource
{
public:
	virtual bool load(const string& name) = 0;
	virtual bool unload() = 0;
};

class GraphicResource : public Resource
{
	virtual void draw() const = 0;
};

class FileData
{
public:
	string ModelName;
	F32 ScaleFactor;
	vec3 position;
	vec3 rotation;
	string PhysXCook;
	string NormalMap;
	bool Vegetation;
};

class TerrainInfo
{
public:
	TerrainInfo(){position.set(0,0,0);}
	//"variables" contains the various strings needed for each terrain such as texture names, terrain name etc.
	map<string,string> variables;
	U32    grassDensity;
	U32    treeDensity;
	F32  grassScale;
	F32  treeScale;
	vec3   position;
	vec2   scale;
	bool   active;

};
#endif