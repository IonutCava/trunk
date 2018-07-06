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
	Light(U8 slot, F32 range, LightType type); 
	virtual ~Light();
	
	///Light range controls the distance in which all contained objects are lit by it
	void setRange(F32 range);
	inline I32  getScore() {return _score;}
	inline void setScore(I32 score) {_score = score;}
	///Set all light vector-properties
    virtual void setLightProperties(LightPropertiesV propName, const vec4<F32>& value);
	///Set all light float-properties
	virtual void setLightProperties(LightPropertiesF propName, F32 value);
	///Get light vector properties 
	inline const vec4<F32>& getVProperty(LightPropertiesV key) {return _lightProperties_v[key];}
	///Get light floating point properties 
	virtual F32 getFProperty(LightPropertiesF key);
	///Get light ID
	inline U32   getId() {return _id;}
	///Get light slot
	inline U8    getSlot() {return _slot;}
	///Is the light a shadow caster?
	inline bool  castsShadows() {return _castsShadows;}
	///Get light diffuse color
	inline const vec4<F32>& getDiffuseColor() {return _lightProperties_v[LIGHT_PROPERTY_DIFFUSE];}
	///Get light position for omni and spot or direction for a directional light (also accesible via the "getDirection" method of the "DirectionalLight" class
	inline vec3<F32>  getPosition() {return _position;}
		   void       setPosition(const vec3<F32>& newPosition);
	///Get direction for spot lights
	inline vec3<F32>  getDirection() {return _direction;}
		   void       setDirection(const vec3<F32>& newDirection);
	///Light effect range
	inline F32   getRange()   {return _lightProperties_f[LIGHT_PROPERTY_RANGE];}
	///Light state (on/off)
	inline bool  getEnabled()  {return _enabled;} 

	///Set a new light ID.
	///The light ID is not the slot! It is used simply to identify the lights uniquely
	inline void  setId(U32 id) {_id = id; _dirty = true;}
	///Set the light slot.
	///Slot is used by the rendering API internally
	inline void  setSlot(U8 slot) {_slot = slot; _dirty = true;}
	///Does this list cast shadows?
	inline void  setCastShadows(bool state) {_castsShadows = state;}
	///Draw a sphere at the lights position
	///The impostor has the range of the light's effect range and the diffuse color as the light's diffuse property
	inline void  setDrawImpostor(bool state) {_drawImpostor = state;}
	///Turn the light on/off
	inline void  setEnabled(bool state) {_enabled = state; _dirty = true;}
	///Get the light type.
	///See LightType enum
	inline LightType getLightType() {return _type;}
	inline LightMode getLightMode() {return _mode;}
	///Get a pointer to the light's imposter
	inline Impostor* getImpostor() {return _impostor;}
	//Checks if this light needs and update
	void updateState(bool force = false);

	///Dummy function from SceneNode;
	void onDraw() {};

	///SceneNode concrete implementations
	bool unload();
	void postLoad(SceneGraphNode* const sgn);	
    bool computeBoundingBox(SceneGraphNode* const sgn);
    bool isInView(bool distanceCheck,BoundingBox& boundingBox,const BoundingSphere& sphere);
    void updateBBatCurrentFrame(SceneGraphNode* const sgn);
	///When the SceneGraph calls the light's render function, we draw the impostor if needed
	virtual void render(SceneGraphNode* const sgn);
	 
	virtual void setCameraToLightView(const vec3<F32>& eyePos) = 0;
	        void setCameraToSceneView();

	///SceneNode test
	bool isInView(bool distanceCheck,BoundingBox& boundingBox){return _drawImpostor;}

	/*----------- Shadow Mapping-------------------*/
	///Set the function used to generate shadows for this light (usually _scenegraph->render)
	inline ShadowMapInfo* getShadowMapInfo() {return _shadowMapInfo;}
	void setShadowMappingCallback(boost::function0<void> callback);
	void addShadowMapInfo(ShadowMapInfo* shadowMapInfo);
	bool removeShadowMapInfo();
	inline const mat4<F32 >& getLightProjectionMatrix() {return _lightProjectionMatrix;}
	///We need to render the scene from the light's perspective
    ///This varies depending on the current pass:
    ///--The higher the pass, the closer the view
	virtual void renderFromLightView(U8 depthPass) = 0;
	virtual void generateShadowMaps(SceneRenderState* renderState);

protected:
	///Set light type
	///@param type Directional/Spot/Omni (see LightType enum)
	inline void  setLightType(LightType type) {_type = type;}
	///Set light mode
	///@param mode Togglable, Movable, Simple, Dominant (see LightMode enum)
	void setLightMode(LightMode mode);
private:
	///Enum to char* translation for vector properties
	const char* LightEnum(LightPropertiesV key);
	///Enum to char* translation for floating point properties
	const char* LightEnum(LightPropertiesF key);
protected:
	U32  _id;
	LightType _type;
	LightMode _mode;
	vec3<F32 > _position; ///<Position is a direction for directional lights
	vec3<F32 > _direction; ///<Used by spot lights;
	Unordered_map<LightPropertiesV,vec4<F32>> _lightProperties_v;
	Unordered_map<LightPropertiesF,F32> _lightProperties_f;

private:

	U8   _slot;
	bool _drawImpostor, _castsShadows;
	Impostor* _impostor; ///< Used for debug rendering -Ionut
	SceneGraphNode *_lightSGN, *_impostorSGN;
	boost::function0<void> _callback;
	I32   _score;
	bool  _dirty;
    bool  _enabled;

protected:
	mat4<F32> _lightProjectionMatrix;
	vec3<F32> _lightPos;
	vec2<F32> _zPlanes;
	vec3<F32> _eyePos;
	ShadowMapInfo* _shadowMapInfo;
	ParamHandler& _par;
};

#endif