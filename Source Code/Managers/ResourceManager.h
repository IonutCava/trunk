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

#ifndef _RESOURCE_MANAGER_H
#define _RESOURCE_MANAGER_H
//#define loadResource<X>(Y) ResourceManager::getInstance().loadResource<X>(Y);

#include "Managers/Manager.h" 
#include "Utility/Headers/Singleton.h" 
class Texture;
typedef Texture Texture2D;
typedef Texture TextureCubemap;

DEFINE_SINGLETON_EXT1( ResourceManager,Manager )

public:
	template<class T>
	T* loadResource(const ResourceDescriptor& descriptor);
	template<class T>
	void removeResource(T*& res,bool force = false);

protected:
	Resource* loadResource(const std::string& name);

	~ResourceManager();
END_SINGLETON

#endif

