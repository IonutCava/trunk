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

#ifndef DIVIDE_LIGHT_H_
#define DIVIDE_LIGHT_H_

#include "core.h"
#include "Graphs/Headers/SceneNode.h"

///The different types of lights supported
enum LightType{
    LIGHT_TYPE_DIRECTIONAL = 0,
    LIGHT_TYPE_POINT = 1, ///< or omni light, if you prefer
    LIGHT_TYPE_SPOT = 2,
    LIGHT_TYPE_PLACEHOLDER
};

enum LightMode{
    LIGHT_MODE_SIMPLE = 0,    ///< normal light. Can't be moved or changed at runtime
    LIGHT_MODE_TOGGLABLE = 1, ///< can be switched on or off and can change brightness,color, range at runtime
    LIGHT_MODE_MOVABLE = 2,   ///< can change position at runtime / most expensive
    LIGHT_MODE_DOMINANT = 3   ///< only shadow caster in scene
};

///Defines all light properties that are stores as a 3 or 4 component vector
enum LightPropertiesV{
    LIGHT_PROPERTY_DIFFUSE,
    LIGHT_PROPERTY_AMBIENT,
    LIGHT_PROPERTY_SPECULAR,
};
///Defines all light properties that are stored as a floating point value
enum LightPropertiesF{
    LIGHT_PROPERTY_SPOT_EXPONENT,
    LIGHT_PROPERTY_SPOT_CUTOFF,
    LIGHT_PROPERTY_CONST_ATT,
    LIGHT_PROPERTY_LIN_ATT,
    LIGHT_PROPERTY_QUAD_ATT,
    LIGHT_PROPERTY_BRIGHTNESS,
    LIGHT_PROPERTY_RANGE
};

struct LightProperties {
    vec4<F32> _position; ///<Position is a direction for directional lights. w-light type: 0.0 - directional, 1.0  - point 2.0 - spot
    vec4<F32> _direction; ///<Used by spot lights; w-component - padding
    vec4<F32> _ambient;
    vec4<F32> _diffuse;
    vec4<F32> _specular;
    vec4<F32> _attenuation; //x = constAtt, y = linearAtt, z = quadraticAtt, w = range
    F32 _spotExponent;
    F32 _spotCutoff;
    F32 _brightness;
    F32 _padding;
};

class Impostor;
class ParamHandler;
class FrameBufferObject;
class ShadowMapInfo;
class SceneRenderState;
///A light object placed in the scene at a certain position
class Light : public SceneNode {
public:
    ///Create a new light assing to the specified slot with the specified range
    /// @param slot = the slot the light is assinged to (as in OpenGL slot for example)
    /// @param range = the light influence range (for spot/point lights)
    Light(const U8 slot,const F32 range,const LightType& type);
    virtual ~Light();

    ///Light range controls the distance in which all contained objects are lit by it
    void setRange(const F32 range);
    ///Light score determines the importance of this light for the current node queries
    inline F32  getScore()                const {return _score;}
    inline void setScore(const F32 score)       {_score = score;}
    ///Set all light vector-properties
    virtual void setLightProperties(const LightPropertiesV& propName, const vec4<F32>& value);
    ///Set all light float-properties
    virtual void setLightProperties(const LightPropertiesF& propName, F32 value);
    ///Get light vector properties
    virtual const vec4<F32>& getVProperty(const LightPropertiesV& key) const;
    ///Get light floating point properties
    virtual F32 getFProperty(const LightPropertiesF& key) const;
    ///Get light ID
    inline U32   getId() const {return _id;}
    ///Get light slot
    inline U8    getSlot() const {return _slot;}
    ///Is the light a shadow caster?
    inline bool  castsShadows() const {return _castsShadows;}
    ///Get the entire property block
    inline const LightProperties& getProperties() const {return _properties;}
    ///Get light diffuse color
    inline const vec4<F32>& getDiffuseColor() const {return _properties._diffuse;}
    ///Get light position for omni and spot or direction for a directional light (also accesible via the "getDirection" method of the "DirectionalLight" class
    inline vec3<F32>  getPosition() const {return _properties._position.rgb();}
           void       setPosition(const vec3<F32>& newPosition);
    ///Get direction for spot lights
    inline vec3<F32>  getDirection() const {return _properties._direction.rgb();}
           void       setDirection(const vec3<F32>& newDirection);
    ///Light effect range
    inline F32   getRange()   const {return _properties._attenuation.w;}
    ///Light state (on/off)
    inline bool  getEnabled() const {return _enabled;}

    ///Set a new light ID.
    ///The light ID is not the slot! It is used simply to identify the lights uniquely
    inline void  setId(const U32 id) {_id = id; _dirty = true;}
    ///Set the light slot.
    ///Slot is used by the rendering API internally
    inline void  setSlot(const U8 slot) {_slot = slot; _dirty = true;}
    ///Does this list cast shadows?
    inline void  setCastShadows(const bool state) {_castsShadows = state;}
    ///Draw a sphere at the lights position
    ///The impostor has the range of the light's effect range and the diffuse color as the light's diffuse property
    inline void  setDrawImpostor(const bool state) {_drawImpostor = state;}
    ///Turn the light on/off
    inline void  setEnabled(const bool state) {_enabled = state; _dirty = true;}
    ///Get the light type.
    ///See LightType enum
    inline const LightType& getLightType() const {return _type;}
    inline const LightMode& getLightMode() const {return _mode;}
    ///Get a pointer to the light's imposter
    inline Impostor* const getImpostor() const {return _impostor;}
    //Checks if this light needs and update
    void updateState();

    ///Dummy function from SceneNode;
    void onDraw(const RenderStage& currentStage) {};

    ///SceneNode concrete implementations
    bool unload();
    void postLoad(SceneGraphNode* const sgn);
    bool computeBoundingBox(SceneGraphNode* const sgn);
    bool isInView(const bool distanceCheck,const BoundingBox& boundingBox, const BoundingSphere& sphere);
    void updateBBatCurrentFrame(SceneGraphNode* const sgn);
	virtual void updateTransform(SceneGraphNode* const sgn) {}
    ///When the SceneGraph calls the light's render function, we draw the impostor if needed
    virtual void render(SceneGraphNode* const sgn);

    virtual void setCameraToLightView(const vec3<F32>& eyePos) = 0;
            void setCameraToSceneView();

    /*----------- Shadow Mapping-------------------*/
    ///Set the function used to generate shadows for this light (usually _scenegraph->render)
    inline ShadowMapInfo* getShadowMapInfo() const {return _shadowMapInfo;}
    void setShadowMappingCallback(boost::function0<void> callback);
    void addShadowMapInfo(ShadowMapInfo* const shadowMapInfo);
    bool removeShadowMapInfo();
    inline const mat4<F32 >& getLightProjectionMatrix() const {return _lightProjectionMatrix;}
    ///We need to render the scene from the light's perspective
    ///This varies depending on the current pass:
    ///--The higher the pass, the closer the view
    ///sceneHalfExtent is the half extent of the world bounding box
    virtual void renderFromLightView(const U8 depthPass,const F32 sceneHalfExtent = 1) = 0;
    virtual void generateShadowMaps(const SceneRenderState& sceneRenderState);

protected:
    ///Set light type
    ///@param type Directional/Spot/Omni (see LightType enum)
    inline void  setLightType(const LightType& type) {_type = type;}
    ///Set light mode
    ///@param mode Togglable, Movable, Simple, Dominant (see LightMode enum)
    void setLightMode(const LightMode& mode);
private:
    ///Enum to char* translation for vector properties
    const char* LightEnum(const LightPropertiesV& key) const;
    ///Enum to char* translation for floating point properties
    const char* LightEnum(const LightPropertiesF& key) const;

protected:

    LightProperties  _properties;

    U32  _id;
    LightType _type;
    LightMode _mode;

private:

    U8   _slot;
    bool _drawImpostor, _castsShadows;
    Impostor* _impostor; ///< Used for debug rendering -Ionut
    SceneGraphNode *_lightSGN, *_impostorSGN;
    boost::function0<void> _callback;
    F32   _score;
    bool  _dirty;
    bool  _enabled;

protected:
    mat4<F32> _lightProjectionMatrix;
    mat4<F32> _bias;
    vec3<F32> _lightPos;
	vec3<F32> _lightPosInitial;
    vec2<F32> _zPlanes;
    vec3<F32> _eyePos;
    ShadowMapInfo* _shadowMapInfo;
    ParamHandler& _par;
};

#endif