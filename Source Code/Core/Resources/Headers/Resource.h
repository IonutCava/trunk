/*�Copyright 2009-2013 DIVIDE-Studio�*/
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

#include "Utility/Headers/UnorderedMap.h"
#include "Core/Math/Headers/MathClasses.h"
#include "Core/MemoryManagement/Headers/TrackedObject.h"

///When "CreateResource" is called, the resource is in "RES_UNKNOWN" state. Once it has been instantiated it will move to the "RES_CREATED" state.
///Calling "load" on a non-created resource will fail. After "load" is called, the resource is move to the "RES_LOADING" state
///Nothing can be done to the resource when it's in "RES_LOADING" state!
///Once loading is complete, preferably in another thread, the resource state will become "RES_LOADED" and ready to be used (e.g. added to the SceneGraph)
///Calling "unload" is only available for "RES_LOADED" state resources. Calling this method will set the state to "RES_LOADING"
///Once unloading is complete, the resource will become "RES_CREATED". It will still exist, but won't contain any data.
///RES_UNKNOWN and RES_CREATED are safe to delete
enum ResourceState{
    RES_UNKNOWN   = 0, //<The resource exists, but it's state is undefined
    RES_CREATED   = 1, //<The pointer has been created and instantiated, but no data has been loaded
    RES_LOADING   = 2, //<The resource is in loading or unloading stage, creating or deleting data, parsing scripts, etc
    RES_LOADED    = 3  //<The resource is loaded and available
};

class Resource : public TrackedObject {
public:
	Resource() : TrackedObject(),  _name("default"),
								   _threadedLoading(true),
								   _threadedLoadComplete(false),
								   _resourceState(RES_CREATED)
    {
    }

	Resource(const std::string& name) : TrackedObject(),
                                       _name(name),
									   _threadedLoading(true),
									   _threadedLoadComplete(false),
							   		   _resourceState(RES_CREATED)
    {
    }

	virtual ~Resource() {}

	///Loading and unloading interface
	virtual bool unload() = 0;

	///Name management
	const  std::string& getName()                       const {return _name;}
	inline void         setName(const std::string& name)      {_name = name;}
	///Physical file location
	const std::string& getResourceLocation()                                   const {return _resourceLocation;}
	      void         setResourceLocation(const std::string& resourceLocation)      {_resourceLocation = resourceLocation;}

    inline ResourceState getState() const {return _resourceState;}
	///Toggle loading in background thread
	inline void enableThreadedLoading(const bool enableThreadedLoading) {_threadedLoading = enableThreadedLoading;}
	virtual void threadedLoad(const std::string& name)                  {_threadedLoadComplete = true;}

protected:
    friend class DVDConverter;
    friend class ResourceCache;
    friend class ResourceLoader;
    template<class X>
    friend class ImplResourceLoader;
    inline void setState(const ResourceState& currentState) {_resourceState = currentState;}

protected:
	std::string	  _name;
	std::string   _resourceLocation; ///< Physical file location
    boost::atomic<ResourceState> _resourceState;
	///Should load resource in a background thread
	boost::atomic_bool _threadedLoading;
	///Becomes true when background thread finishes loading
	boost::atomic_bool _threadedLoadComplete;
};

enum GeometryType {
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
	GeometryType type;
	F32 data; ///< general purpose
	std::string data2;
	std::string data3;
	F32 version;
    bool staticUsage; //<Used to determine if it's a static object or dynamic. Affects lighting, navigation, etc.
    bool navigationUsage; //< Used to determine if the object should be added to the nav mesh generation process or not
};

struct TerrainInfo {
	TerrainInfo(){position.set(0,0,0);}
	///"variables" contains the various strings needed for each terrain such as texture names, terrain name etc.
	Unordered_map<std::string,std::string> variables;
	U32    grassDensity;
	U16    treeDensity;
	F32  grassScale;
	F32  treeScale;
	vec3<F32>   position;
	vec2<F32>   scale;
	bool   active;
};

#endif