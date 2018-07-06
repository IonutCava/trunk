/*“Copyright 2009-2013 DIVIDE-Studio”*/
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

#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#include "core.h"
#include "Core/MemoryManagement/Headers/TrackedObject.h"

class Resource : public TrackedObject {

public:
	Resource() : TrackedObject(),  _name("default"),
								   _loadingComplete(false){}

	Resource(const std::string& name) :TrackedObject(), _name(name),
							   					        _loadingComplete(false){}
	virtual ~Resource() {}

	///Loading and unloading interface
	virtual bool unload() = 0;

	///Name management
	const std::string& getName() {return _name;}
	inline void setName(const std::string& name) {_name = name;}

	///Physical file location
	const std::string& getResourceLocation() {return _resourceLocation;}
	void setResourceLocation(const std::string& resourceLocation) {_resourceLocation = resourceLocation;}

	inline bool isLoaded() {return _loadingComplete;}

protected:
	template<typename T>
	friend class ImplResourceLoader;
	///Use this as a callback for multi-threaded loading;
	virtual bool setInitialData(const std::string& name) {_loadingComplete = true; return true;}

protected:
	std::string	 _name;
	std::string  _resourceLocation; ///< Physical file location
	bool _loadingComplete;
};

enum GEOMETRY_TYPE {

	VEGETATION,  ///< For special rendering subroutines
	PRIMITIVE,   ///< Simple objects: Boxes, Spheres etc
	GEOMETRY     ///< All other forms of geometry
};

struct FileData {
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

struct TerrainInfo {

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