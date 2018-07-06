#ifndef BASE_CLASS_H_
#define BASE_CLASS_H_

#include "resource.h"

class Resource
{
public:
	Resource() : _shouldDelete(false){}
	virtual bool load(const std::string& name) = 0;
	virtual bool unload() = 0;
	virtual void scheduleDeletion(){_shouldDelete = true;}
	virtual void cancelDeletion(){_shouldDelete = false;}
	virtual bool clean() {if(_shouldDelete) return unload(); else return false;}

protected:
	bool _shouldDelete;
};

class GraphicResource : public Resource
{
	virtual void draw() const = 0;
};

enum GEOMETRY_TYPE
{
	VEGETATION,  //For special rendering subroutines
	PRIMITIVE,   //Simple objects: Boxes, Spheres etc
	GEOMETRY     //All other forms of geometry
};

class FileData
{
public:
	std::string ItemName;
	std::string ModelName;
	vec3 scale;
	vec3 position;
	vec3 orientation;
	vec3 color;
	GEOMETRY_TYPE type;
	F32 data; //general purpose
	std::string data2;
	F32 version;
};

class TerrainInfo
{
public:
	TerrainInfo(){position.set(0,0,0);}
	//"variables" contains the various strings needed for each terrain such as texture names, terrain name etc.
	std::map<std::string,std::string> variables;
	U32    grassDensity;
	U32    treeDensity;
	F32  grassScale;
	F32  treeScale;
	vec3   position;
	vec2   scale;
	bool   active;

};
#endif