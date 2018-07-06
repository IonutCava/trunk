/*“Copyright 2009-2012 DIVIDE-Studio”*/
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
enum LIGHT_TYPE{
	LIGHT_OMNI = 0,
	LIGHT_DIRECTIONAL = 1,
	LIGHT_SPOT = 2
};

///Defines all light properties that are stores as a 3 or 4 component vector
enum LIGHT_V_PROPERTIES{
	LIGHT_POSITION,
	LIGHT_DIFFUSE,
	LIGHT_AMBIENT,
	LIGHT_SPECULAR,
	LIGHT_SPOT_DIRECTION
};
///Defines all light properties that are stored as a floating point value
enum LIGHT_F_PROPERTIES{
	LIGHT_SPOT_EXPONENT,
	LIGHT_SPOT_CUTOFF,
	LIGHT_CONST_ATT,
	LIGHT_LIN_ATT,
	LIGHT_QUAD_ATT
};

class Impostor;
class ParamHandler;
class FrameBufferObject;
///A light object placed in the scene at a certain position
class Light : public SceneNode {

public:
	///Create a new light assing to the specified slot with the specified radius
	/// @param slot = the slot the light is assinged to (as in OpenGL slot for example)
	/// @param radius = the light influence radius (for spot/point lights)
	Light(U8 slot, F32 radius = 2); 
	~Light();
	
	///Light radius controls the distance in which all contained objects are lit by it
	void setRadius(F32 radius);
	///Set all light vector-properties
	void setLightProperties(const LIGHT_V_PROPERTIES& propName, const vec4<F32>& value);
	///Set all light float-properties
	void setLightProperties(const LIGHT_F_PROPERTIES& propName, F32 value);
	///Set the function used to generate shadows for this light (usually _scenegraph->render)
	void setShadowMappingCallback(boost::function0<void> callback);
	///Get light vector properties 
    inline vec4<F32>& getVProperty(const LIGHT_V_PROPERTIES& key){
		return _lightProperties_v[key];
	}
	///Get light floating point properties 
	 inline F32& getFProperty(const LIGHT_F_PROPERTIES& key){
		return _lightProperties_f[key];
	}

	///Get light ID
	inline U32   getId() {return _id;}
	///Get light slot
	inline U8    getSlot() {return _slot;}
	///Get light diffuse color
	inline vec4<F32>& getDiffuseColor() {return _lightProperties_v[LIGHT_DIFFUSE];}
	///Get light position (or direction for directional lights)
	inline vec4<F32>& getPosition() {return  _lightProperties_v[LIGHT_POSITION];}
	///Light effect radius
	inline F32   getRadius()   {return _radius;}
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
	///Set light type
	///@param type Directional/Spot/Omni (see LIGHT_TYPE enum)
	inline void  setLightType(LIGHT_TYPE type) {_type = type; _dirty = true;}
	///Draw a sphere at the lights position
	///The impostor has the radius of the light's effect radius and the diffuse color as the light's diffuse property
	inline void  setDrawImpostor(bool state) {_drawImpostor = state;}
	///Turn the light on/off
	inline void  setEnabled(bool state) {_enabled = state; _dirty = true;}
	///Get the light type.
	///See LIGHT_TYPE enum
	inline LIGHT_TYPE getLightType() {return _type;}
	///Get a pointer to the light's imposter
	inline Impostor* getImpostor() {return _impostor;}
	//Checks if this light needs and update
	void updateState(bool force = false);

	///Dummy function from SceneNode;
	void onDraw() {};

	///SceneNode concrete implementations
	bool unload();
	void postLoad(SceneGraphNode* const sgn);	

	///When the SceneGraph calls the light's render function, we draw the impostor if needed
	virtual void render(SceneGraphNode* const sgn);

	///Shadow Mapping (all virtual in case we need to expand each light type. Might do ...:
	inline std::vector<FrameBufferObject* >& getDepthMaps() {return _depthMaps;}
	virtual void generateShadowMaps();
	virtual void setCameraToSceneView();
	virtual void setCameraToLightView();
	virtual void setCameraToLightViewOmni();
	virtual void setCameraToLightViewSpot();
	virtual void setCameraToLightViewDirectional();
	virtual void renderFromLightView(U8 depthPass);
	virtual void renderFromLightViewOmni(U8 depthPass);
	virtual void renderFromLightViewSpot(U8 depthPass);
	virtual void renderFromLightViewDirectional(U8 depthPass);

	///SceneNode test
	bool isInView(bool distanceCheck,BoundingBox& boundingBox){return _drawImpostor;}

protected:
	LIGHT_TYPE _type;

private:
	unordered_map<LIGHT_V_PROPERTIES,vec4<F32>> _lightProperties_v;
	unordered_map<LIGHT_F_PROPERTIES,F32> _lightProperties_f;
	U8   _slot;
	U32  _id;
	F32  _radius;
	bool _drawImpostor, _castsShadows;
	Impostor* _impostor; ///< Used for debug rendering -Ionut
	SceneGraphNode *_lightSGN, *_impostorSGN;
	boost::function0<void> _callback;

	bool  _dirty;
    bool  _enabled;


private: //Shadow Mapping
	///The 3 depth maps
	std::vector<FrameBufferObject* > _depthMaps;
	F32 _resolutionFactor;
	///Temp variables used for caching data between function calls
	vec3<F32> _lightPos;
	vec2<F32> _zPlanes;
	vec3<F32> _eyePos;

	///cached values
	F32 _zNear;
	F32 _zFar;
	ParamHandler& _par;
};

#endif