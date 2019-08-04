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
#include "Utility/Headers/XMLParser.h"

namespace Divide {

class TerrainDescriptor final : public PropertyDescriptor {
   public:
	   enum class WireframeMode : U8 {
		   NONE = 0,
		   EDGES,
		   NORMALS
	   };

	   enum class ParallaxMode : U8 {
		   NONE = 0,
		   NORMAL,
		   OCCLUSION
	   };
   public:
    explicit TerrainDescriptor(const stringImpl& name) noexcept;

    ~TerrainDescriptor();

    bool loadFromXML(const boost::property_tree::ptree& pt, const stringImpl& name);

    void clone(std::shared_ptr<PropertyDescriptor>& target) const override;

    void addVariable(const stringImpl& name, const stringImpl& value);

    void addVariable(const stringImpl& name, F32 value);

    void setTextureLayerCount(U8 count) noexcept { _textureLayers = count; }
    void setDimensions(const vec2<U16>& dim) noexcept { _dimensions = dim; }
    void setAltitudeRange(const vec2<F32>& dim) noexcept { _altitudeRange = dim; }
    void setTessellationRange(const vec4<F32>& rangeChunkAndPatch) noexcept { _tessellationRange = rangeChunkAndPatch; }
    void setActive(bool active) noexcept { _active = active; }
    void setWireframeDebug(U8 state) noexcept { _wireframeDebug = static_cast<WireframeMode>(CLAMPED(to_I32(state), 0, 2)); }
	void setParallaxMode(U8 state) noexcept { _parallaxMode = static_cast<ParallaxMode>(CLAMPED(to_I32(state), 0, 2)); }

    U8 getTextureLayerCount() const noexcept { return _textureLayers; }
    bool getActive() const noexcept { return _active; }
	WireframeMode wireframeDebug() const noexcept { return _wireframeDebug; }
	ParallaxMode parallaxMode() const noexcept { return _parallaxMode; }

    const vec2<F32>& getAltitudeRange() const noexcept { return _altitudeRange; }
    const vec4<F32>& getTessellationRange() const noexcept { return _tessellationRange; }
    const vec2<U16>& getDimensions() const noexcept { return _dimensions; }

    stringImpl getVariable(const stringImpl& name) const {
        const hashMap<U64, stringImpl>::const_iterator it = _variables.find(_ID(name.c_str()));
        if (it != std::end(_variables)) {
            return it->second;
        }
        return "";
    }

    F32 getVariablef(const stringImpl& name) const {
        const hashMap<U64, F32>::const_iterator it = _variablesf.find(_ID(name.c_str()));
        if (it != std::end(_variablesf)) {
            return it->second;
        }
        return 0.0f;
    }

    inline size_t getHash() const override {
        size_t hash = 17;
        for (hashMap<U64, stringImpl>::value_type it : _variables) {
            Util::Hash_combine(hash, it.first);
            Util::Hash_combine(hash, it.second);
        }
        for (hashMap<U64, F32>::value_type it : _variablesf) {
            Util::Hash_combine(hash, it.first);
            Util::Hash_combine(hash, it.second);
        }
        Util::Hash_combine(hash, _active);
        Util::Hash_combine(hash, _textureLayers);
        Util::Hash_combine(hash, _altitudeRange.x);
        Util::Hash_combine(hash, _altitudeRange.y);
        Util::Hash_combine(hash, _tessellationRange.x);
        Util::Hash_combine(hash, _tessellationRange.y);
        Util::Hash_combine(hash, _tessellationRange.z);
        Util::Hash_combine(hash, _tessellationRange.w);
        Util::Hash_combine(hash, _dimensions.x);
        Util::Hash_combine(hash, _dimensions.y);
		Util::Hash_combine(hash, to_base(_wireframeDebug));
		Util::Hash_combine(hash, to_base(_parallaxMode));
        Util::Hash_combine(hash, PropertyDescriptor::getHash());

        return hash;
    }

   private:
    hashMap<U64, stringImpl> _variables;
    hashMap<U64, F32> _variablesf;
    vec4<F32> _tessellationRange = { 10.0f, 150.0f, 32.0f, 100.0f };
    vec2<F32> _altitudeRange = { 0.f, 1.f };
    vec2<U16> _dimensions = { 1.f, 1.f };
    U8 _textureLayers = 1;
	WireframeMode _wireframeDebug = WireframeMode::NONE;
	ParallaxMode _parallaxMode = ParallaxMode::NONE;
    bool _active = false;
};

};  // namespace Divide

#endif