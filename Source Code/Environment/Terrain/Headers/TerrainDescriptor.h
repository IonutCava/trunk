/*
   Copyright (c) 2014 DIVIDE-Studio
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
    TerrainDescriptor() :  Resource("temp_terrain_descriptor"),
                           _active(false),
                           _addToPhysics(false),
                           _is16Bit(false),
                           _grassDensity(0),
                           _chunkSize(0),
                           _treeScale(1.0f),
                           _grassScale(1.0f),
                           _treeDensity(1),
                           _textureLayers(1)
    {
    }

    ~TerrainDescriptor() {_variables.clear();}

    bool  unload() {return true;}

    void addVariable(const std::string& name, const std::string& value){
        _variables.insert(make_pair(name,value));
    }

    void addVariable(const std::string& name, F32 value){
        _variablesf.insert(make_pair(name, value));
    }

    void setTextureLayerCount(U8 count)         { _textureLayers = count; }
    void setDimensions(const vec2<U16>& dim)    { _dimensions = dim; }
    void setAltitudeRange(const vec2<F32>& dim) { _altitudeRange = dim; }
    void setPosition(const vec3<F32>& position) { _position = position; }
    void setScale(const vec2<F32>& scale)	    { _scale = scale; }
    void setGrassDensity(U32 grassDensity)      { _grassDensity = grassDensity; }
    void setTreeDensity(U16 treeDensity)        { _treeDensity = treeDensity; }
    void setGrassScale(F32 grassScale)          { _grassScale = grassScale; }
    void setTreeScale(F32 treeScale)            { _treeScale = treeScale; }
    void setActive(bool active)                 { _active = active; }
    void setCreatePXActor(bool create)          { _addToPhysics = create; }
    void setChunkSize(U32 size)                 { _chunkSize = size; }
    void set16Bit(bool state)                   { _is16Bit = state;}
    
    U8    getTextureLayerCount() const { return _textureLayers; }
    U32   getGrassDensity()      const { return _grassDensity; }
    U16   getTreeDensity()       const { return _treeDensity; }
    F32   getGrassScale()        const { return _grassScale; }
    F32   getTreeScale()         const { return _treeScale; }
    bool  getActive()            const { return _active; }
    bool  getCreatePXActor()     const { return _addToPhysics; }
    U32   getChunkSize()         const { return _chunkSize; }
    bool  is16Bit()              const { return _is16Bit; }

    const vec2<F32>& getAltitudeRange() const { return _altitudeRange; }
    const vec2<U16>& getDimensions()    const { return _dimensions; }
    const vec3<F32>& getPosition()	    const { return _position; }
    const vec2<F32>& getScale()         const { return _scale; }

    std::string getVariable(const std::string& name) {
        Unordered_map<std::string, std::string>::const_iterator it = _variables.find(name);
        if (it != _variables.end())
            return it->second;

        return "";
    }

    F32 getVariablef(const std::string& name){
        Unordered_map<std::string, F32>::const_iterator it = _variablesf.find(name);
        if (it != _variablesf.end())
            return it->second;

        return 0.0f;
    }

private:
    Unordered_map<std::string, std::string> _variables;
    Unordered_map<std::string, F32>         _variablesf;
    U32    _grassDensity;
    U32    _chunkSize;
    U16    _treeDensity;
    F32	   _grassScale;
    F32    _treeScale;
    bool   _is16Bit;
    bool   _active;
    bool   _addToPhysics;
    U8     _textureLayers;
    vec3<F32>   _position;
    vec2<F32>   _scale;
    vec2<F32>   _altitudeRange;
    vec2<U16>   _dimensions;
};

template<class T>
inline T TER_COORD(T x, T y, T w) { return ((y)*(w)+(x)); }

#endif