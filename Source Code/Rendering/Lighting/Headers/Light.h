/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#include "resource.h"
#include "Graphs/Headers/SceneNode.h"

enum LIGHT_TYPE{
	LIGHT_OMNI = 0,
	LIGHT_DIRECTIONAL = 1,
	LIGHT_SPOT = 2
};

class LightImpostor;
class FrameBufferObject;
class Light : public SceneNode {

public:
	Light(U8 slot, F32 radius = 0.5f); 
	~Light();
	void onDraw();
	void setLightProperties(const std::string& name, const vec4& value);
	void setLightProperties(const std::string& name, F32 value);
	void setShadowMappingCallback(boost::function0<void> callback);

	inline void  setId(U32 id) {_id = id;}
	inline vec4& getDiffuseColor() {return _lightProperties_v["diffuse"];}
	inline vec4& getPosition() {return  _lightProperties_v["position"];}
	inline F32   getRadius()   {return _radius;}
	inline void  setRadius(F32 radius) {_radius = radius;}
	inline U32   getId() {return _id;}
	inline void  setCastShadows(bool state) {_castsShadows = state;}
	inline void  setLightType(LIGHT_TYPE type) {_type = type;}
	inline LIGHT_TYPE getLightType() {return _type;}

	inline LightImpostor* getImpostor() {return _impostor;}
	inline void toggleImpostor(bool state) {_drawImpostor = state;}

	virtual void render(SceneGraphNode* const node);
	bool load(const std::string& name);
	bool unload();
	void postLoad(SceneGraphNode* const node);	


//Shadow Mapping (all virtual in case we need to expand each light type. Might do ...:
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

	//SceneNode test
	bool isInView(bool distanceCheck,BoundingBox& boundingBox){return _drawImpostor;}

protected:
	LIGHT_TYPE _type;

private:
	unordered_map<std::string,vec4> _lightProperties_v;
	unordered_map<std::string,F32> _lightProperties_f;
	U8   _slot;
	U32  _id;
	F32  _radius;
	bool _drawImpostor, _castsShadows;
	LightImpostor* _impostor; //Used for debug rendering -Ionut
	SceneGraphNode *_lightSGN, *_impostorSGN;
	boost::function0<void> _callback;

private: //Shadow Mapping
	//The 3 depth maps
	std::vector<FrameBufferObject* > _depthMaps;
	//Temp variables used for caching data between function calls
	vec3 _lightPos;
	vec2 _zPlanes;
	vec3 _eyePos;
};

#endif