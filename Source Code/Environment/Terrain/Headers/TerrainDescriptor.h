/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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