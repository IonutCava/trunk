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

#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include "Utility/Headers/BoundingBox.h"
#include "TextureManager/ImageTools.h"
#include "Hardware/Video/FrameBufferObject.h"
#include "Managers/ResourceManager.h"
#include "Vegetation/Vegetation.h"
#include "EngineGraphs/SceneNode.h"

class Shader;
class Quadtree;
class VertexBufferObject;
class Quadtree;
class TerrainDescriptor;
class Terrain : public SceneNode
{
public:

	Terrain();
	~Terrain() {}

	bool load(const std::string& name);
	bool unload();
	
	void drawGround(bool drawInReflection) const;
	void render();
	void terrainSetParameters(const vec3& pos,const vec2& scale);

	vec3  getPosition(F32 x_clampf, F32 z_clampf) const;
	vec3  getNormal(F32 x_clampf, F32 z_clampf) const;
	vec3  getTangent(F32 x_clampf, F32 z_clampf) const;
	vec2  getDimensions(){return vec2((F32)_terrainWidth, (F32)_terrainHeight);}
	//The setActive method bypases ONLY RENDERING, not SceneGraphNode processing!!!!!!
	void  setLoaded(bool state) {_loaded = state;}
	void  setActive(bool state) {if(!_loaded) return; _wasActive = _active; _active = state;}
	void  restoreActive() {if(!_loaded) return; _active = _wasActive;}
	
	Vegetation* getVegetation() const {return _veg;}

	Quadtree& getQuadtree() {return *_terrainQuadtree;}

	void  terrainSmooth(F32 k);
	FrameBufferObject*	_depthMapFBO[2];

	void addVegetation(Vegetation* veg, std::string grassShader){_veg = veg; _grassShader = grassShader;} 
	void initializeVegetation(TerrainDescriptor* terrain);
	void toggleVegetation(bool state){ _veg->toggleRendering(state); }
	void setRenderingParams(bool drawInReflection,vec4& ambientcolor,mat4& sunModelviewProj){_drawInReflection = drawInReflection; _terrainMaterial->setAmbient(ambientcolor/1.5);_sunModelviewProj = sunModelviewProj;}
	void setDepthMap(U8 index, FrameBufferObject* depthMap){_depthMapFBO[index] = depthMap;}

	bool isActive() {return _active;}
	bool computeBoundingBox();
private:
	

	U16						_terrainWidth, _terrainHeight;
	Quadtree*				_terrainQuadtree;
	VertexBufferObject*		_groundVBO;
	
	F32 terrainScaleFactor, _terrainHeightScaleFactor;
	bool	_drawInReflection,_loaded, _active, _wasActive,_alphaTexturePresent;

	Shader*					_terrainShader;
	std::vector<Texture2D*>	_terrainTextures;
	Texture2D*				_terrainDiffuseMap;
	Vegetation*             _veg;
	std::string             _grassShader;
	mat4					_sunModelviewProj;
	Material*               _terrainMaterial;
	
};

#endif
