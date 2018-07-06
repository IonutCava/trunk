/*�Copyright 2009-2013 DIVIDE-Studio�*/
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
#include "Hardware/Video/Buffers/FrameBufferObject/Headers/FrameBufferObject.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/RenderPass/Headers/Reflector.h"

class ShaderProgram;

class Texture;
typedef Texture Texture2D;

class WaterPlane : public SceneNode, public Reflector{
public:
	WaterPlane();
	~WaterPlane(){}

	/// Resource inherited "unload"
	bool unload();

	/// General SceneNode stuff
	void onDraw(const RenderStage& currentStage);
    void postDraw(const RenderStage& currentStage);
	void render(SceneGraphNode* const sgn);
	void postLoad(SceneGraphNode* const sgn);
	void prepareMaterial(SceneGraphNode* const sgn);
	void releaseMaterial();
    void prepareDepthMaterial(SceneGraphNode* const sgn){}
    void releaseDepthMaterial(){}
	bool getDrawState(const RenderStage& currentStage)  const;

	bool isInView(const bool distanceCheck,const BoundingBox& boundingBox,const BoundingSphere& sphere) {return true;}

	void setParams(F32 shininess, F32 noiseTile, F32 noiseFactor, F32 transparency);
	inline Quad3D*     getQuad()    {return _plane;}

	/// Reflector overwrite
	void updateReflection();
	/// Used for many things, such as culling switches, and underwater effects
	inline bool isPointUnderWater(const vec3<F32>& pos) { return (pos.y < _waterLevel); }

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
	U8                 _lightCount;
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
	bool               _reflectionRendering;
};

#endif