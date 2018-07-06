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

#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include "Graphs/Headers/SceneNode.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Environment/Vegetation/Headers/Vegetation.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

class Quad3D;
class Quadtree;
class ShaderProgram;
class TerrainDescriptor;
class VertexBufferObject;

class Terrain : public SceneNode {

   enum TerrainTextureUsage{
	  TERRAIN_TEXTURE_DIFFUSE = 0,
	  TERRAIN_TEXTURE_NORMALMAP = 1,
	  TERRAIN_TEXTURE_CAUSTICS = 2,
	  TERRAIN_TEXTURE_RED = 3,
	  TERRAIN_TEXTURE_GREEN = 4,
	  TERRAIN_TEXTURE_BLUE = 5,
	  TERRAIN_TEXTURE_ALPHA = 6
  };

  typedef Unordered_map<TerrainTextureUsage, Texture2D*> TerrainTextureMap;

public:

	Terrain();
	~Terrain();

	bool unload();
	
	void drawGround() const;
	void drawInfinitePlain();
	void render(SceneGraphNode* const sgn);
	void postDraw();
	void prepareMaterial(SceneGraphNode* const sgn);
	void releaseMaterial();
	void sceneUpdate(D32 sceneTime);
	void drawBoundingBox(SceneGraphNode* const sgn);

	inline void toggleBoundingBoxes(){ _drawBBoxes = !_drawBBoxes; }

	vec3<F32>  getPosition(F32 x_clampf, F32 z_clampf) const;
	vec3<F32>  getNormal(F32 x_clampf, F32 z_clampf) const;
	vec3<F32>  getTangent(F32 x_clampf, F32 z_clampf) const;
	vec2<F32>  getDimensions(){return vec2<F32>((F32)_terrainWidth, (F32)_terrainHeight);}

		   void  terrainSmooth(F32 k);
		   void  postLoad(SceneGraphNode* const sgn);	
		   void  initializeVegetation(TerrainDescriptor* const terrain,SceneGraphNode* const terrainSGN);

    inline Quadtree&         getQuadtree()   const {return *_terrainQuadtree;}
	inline Vegetation* const getVegetation() const {return _veg;}
	inline void addVegetation(Vegetation* veg, std::string grassShader){_veg = veg; _grassShader = grassShader;} 
	inline void toggleVegetation(bool state){ _veg->toggleRendering(state); }
	inline void setRenderingOptions(bool drawInReflection, const vec3<F32>& eyePos = vec3<F32>(0,0,0)){_drawInReflection = drawInReflection; _eyePos = eyePos;}
	bool computeBoundingBox(SceneGraphNode* const sgn);
	inline bool isInView(bool distanceCheck,BoundingBox& boundingBox,const BoundingSphere& sphere) {return true;}

	void addTexture(TerrainTextureUsage channel, Texture2D* const texture);
	inline Texture2D* getTexture(TerrainTextureUsage channel) {return _terrainTextures[channel];}

protected:
	template<typename T>
	friend class ImplResourceLoader;
	void loadVisualResources();
	bool loadThreadedResources(TerrainDescriptor* const terrain);

private:

	U8                      _lightCount; 
	U16						_terrainWidth, _terrainHeight;
	Quadtree*				_terrainQuadtree;
	VertexBufferObject*		_groundVBO;

	F32  _terrainScaleFactor;
	F32  _terrainHeightScaleFactor;
	F32	 _farPlane;
	F32  _stateRefreshInterval;
	F32  _stateRefreshIntervalBuffer;
	bool _drawInReflection;
	bool _alphaTexturePresent;
	bool _drawBBoxes;
	bool _shadowMapped;

	TerrainTextureMap       _terrainTextures;
	Vegetation*             _veg;
	std::string             _grassShader;
	BoundingBox             _boundingBox;
	vec3<F32>               _eyePos;
	Quad3D*					_plane;
	Transform*				_planeTransform;
	SceneGraphNode*         _node;
	SceneGraphNode*			_planeSGN;
	RenderStateBlock*       _terrainRenderState;
};

#endif
