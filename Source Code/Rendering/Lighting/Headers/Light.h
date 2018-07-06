/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef DIVIDE_LIGHT_H_
#define DIVIDE_LIGHT_H_

#include "Graphs/Headers/SceneNode.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"

namespace Divide {

/// The different types of lights supported
enum class LightType : U32 {
    DIRECTIONAL = 0,
    POINT = 1,
    SPOT = 2,
    COUNT
};

class Camera;
class ImpostorSphere;
class ParamHandler;
class SceneRenderState;
/// A light object placed in the scene at a certain position
class Light : public SceneNode {
   public:
       struct ShadowProperties {
           /// light viewProjection matrices
           mat4<F32> _lightVP[Config::Lighting::MAX_SPLITS_PER_LIGHT];
           /// light's position in world space
           vec4<F32> _lightPosition[Config::Lighting::MAX_SPLITS_PER_LIGHT];
           /// random float values (e.g. split distances)
           vec4<F32> _floatValues[Config::Lighting::MAX_SPLITS_PER_LIGHT];
           vec4<U32> _arrayOffset;

           inline void set(const ShadowProperties& other) {
               for (U8 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
                   _lightVP[i].set(other._lightVP[i]);
                   _lightPosition[i].set(other._lightPosition[i]);
                   _floatValues[i].set(other._floatValues[i]);
                   _arrayOffset.set(other._arrayOffset);
               }
           }
       };

    typedef vectorImpl<Light*> LightList;

    /// Create a new light assigned to the specified slot with the specified range
    /// @param slot = the slot the light is assigned to (as in OpenGL slot for example)
    /// @param range = the light influence range (for spot/point lights)
    Light(const F32 range, const LightType& type);
    virtual ~Light();

    /// Is the light a shadow caster?
    inline bool castsShadows() const { return _castsShadows; }

    inline F32 getRange() const { return _positionAndRange.w; }

    void setRange(F32 range);

    /// Get light diffuse color
    inline vec3<F32> getDiffuseColor() const {
        return Util::ToFloatColor(_color.rgb());
    }

    void setDiffuseColor(const vec3<U8>& newDiffuseColor);
    inline void setDiffuseColor(const vec3<F32>& newDiffuseColor) {
        setDiffuseColor(Util::ToByteColor(newDiffuseColor));
    }

    /// Get light position for omni and spot or direction for a directional light
    inline vec3<F32> getPosition() const { return _positionAndRange.xyz(); }

    /// Get direction for spot lights
    inline vec3<F32> getSpotDirection() const { return _spotProperties.xyz(); }

    void setSpotDirection(const vec3<F32>& newDirection);

    inline F32 getSpotAngle() const { return _spotProperties.w; }

    void setSpotAngle(F32 newAngle);
    
    /// Light state (on/off)
    inline bool getEnabled() const { return _enabled; }

    /// Does this list cast shadows?
    inline void setCastShadows(const bool state) { _castsShadows = state; }

    /// Draw a sphere at the lights position
    /// The impostor has the range of the light's effect range and the diffuse
    /// color as the light's diffuse property
    inline void setDrawImpostor(const bool state) { _drawImpostor = state; }

    /// Turn the light on/off
    inline void setEnabled(const bool state) { _enabled = state; }

    /// Get the light type. (see LightType enum)
    inline const LightType& getLightType() const { return _type; }

    /// Get a pointer to the light's impostor
    inline ImpostorSphere* const getImpostor() const { return _impostor; }

    /// Checks if this light needs and update
    void onCameraUpdate(Camera& camera);

    bool onDraw(SceneGraphNode& sgn, RenderStage currentStage) override;

    /// SceneNode concrete implementations
    bool unload() override;

    bool computeBoundingBox(SceneGraphNode& sgn);

    bool isInView(const SceneRenderState& sceneRenderState,
                  SceneGraphNode& sgn, const bool distanceCheck = true);

    void sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                     SceneState& sceneState);

    /*----------- Shadow Mapping-------------------*/
    /// Set the function used to generate shadows for this light
    inline ShadowMapInfo* getShadowMapInfo() const { return _shadowMapInfo; }

    void addShadowMapInfo(ShadowMapInfo* const shadowMapInfo);
    bool removeShadowMapInfo();

    void validateOrCreateShadowMaps(SceneRenderState& sceneRenderState);
    virtual void generateShadowMaps(SceneRenderState& sceneRenderState);

    inline const ShadowProperties& getShadowProperties() const {
        return _shadowProperties;
    }

    inline const mat4<F32>& getShadowVPMatrix(U8 index) const {
        return _shadowProperties._lightVP[index];
    }

    inline const vec4<F32>& getShadowFloatValues(U8 index) const {
        return _shadowProperties._floatValues[index];
    }

    inline const vec4<F32>& getShadowLightPos(U8 index) const {
        return _shadowProperties._lightPosition[index];
    }

    inline void setShadowVPMatrix(U8 index, const mat4<F32>& newValue) {
        _shadowProperties._lightVP[index].set(newValue);
    }

    inline void setShadowFloatValue(U8 index, F32 newValue) {
        _shadowProperties._floatValues[index].set(newValue);
    }

    inline void setShadowFloatValue(U8 index, const vec4<F32>& newValue) {
        _shadowProperties._floatValues[index].set(newValue);
    }

    inline void setShadowLightPos(U8 index, const vec3<F32>& newValue) {
        _shadowProperties._lightPosition[index].set(newValue, 1.0f);
    }

   protected:
    friend class LightManager;
    template <typename T>
    friend class ImplResourceLoader;
    bool load(const stringImpl& name);
    void postLoad(SceneGraphNode& sgn) override;

    /// Set light type
    /// @param type Directional/Spot/Omni (see LightType enum)
    inline void setLightType(LightType type) {
        _type = type;
    }

    /// Get a ref to the shadow camera used by this light
    Camera* const shadowCamera() const { return _shadowCamera; }
    
   protected:
    /// xyz - position/direction, w - range
    vec4<F32> _positionAndRange;
    /// xyz - direction, w - cone angle
    vec4<F32> _spotProperties;
    /// rgb - diffuse, w - reserved
    vec4<U8>  _color;
    // does this light casts shadows?
    bool _castsShadows;
    // Shadow mapping properties
    ShadowProperties _shadowProperties;

    ShadowMapInfo* _shadowMapInfo;

    LightType _type;

   private:
    bool _drawImpostor;
    SceneGraphNode* _lightSGN;
    /// Used for debug rendering
    ImpostorSphere* _impostor;
    SceneGraphNode_wptr _impostorSGN;
    Camera* _shadowCamera;
    bool _enabled;
};

};  // namespace Divide

#endif