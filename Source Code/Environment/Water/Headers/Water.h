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

#ifndef _WATER_PLANE_H_
#define _WATER_PLANE_H_

#include "core.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Buffers/FrameBufferObject/Headers/FrameBufferObject.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/RenderPass/Headers/Reflector.h"

class ShaderProgram;
class WaterPlane : public SceneNode, public Reflector{

public:
	WaterPlane();
	~WaterPlane(){}

	/// Resource inherited "unload"
	bool unload();
	bool setInitialData(const std::string& name);

	/// General SceneNode stuff
	void onDraw();
	void render(SceneGraphNode* const sgn);
	void postLoad(SceneGraphNode* const sgn);
	void prepareMaterial(SceneGraphNode const* const sgn);
	void releaseMaterial();
	bool getDrawState(RENDER_STAGE currentStage)  const;

	/// ToDo: check water visibility - Ionut
	bool isInView(bool distanceCheck,BoundingBox& boundingBox) {return true;}

	void setParams(F32 shininess, F32 noiseTile, F32 noiseFactor, F32 transparency);
	inline Quad3D*     getQuad()    {return _plane;}
	
	/// Reflector overwrite
	void updateReflection();
	/// Used for many things, such as culling switches, and underwater effects
	inline bool isCameraUnderWater() { return (_eyePos.y < _waterLevel); }
	/// Used to set rendering options for the water
	inline void setRenderingOptions(const vec3<F32> eyePos = vec3<F32>(0,0,0)) {_eyePos = eyePos;}

protected:
	template<typename T>
	friend class ImplResourceLoader;
	inline void setWaterNormalMap(Texture2D* const waterNM){_texture = waterNM;}
	inline void setShaderProgram(ShaderProgram* const shaderProg){_shader = shaderProg;}
	inline void setGeometry(Quad3D* const waterPlane){_plane = waterPlane;}

private:
	/// Bounding Box computation overwrite from SceneNode
	bool computeBoundingBox(SceneGraphNode* const sgn);

private:
	/// cached far plane value
	F32				   _farPlane;
	/// cached water level
	F32                _waterLevel;
	/// Last known camera position
	vec3<F32>          _eyePos;
	/// the water's "geometry"
	Quad3D*			   _plane;
	Texture2D*		   _texture;
	ShaderProgram* 	   _shader;
	Transform*         _planeTransform;
	SceneGraphNode*    _node;
	SceneGraphNode*    _planeSGN;
};

#endif