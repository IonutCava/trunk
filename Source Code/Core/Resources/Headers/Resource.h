/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef RESOURCE_H_
#define RESOURCE_H_

#include "core.h"


class Resource : private boost::noncopyable {
	friend class BaseCache;
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
	inline void setName(const std::string& name) {_name = name;}
	/*//ID management*/
	U32   getRefCount(){return _refCount;}

	const std::string& getResourceLocation() {return _resourceLocation;}
	void setResourceLocation(const std::string& resourceLocation) {_resourceLocation = resourceLocation;}
private:  // emphasize the following members are private
	  U32 _refCount;
protected:
	void incRefCount(){_refCount++;}
	void decRefCount(){_refCount--;}

protected:
	bool		 _shouldDelete;
	std::string	 _name;
	std::string  _resourceLocation; ///< Physical file location
};

enum GEOMETRY_TYPE
{
	VEGETATION,  ///< For special rendering subroutines
	PRIMITIVE,   ///< Simple objects: Boxes, Spheres etc
	GEOMETRY     ///< All other forms of geometry
};

class FileData
{
public:
	std::string ItemName;
	std::string ModelName;
	vec3<F32> scale;
	vec3<F32> position;
	vec3<F32> orientation;
	vec3<F32> color;
	GEOMETRY_TYPE type;
	F32 data; ///< general purpose
	std::string data2;
	F32 version;
};

class TerrainInfo
{
public:
	TerrainInfo(){position.set(0,0,0);}
	///"variables" contains the various strings needed for each terrain such as texture names, terrain name etc.
	unordered_map<std::string,std::string> variables;
	U32    grassDensity;
	U16    treeDensity;
	F32  grassScale;
	F32  treeScale;
	vec3<F32>   position;
	vec2<F32>   scale;
	bool   active;

};

#endif