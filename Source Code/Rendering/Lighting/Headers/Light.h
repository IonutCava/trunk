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

#ifndef DIVIDE_LIGHT_H_
#define DIVIDE_LIGHT_H_

#include "core.h"
#include "Graphs/Headers/SceneNode.h"

namespace Divide {

///The different types of lights supported
enum LightType{
    LIGHT_TYPE_DIRECTIONAL = 0,
    LIGHT_TYPE_POINT = 1, ///< or omni light, if you prefer
    LIGHT_TYPE_SPOT = 2,
    LightType_PLACEHOLDER
};

enum LightMode{
    LIGHT_MODE_SIMPLE = 0,    ///< normal light. Can't be moved or changed at runtime
    LIGHT_MODE_TOGGLABLE = 1, ///< can be switched on or off and can change brightness,color, range at runtime
    LIGHT_MODE_MOVABLE = 2,   ///< can change position at runtime / most expensive
    //LIGHT_MODE_DOMINANT = 3   ///< only shadow caster in scene
};

struct LightProperties {
    vec4<F32> _diffuse;     //< rgb = diffuse,  w = ambientFactor;
    vec4<F32> _attenuation; //< x = constAtt, y = linearAtt, z = quadraticAtt,  w = brightness
    vec4<F32> _position;    //< Position is a direction for directional lights. w = reserved
    vec4<F32> _direction;   //< xyz = Used by spot lights, w = spotExponent
    vec4<I32> _options;     //< x = light type: 0.0 - directional, 1.0  - point, 2.0 - spot, y = casts shadows, zw - reserved;
};

struct LightShadowProperties {
    mat4<F32> _lightVP[4];  //< light viewProjection matrices
    vec4<F32> _floatValues; //< random float values (e.g. split distances)
    vec4<F32> _lightPosition[4]; //<light's position in world space
};

class Camera;
class Impostor;
class ParamHandler;
class ShadowMapInfo;
class SceneRenderState;
///A light object placed in the scene at a certain position
class Light : public SceneNode {
public:
    enum PropertyType{
        PROPERTY_TYPE_VISUAL = 0,
        PROPERTY_TYPE_PHYSICAL = 1,
        PROPERTY_TYPE_SHADOW = 2,
        PropertyType_PLACEHOLDER = 3
    };

    typedef hashMapImpl<I64, Light*> LightMap;

    ///Create a new light assigned to the specified slot with the specified range
    /// @param slot = the slot the light is assigned to (as in OpenGL slot for example)
    /// @param range = the light influence range (for spot/point lights)
    Light(const F32 range,const LightType& type);
    virtual ~Light();

    ///Light score determines the importance of this light for the current node queries
    inline F32  getScore()                const {return _score;}
    inline void setScore(const F32 score)       {_score = score;}

    ///Is the light a shadow caster?
    inline bool  castsShadows() const {return _properties._options.y == 1;}
    ///Get the entire property block
    inline const LightProperties& getProperties() const { return _properties; }
    inline const LightShadowProperties& getShadowProperties() const {return _shadowProperties;}

    inline F32        getRange() const {return _properties._attenuation.w;}
           void       setRange(F32 range);
    ///Get light diffuse color
    inline vec3<F32>  getDiffuseColor() const { return _properties._diffuse.rgb(); }
           void       setDiffuseColor(const vec3<F32>& newDiffuseColor);
    ///Get light position for omni and spot or direction for a directional light
    inline vec3<F32>  getPosition()     const { return _properties._position.xyz(); }
           void       setPosition(const vec3<F32>& newPosition);
    ///Get direction for spot lights
    inline vec3<F32>  getDirection() const { return _properties._direction.xyz(); }
           void       setDirection(const vec3<F32>& newDirection);
    ///Light state (on/off)
    inline bool  getEnabled() const {return _enabled;}

    ///Does this list cast shadows?
    inline void  setCastShadows(const bool state) { _properties._options.y = (state ? 1 : 0);}
    ///Draw a sphere at the lights position
    ///The impostor has the range of the light's effect range and the diffuse color as the light's diffuse property
    inline void  setDrawImpostor(const bool state) {_drawImpostor = state;}
    ///Turn the light on/off
    inline void  setEnabled(const bool state) {_enabled = state;}
    ///Get the light type.
    ///See LightType enum
    inline const LightType& getLightType() const {return _type;}
    inline const LightMode& getLightMode() const {return _mode;}
    ///Get a pointer to the light's impostor
    inline Impostor* const getImpostor() const {return _impostor;}
    //Checks if this light needs and update
    void onCameraChange();

    ///Dummy function from SceneNode;
    bool onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage) { return true; }

    ///SceneNode concrete implementations
    bool unload();
    bool computeBoundingBox(SceneGraphNode* const sgn);
    bool isInView( const SceneRenderState& sceneRenderState, SceneGraphNode* const sgn, const bool distanceCheck = true );
    void sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);

    /*----------- Shadow Mapping-------------------*/
    ///Set the function used to generate shadows for this light (usually _scenegraph->render)
    inline ShadowMapInfo* getShadowMapInfo() const {return _shadowMapInfo;}
	inline void setShadowMappingCallback(const DELEGATE_CBK<>& callback) { _callback = callback; }
    void addShadowMapInfo(ShadowMapInfo* const shadowMapInfo);
    bool removeShadowMapInfo();
    virtual void generateShadowMaps(SceneRenderState& sceneRenderState);
    inline const mat4<F32>& getShadowVPMatrix(U8 index)   const { return _shadowProperties._lightVP[index]; }
    inline const vec4<F32>& getShadowFloatValues()        const { return _shadowProperties._floatValues; }
    inline const vec4<F32>& getShadowLightPos(U8 index)   const { return _shadowProperties._lightPosition[index]; }
    inline       void       setShadowVPMatrix(U8 index, const mat4<F32>& newValue)   { _shadowProperties._lightVP[index].set(newValue);  }
    inline       void       setShadowFloatValue(U8 index, F32 newValue)              { _shadowProperties._floatValues[index] = newValue; }
    inline       void       setShadowLightPos(U8 index, const vec3<F32>& newValue)   { _shadowProperties._lightPosition[index].set(newValue, 1.0f); }
    inline void shadowMapResolutionFactor(U8 factor)       {_resolutionFactor = factor;}
    inline U8   shadowMapResolutionFactor()          const {return _resolutionFactor;}

protected:
    friend class LightManager;
    template<typename T>
    friend class ImplResourceLoader;
    bool load(const stringImpl& name);

    ///When the SceneGraph calls the light's render function, we draw the impostor if needed
    virtual void render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage);
    virtual void getDrawCommands(SceneGraphNode* const sgn, const RenderStage& currentRenderStage, SceneRenderState& sceneRenderState, vectorImpl<GenericDrawCommand>& drawCommandsOut) {}
    void postLoad(SceneGraphNode* const sgn);
    ///Set light type
    ///@param type Directional/Spot/Omni (see LightType enum)
    inline void  setLightType(const LightType& type) { _type = type; _properties._options.x = getLightTypeValue(); }
    ///Set light mode
    ///@param mode Togglable, Movable, Simple, Dominant (see LightMode enum)
    void setLightMode(const LightMode& mode);
    ///Called when the rendering resolution changes
    void updateResolution(I32 newWidth, I32 newHeight);
    ///Get a ref to the shadow camera used by this light
    Camera* const shadowCamera() const {return _shadowCamera;}
    const I32 getLightTypeValue() const { return _type == LIGHT_TYPE_DIRECTIONAL ? 0 : (_type == LIGHT_TYPE_POINT ? 1 : 2); }

protected:
    LightProperties       _properties;
    LightShadowProperties _shadowProperties;

    LightType _type;
    LightMode _mode;
    bool      _dirty[PropertyType_PLACEHOLDER];

private:
    U8   _resolutionFactor;
    bool _drawImpostor;
    bool _updateLightBB;
    Impostor* _impostor; ///< Used for debug rendering -Ionut
    SceneGraphNode* _lightSGN;
    SceneGraphNode* _impostorSGN;
    Camera*         _shadowCamera;
	DELEGATE_CBK<> _callback;
    F32   _score;
    bool  _enabled;

protected:
    vec2<F32> _zPlanes;
    vec3<F32> _eyePos;
    mat4<F32> _lightViewMatrix;
    ShadowMapInfo* _shadowMapInfo;
    ParamHandler& _par;
};

}; //namespace Divide

#endif