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

#ifndef _RESOURCE_MANAGER_H
#define _RESOURCE_MANAGER_H

#include "Manager.h" 

class Texture;
typedef Texture Texture2D;
typedef Texture TextureCubemap;

template<class T>void RemoveResource(T*& resource, bool force = false){
	if(ResourceManager::getInstance().remove(resource,force)){
		ResourceManager::getInstance().eraseEntry(resource->getName());
		delete resource;
		resource = NULL;
	}
}

DEFINE_SINGLETON_EXT1( ResourceManager,Manager )

public:
	template<class T>
	T* loadResource(const ResourceDescriptor& descriptor);
	virtual void add(const std::string& name, Resource* const resource);

protected:
	Resource* loadResource(const std::string& name);
	boost::mutex _creationMutex;
	~ResourceManager();

END_SINGLETON

#endif

