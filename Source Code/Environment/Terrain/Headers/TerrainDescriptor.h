/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _TERRAIN_DESCRIPTOR_H_
#define _TERRAIN_DESCRIPTOR_H_

#include "Core/Resources/Headers/ResourceDescriptor.h"

namespace Divide {

class TerrainDescriptor final : public PropertyDescriptor {
   public:
    explicit TerrainDescriptor(const stringImpl& name) noexcept
        : PropertyDescriptor(PropertyDescriptor::DescriptorType::DESCRIPTOR_TERRAIN_INFO)
    {
    }

    ~TerrainDescriptor()
    {
        _variables.clear();
    }

    TerrainDescriptor* clone() const {
        return MemoryManager_NEW TerrainDescriptor(*this);
    }

    void addVariable(const stringImpl& name, const stringImpl& value) {
        hashAlg::insert(_variables, hashAlg::make_pair(_ID(name.c_str()), value));
    }

    void addVariable(const stringImpl& name, F32 value) {
        hashAlg::insert(_variablesf, hashAlg::make_pair(_ID(name.c_str()), value));
    }

    void setTextureLayerCount(U8 count) { _textureLayers = count; }
    void setDimensions(const vec2<U16>& dim) { _dimensions = dim; }
    void setAltitudeRange(const vec2<F32>& dim) { _altitudeRange = dim; }
    void setGrassDensity(F32 grassDensity) { _grassDensity = grassDensity; }
    void setTreeDensity(F32 treeDensity) { _treeDensity = treeDensity; }
    void setGrassScale(F32 grassScale) { _grassScale = grassScale; }
    void setTreeScale(F32 treeScale) { _treeScale = treeScale; }
    void setActive(bool active) { _active = active; }
    void setChunkSize(U32 size) { _chunkSize = size; }
    void set16Bit(bool state) { _is16Bit = state; }

    U8 getTextureLayerCount() const { return _textureLayers; }
    F32 getGrassDensity() const { return _grassDensity; }
    F32 getTreeDensity() const { return _treeDensity; }
    F32 getGrassScale() const { return _grassScale; }
    F32 getTreeScale() const { return _treeScale; }
    bool getActive() const { return _active; }
    U32 getChunkSize() const { return _chunkSize; }
    bool is16Bit() const { return _is16Bit; }

    const vec2<F32>& getAltitudeRange() const { return _altitudeRange; }
    const vec2<U16>& getDimensions() const { return _dimensions; }

    stringImpl getVariable(const stringImpl& name) const {
        hashMap<U64, stringImpl>::const_iterator it = _variables.find(_ID(name.c_str()));
        if (it != std::end(_variables)) {
            return it->second;
        }
        return "";
    }

    F32 getVariablef(const stringImpl& name) const {
        hashMap<U64, F32>::const_iterator it = _variablesf.find(_ID(name.c_str()));
        if (it != std::end(_variablesf)) {
            return it->second;
        }
        return 0.0f;
    }

    inline size_t getHash() const override {
        size_t hash = 0;
        for (hashMap<U64, stringImpl>::value_type it : _variables) {
            Util::Hash_combine(hash, it.first);
            Util::Hash_combine(hash, it.second);
        }
        for (hashMap<U64, F32>::value_type it : _variablesf) {
            Util::Hash_combine(hash, it.first);
            Util::Hash_combine(hash, it.second);
        }
        Util::Hash_combine(hash, _grassDensity);
        Util::Hash_combine(hash, _chunkSize);
        Util::Hash_combine(hash, _treeDensity);
        Util::Hash_combine(hash, _grassScale);
        Util::Hash_combine(hash, _treeScale);
        Util::Hash_combine(hash, _is16Bit);
        Util::Hash_combine(hash, _active);
        Util::Hash_combine(hash, _textureLayers);
        Util::Hash_combine(hash, _altitudeRange.x);
        Util::Hash_combine(hash, _altitudeRange.y);
        Util::Hash_combine(hash, _dimensions.x);
        Util::Hash_combine(hash, _dimensions.y);
        Util::Hash_combine(hash, PropertyDescriptor::getHash());

        return hash;
    }

   private:
    /// chunk size defines the dimensions of a quadtree chunk of vertices in the format NxN
    /// e.g.Specifying a 256 chunk size for a 1024x1024 terrain will generate 16 leafs in the quadtree
    U32 _chunkSize = 0;

    hashMap<U64, stringImpl> _variables;
    hashMap<U64, F32> _variablesf;
    F32 _grassDensity = 0.0f;
    F32 _treeDensity = 0.0f;
    F32 _grassScale = 1.0f;
    F32 _treeScale = 1.0f;
    bool _is16Bit = false;
    bool _active = false;
    U8 _textureLayers = 1;
    vec2<F32> _altitudeRange = {0.f, 1.f};
    vec2<U16> _dimensions = {1.f, 1.f};
};

};  // namespace Divide

#endif