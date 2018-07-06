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

#ifndef _TERRAIN_DESCRIPTOR_H_
#define _TERRAIN_DESCRIPTOR_H_

#include "core.h"
#include "Core/Resources/Headers/Resource.h"

class TerrainDescriptor : public Resource {
public:
	TerrainDescriptor() :  Resource(),
                           _active(false),
                           _normalMapScale(0.0f),
                           _diffuseScale(0.0f),
                           _grassDensity(0),
                           _chunkSize(0),
                           _treeScale(1.0f),
                           _grassScale(1.0f),
                           _treeDensity(1)
    {
    }

	~TerrainDescriptor() {_variables.clear();}

	bool  unload() {return true;}

	void addVariable(const std::string& name, const std::string& value){
		_variables.insert(make_pair(name,value));
	}

	void setPosition(const vec3<F32>& position) {_position = position;}
	void setScale(const vec2<F32>& scale)	    {_scale = scale;}
    void setDiffuseScale(F32 diffuseScale)      {_diffuseScale = diffuseScale;}
    void setNormalMapScale(F32 normalMapScale)  {_normalMapScale = normalMapScale;}
	void setGrassDensity(U32 grassDensity)      {_grassDensity = grassDensity;}
	void setTreeDensity(U16 treeDensity)        {_treeDensity = treeDensity;}
	void setGrassScale(F32 grassScale)          {_grassScale = grassScale;}
	void setTreeScale(F32 treeScale)            {_treeScale = treeScale;}
	void setActive(bool active)                 {_active = active;}
    void setChunkSize(U32 size)                 {_chunkSize = size;}

	U32   getGrassDensity()  {return _grassDensity;}
	U16   getTreeDensity()   {return _treeDensity;}
	F32   getGrassScale()    {return _grassScale;}
	F32   getTreeScale()     {return _treeScale;}
	bool  getActive()        {return _active;}
    U32   getChunkSize()     {return _chunkSize;}
    F32   getDiffuseScale()  {return _diffuseScale;}
    F32   getNormalMapScale(){return _normalMapScale;}
	vec3<F32>& getPosition()	 {return _position;}
	vec2<F32>& getScale()        {return _scale;}

	std::string& getVariable(const std::string& name) {
		std::string& var = _variables.find(name)->second;
		if(var.empty()) ERROR_FN(Locale::get("ERROR_TERRAIN_DESCRIPTOR_MISSING_VAR"),name.c_str());
		return var;
	}
private:
	Unordered_map<std::string,std::string> _variables;
	U32    _grassDensity;
    U32    _chunkSize;
	U16    _treeDensity;
	F32	   _grassScale;
	F32    _treeScale;
    F32    _normalMapScale;
    F32    _diffuseScale;
	bool   _active;
	vec3<F32>   _position;
	vec2<F32>   _scale;
};
#endif