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

#ifndef _TERRAIN_DESCRIPTOR_H_
#define _TERRAIN_DESCRIPTOR_H_

#include "resource.h"
#include "Core/Headers/BaseClasses.h"

class TerrainDescriptor : public Resource
{
public:
	TerrainDescriptor() :  Resource(), _active(false){}
	~TerrainDescriptor() {_variables.clear();}

	bool  load(const std::string& name) {_name = name; return true;}
	bool  unload() {return true;}

	void addVariable(const std::string& name, const std::string& value){
		_variables.insert(make_pair(name,value));
	}

	void setPosition(const vec3& position) {_position = position;}
	void setScale(const vec2& scale)	   {_scale = scale;}
	void setGrassDensity(U32 grassDensity) {_grassDensity = grassDensity;}
	void setTreeDensity(U16 treeDensity)   {_treeDensity = treeDensity;}
	void setGrassScale(F32 grassScale)     {_grassScale = grassScale;}
	void setTreeScale(F32 treeScale)       {_treeScale = treeScale;}
	void setActive(bool active)            {_active = active;}

	U32   getGrassDensity() {return _grassDensity;}
	U16   getTreeDensity()  {return _treeDensity;}
	F32   getGrassScale()   {return _grassScale;}
	F32   getTreeScale()    {return _treeScale;}
	vec3& getPosition()	    {return _position;}
	vec2& getScale()        {return _scale;}
	bool  getActive()       {return _active;}

	std::string& getVariable(const std::string& name) {
		std::string& var = _variables.find(name)->second;
		if(var.empty()) Console::getInstance().errorfn("TerrainDescriptor: Accessing inexistent variable [ %s ]",name.c_str());
		return var;

	}
	void createCopy(){incRefCount();}
	void removeCopy(){decRefCount();}
	
private:
	std::map<std::string,std::string> _variables;
	U32    _grassDensity;
	U16    _treeDensity;
	F32	   _grassScale;
	F32    _treeScale;
	vec3   _position;
	vec2   _scale;
	bool   _active;
};
#endif