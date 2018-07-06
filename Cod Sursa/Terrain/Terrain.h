#ifndef TERRAIN_H_
#define TERRAIN_H_

#include "Utility/Headers/BoundingBox.h"
#include "TextureManager/ImageTools.h"
#include "Hardware/Video/FrameBufferObject.h"
#include "Managers/ResourceManager.h"
#include "Vegetation/Vegetation.h"

class Shader;
class Quadtree;
class VertexBufferObject;
class Quadtree;
class Terrain : public Resource
{
public:

	Terrain();
	Terrain(vec3 pos, vec2 scale);
	~Terrain() {unload();}

	bool load(const std::string& heightmap);
	bool unload(){Destroy(); if(!_groundVBO) return true; else return false;}
	bool computeBoundingBox();
	
	void Destroy();
	void drawGround(bool drawInReflexion) const;
	void draw() const;
	void terrainSetParameters(const vec3& pos,const vec2& scale);

	vec3  getPosition(F32 x_clampf, F32 z_clampf) const;
	vec3  getNormal(F32 x_clampf, F32 z_clampf) const;
	vec3  getTangent(F32 x_clampf, F32 z_clampf) const;
	vec2  getDimensions(){return vec2((F32)_terrainWidth, (F32)_terrainHeight);}
	bool  getLoaded() {return _loaded;}
	void  setLoaded(bool state) {_wasLoaded = _loaded; _loaded = state;}
	void  restoreLoaded() {_loaded = _wasLoaded;}
	Vegetation* getVegetation() const {return _veg;}

	Quadtree& getQuadtree() {return *_terrainQuadtree;}
	const BoundingBox&	getBoundingBox() const	{return _terrainBBox;}

	void  terrainSmooth(F32 k);
	FrameBufferObject*	_depthMapFBO[2];
	void setShader(Shader *s){terrainShader = s;}
	void addTexture(Texture2D *t){_terrainTextures.push_back(t);}
	void setDiffuse(Texture2D *t){_terrainDiffuseMap = t;}
	void addVegetation(Vegetation* veg, std::string grassShader){_veg = veg; _grassShader = grassShader;} 
	void initializeVegetation() { _veg->initialize(_grassShader);}
	void toggleVegetation(bool state){ _veg->toggleRendering(state); }
	void toggleRenderingParams(bool drawInReflexion,vec4& ambientcolor,mat4& sunModelviewProj){_drawInReflexion = drawInReflexion; _ambientColor = ambientcolor; _sunModelviewProj = sunModelviewProj;}
	bool postLoad();
	bool isPostLoaded() {return _postLoaded;}

private:
	

	BoundingBox				_terrainBBox;
	U16						_terrainWidth, _terrainHeight;
	Quadtree*				_terrainQuadtree;
	Texture2D*              _lightmap;
	VertexBufferObject*		_groundVBO;
	
	F32 terrainScaleFactor, _terrainHeightScaleFactor;
	bool	_drawInReflexion,_postLoaded,_loaded,_wasLoaded;

	Shader *terrainShader;
	std::vector<Texture2D*>	_terrainTextures;
	Texture2D*				_terrainDiffuseMap;
	Vegetation*             _veg;
	std::string             _grassShader;
	vec4                    _ambientColor;
	mat4					_sunModelviewProj;
	
};

#endif
