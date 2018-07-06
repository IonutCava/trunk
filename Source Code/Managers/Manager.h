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

#ifndef _MANAGER_H_
#define _MANAGER_H_

#include "resource.h"
#include "Utility/Headers/BaseClasses.h"

class Manager{
protected:
	typedef unordered_map<std::string, Resource*> ResourceMap;
	ResourceMap _resDB;
	ResourceMap::iterator _resDBiter;

public: 
	void add(const std::string& name, Resource* res);
	Resource* find(const std::string& name);
	//If we crash here due to an invalid name, the problem lies with the resource, not the manager
	virtual bool remove(Resource* res,bool force = false);
	virtual void Destroy();
	virtual ~Manager() {Destroy();} //Deleting any manager, will destroy it first
};

#endif