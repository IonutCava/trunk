/*
   Copyright (c) 2017 DIVIDE-Studio
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

#include "config.h"

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
class LightPool;
class ImpostorSphere;
class SceneRenderState;
/// A light object placed in the scene at a certain position
class Light : public SceneNode {
   public:
       struct ShadowProperties {
           vec4<U32> _arrayOffset;
           /// light viewProjection matrices
           mat4<F32> _lightVP[Config::Lighting::MAX_SPLITS_PER_LIGHT];
           /// light's position in world space
           vec4<F32> _lightPosition[Config::Lighting::MAX_SPLITS_PER_LIGHT];
           /// random float values (e.g. split distances)
           vec4<F32> _floatValues[Config::Lighting::MAX_SPLITS_PER_LIGHT];

           inline void set(const ShadowProperties& other) {
               _arrayOffset.set(other._arrayOffset);
               for (U8 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
                   _lightVP[i].set(other._lightVP[i]);
                   _lightPosition[i].set(other._lightPosition[i]);
                   _floatValues[i].set(other._floatValues[i]);
               }
           }
       };

    typedef vectorImpl<Light*> LightList;

    /// Create a new light assigned to the specified slot with the specified range
    /// @param slot = the slot the light is assigned to (as in OpenGL slot for example)
    /// @param range = the light influence range (for spot/point lights)
    explicit Light(ResourceCache& parentCache, const stringImpl& name, const F32 range, const LightType& type, LightPool& parentPool);
    virtual ~Light();

    /// Is the light a shadow caster?
    inline bool castsShadows() const { return _castsShadows; }

    inline F32 getRange() const { return _positionAndRange.w; }

    void setRange(F32 range);

    /// Get light diffuse colour
    inline void getDiffuseColour(vec3<F32>& colourOut) const {
        Util::ToFloatColour(_colour.rgb(), colourOut);
    }

    inline vec3<F32> getDiffuseColour() const {
        return Util::ToFloatColour(_colour.rgb());
    }

    void setDiffuseColour(const vec3<U8>& newDiffuseColour);

    inline void setDiffuseColour(const vec4<U8>& newDiffuseColour) {
        setDiffuseColour(newDiffuseColour.rgb());
    }

    inline void setDiffuseColour(const vec4<F32>& newDiffuseColour) {
        setDiffuseColour(newDiffuseColour.rgb());
    }

    inline void setDiffuseColour(const vec3<F32>& newDiffuseColour) {
        setDiffuseColour(Util::ToByteColour(newDiffuseColour));
    }

    /// Get light position for omni and spot or direction for a directional light
    inline vec3<F32> getPosition() const { return _positionAndRange.xyz(); }

    /// Get direction for spot lights
    inline vec3<F32> getSpotDirection() const { return _spotProperties.xyz(); }

    inline F32 getSpotAngle() const { return _spotProperties.w; }

    void setSpotAngle(F32 newAngle);

    inline F32 getSpotCosOuterConeAngle() const { return _spotCosOuterConeAngle; }

    void setSpotCosOuterConeAngle(F32 newCosAngle);

    /// Light state (on/off)
    inline void toggleEnabled() { setEnabled(!getEnabled()); }

    inline bool getEnabled() const { return _enabled; }

    /// Does this list cast shadows?
    inline void setCastShadows(const bool state) { _castsShadows = state; }

    /// Draw a sphere at the lights position
    /// The impostor has the range of the light's effect range and the diffuse
    /// colour as the light's diffuse property
    inline void setDrawImpostor(const bool state) { _drawImpostor = state; }

    /// Turn the light on/off
    inline void setEnabled(const bool state) { _enabled = state; }

    /// Get the light type. (see LightType enum)
    inline const LightType& getLightType() const { return _type; }

    /// Get a pointer to the light's impostor
    inline ImpostorSphere* const getImpostor() const { return _impostor.get(); }

    bool onRender(const RenderStagePass& renderStagePass) override;

    /// SceneNode concrete implementations
    bool unload() override;

    void updateBoundsInternal(SceneGraphNode& sgn);

    void sceneUpdate(const U64 deltaTime,
                     SceneGraphNode& sgn,
                     SceneState& sceneState) override;

    /*----------- Shadow Mapping-------------------*/
    /// Set the function used to generate shadows for this light
    inline ShadowMapInfo* getShadowMapInfo() const { return _shadowMapInfo; }

    void addShadowMapInfo(ShadowMapInfo* const shadowMapInfo);
    bool removeShadowMapInfo();

    void validateOrCreateShadowMaps(GFXDevice& context, SceneRenderState& sceneRenderState);
    virtual void generateShadowMaps(GFXDevice& context, U32 passIdx);

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
    friend class LightPool;
    template <typename T>
    friend class ImplResourceLoader;
    bool load(DELEGATE_CBK<void, Resource_ptr> onLoadCallback) override;
    void postLoad(SceneGraphNode& sgn) override;
    SceneGraphNode* getSGN() const { return _lightSGN; }
    /// Set light type
    /// @param type Directional/Spot/Omni (see LightType enum)
    inline void setLightType(LightType type) {
        _type = type;
    }

    /// Get a ref to the shadow camera used by this light
    Camera* const shadowCamera() const { return _shadowCamera; }
    void updateImpostor();
   protected:
    bool _spotPropertiesChanged;
    /// Used to generate spot light penumbra using D3D's dual-cone method
    F32 _spotCosOuterConeAngle;
    /// xyz - position/direction, w - range
    vec4<F32> _positionAndRange;
    /// xyz - direction, w - cone angle
    vec4<F32> _spotProperties;
    /// rgb - diffuse, w - reserved
    vec4<U8>  _colour;
    // does this light casts shadows?
    bool _castsShadows;
    // Shadow mapping properties
    ShadowProperties _shadowProperties;

    ShadowMapInfo* _shadowMapInfo;

    LightType _type;

   private:
    bool _rangeChanged;
    bool _drawImpostor;
    SceneGraphNode* _lightSGN;
    /// Used for debug rendering
    std::shared_ptr<ImpostorSphere> _impostor;
    SceneGraphNode_wptr _impostorSGN;
    Camera* _shadowCamera;
    LightPool& _parentPool;
    bool _enabled;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(Light);

};  // namespace Divide

#endif