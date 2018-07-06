/*“Copyright 2009-2011 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BASE_CLASS_H_
#define BASE_CLASS_H_

#include "resource.h"

class ResourceDescriptor{
public:
	ResourceDescriptor(const std::string& name = "default", 
					  const std::string& resourceLocation = "default",
					   bool flag = false, U32 id = RAND_MAX) : _name(name),
															  _resourceLocation(resourceLocation),
															  _flag(flag),
															  _id(id){}

	const std::string& getResourceLocation() const {return _resourceLocation;}
	const std::string& getName()			 const {return _name;}
		  bool getFlag()					 const {return _flag;}
		  U32  getId()						 const {return _id;}

	void setResourceLocation(const std::string& resourceLocation) {_resourceLocation = resourceLocation;}
	void setName(const std::string& name)					      {_name = name;}
	void setFlag(bool flag)				                          {_flag = flag;}
	void setId(U32 id)					                          {_id = id;}

private:
	std::string _name;			   //Item name
	std::string _resourceLocation; //Physical file location
	bool        _flag;
	U32         _id;
};

class Resource;
typedef unordered_map<std::string, Resource* > ResourceDependencyMap;
class Resource{
	friend class Manager;
public:
	Resource() : _shouldDelete(false), _name("default"),_refCount(1){}
	Resource(std::string name) : _shouldDelete(false), _name(name),_refCount(1){}
	virtual ~Resource() {}

	/*Loading/Unloading*/
	virtual bool load(const std::string& name) = 0;
	virtual bool unload() = 0;
	/*//Loading/Unloading*/

	/*Garbage Collection*/
	virtual void scheduleDeletion(){_shouldDelete = true;}
	virtual void cancelDeletion(){_shouldDelete = false;}
	virtual bool shouldDelete(){return _shouldDelete;}
	virtual void createCopy() = 0;
	virtual void removeCopy() = 0;
	/*//Garbage Collection*/

	/*ID management*/
	const std::string& getName() {return _name;}
	void setName(const std::string& name) {_name = name;}

	const std::string& getResourceLocation() {return _resourceLocation;}
	void setResourceLocation(const std::string& resourceLocation) {_resourceLocation = resourceLocation;}
	/*//ID management*/
	U32   getRefCount(){return _refCount;}

private:  // emphasize the following members are private
      Resource( const Resource& );
      const Resource& operator=( const Resource& );
	  U32 _refCount;
protected:
	void incRefCount(){_refCount++;}
	void decRefCount(){_refCount--;}

protected:
	bool		 _shouldDelete;
	std::string	 _name;
	std::string  _resourceLocation; //Physical file location
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
	U16    treeDensity;
	F32  grassScale;
	F32  treeScale;
	vec3   position;
	vec2   scale;
	bool   active;

};
#endif