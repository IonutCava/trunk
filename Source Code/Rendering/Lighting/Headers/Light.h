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

#ifndef _DIVIDE_LIGHT_COMPONENT_H_
#define _DIVIDE_LIGHT_COMPONENT_H_

#include "config.h"

#include "ECS/Components/Headers/SGNComponent.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"

namespace Divide {

typedef std::array<Camera*, Config::Lighting::MAX_SPLITS_PER_LIGHT> ShadowCameraPool;

/// The different types of lights supported
enum class LightType : U8 {
    DIRECTIONAL = 0,
    POINT = 1,
    SPOT = 2,
    COUNT
};

class Camera;
class LightPool;
class SceneGraphNode;
class EditorComponent;
class SceneRenderState;
/// A light object placed in the scene at a certain position
class Light : public GUIDWrapper
{
   public:
       struct ShadowProperties {
           // x = light type,  y = csm split count, z = arrayOffset
           vec4<U32> _lightDetails;
           /// light viewProjection matrices
           mat4<F32> _lightVP[Config::Lighting::MAX_SPLITS_PER_LIGHT];
           /// light's position in world space
           vec4<F32> _lightPosition[Config::Lighting::MAX_SPLITS_PER_LIGHT];
           /// random float values (e.g. split distances)
           F32 _floatValues[Config::Lighting::MAX_SPLITS_PER_LIGHT];

           inline void set(const ShadowProperties& other) {
               _lightDetails.set(other._lightDetails);
               memcpy(_lightVP,       other._lightVP,       Config::Lighting::MAX_SPLITS_PER_LIGHT * sizeof(mat4<F32>));
               memcpy(_lightPosition, other._lightPosition, Config::Lighting::MAX_SPLITS_PER_LIGHT * sizeof(vec4<F32>));
               memcpy(_floatValues,   other._floatValues,   Config::Lighting::MAX_SPLITS_PER_LIGHT * sizeof(F32));
           }

           // Renderdoc:
           //uvec4 details; mat4 vp[6]; vec4 pos[6]; float f[6]
       };

    /// Create a new light assigned to the specified slot with the specified range
    explicit Light(SceneGraphNode& sgn, const F32 range, LightType type, LightPool& parentPool);
    virtual ~Light();

    /// Is the light a shadow caster?
    inline bool castsShadows() const { return _castsShadows; }
    /// Does this light cast shadows?
    inline void castsShadows(const bool state) { _castsShadows = state; }

    /// Get light diffuse colour
    inline void getDiffuseColour(vec3<F32>& colourOut) const {
        Util::ToFloatColour(_colour.rgb(), colourOut);
    }

    inline vec3<F32> getDiffuseColour() const {
        return Util::ToFloatColour(_colour.rgb());
    }

    void setDiffuseColour(const vec3<U8>& newDiffuseColour);

    inline void setDiffuseColour(const UColour& newDiffuseColour) {
        setDiffuseColour(newDiffuseColour.rgb());
    }

    inline void setDiffuseColour(const FColour& newDiffuseColour) {
        setDiffuseColour(newDiffuseColour.rgb());
    }

    inline void setDiffuseColour(const vec3<F32>& newDiffuseColour) {
        setDiffuseColour(Util::ToByteColour(newDiffuseColour));
    }

    inline F32 getRange() const { return _rangeAndCones.x; }
    inline F32 getConeAngle() const { return _rangeAndCones.y; }
    inline F32 getSpotCosOuterConeAngle() const { return _rangeAndCones.z; }

    inline void setRange(F32 range) { _rangeAndCones.x = range;  }
    inline void setConeAngle(F32 newAngle) { _rangeAndCones.y = newAngle; }
    inline void setSpotCosOuterConeAngle(F32 newCosAngle) { _rangeAndCones.z = newCosAngle; }

    /// Light state (on/off)
    inline void toggleEnabled() { setEnabled(!getEnabled()); }

    inline bool getEnabled() const { return _enabled; }

    /// Turn the light on/off
    inline void setEnabled(const bool state) { _enabled = state; }

    /// Get the light type. (see LightType enum)
    inline const LightType& getLightType() const { return _type; }

    /*----------- Shadow Mapping-------------------*/
     inline const ShadowProperties& getShadowProperties() const {
        return _shadowProperties;
    }

    inline const mat4<F32>& getShadowVPMatrix(U8 index) const {
        assert(index < Config::Lighting::MAX_SPLITS_PER_LIGHT);

        return _shadowProperties._lightVP[index];
    }

    inline F32 getShadowFloatValues(U8 index) const {
        assert(index < Config::Lighting::MAX_SPLITS_PER_LIGHT);

        return _shadowProperties._floatValues[index];
    }

    inline const vec4<F32>& getShadowLightPos(U8 index) const {
        assert(index < Config::Lighting::MAX_SPLITS_PER_LIGHT);

        return _shadowProperties._lightPosition[index];
    }

    inline U16 getShadowOffset() const {
        return to_U16(_shadowProperties._lightDetails.z);
    }

    inline void setShadowVPMatrix(U8 index, const mat4<F32>& newValue) {
        assert(index < Config::Lighting::MAX_SPLITS_PER_LIGHT);

        _shadowProperties._lightVP[index].set(newValue);
    }

    inline void setShadowFloatValue(U8 index, F32 newValue) {
        assert(index < Config::Lighting::MAX_SPLITS_PER_LIGHT);

        _shadowProperties._floatValues[index] = newValue;
    }

    inline void setShadowLightPos(U8 index, const vec3<F32>& newValue) {
        _shadowProperties._lightPosition[index].set(newValue, 1.0f);
    }

    inline void setShadowOffset(U16 offset) {
        _shadowProperties._lightDetails.z = offset;
    }

    inline ShadowCameraPool& shadowCameras() { return _shadowCameras; }
    inline const ShadowCameraPool& shadowCameras() const { return _shadowCameras; }

    inline       SceneGraphNode& getSGN()       { return _sgn; }
    inline const SceneGraphNode& getSGN() const { return _sgn; }

    vec3<F32> getPosition() const;
    vec3<F32> getDirection() const;
   protected:
    SceneGraphNode& _sgn;
    /// x - range, y = iner cone, z - cos outer cone
    vec3<F32> _rangeAndCones;
    /// rgb - diffuse, a - reserved
    UColour  _colour;
    // does this light casts shadows?
    bool _castsShadows;
    // Shadow mapping properties
    ShadowProperties _shadowProperties;

    LightType _type;

   private:
    ShadowCameraPool _shadowCameras;
    LightPool& _parentPool;
    bool _enabled;
};

};  // namespace Divide

#endif //_DIVIDE_LIGHT_COMPONENT_H_