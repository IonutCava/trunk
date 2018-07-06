#ifndef TERRAIN_H_
#define TERRAIN_H_

#include "Utility/Headers/BoundingBox.h"
#include "Utility/Headers/DataTypes.h"
#include "TextureManager/ImageTools.h"
#include "Hardware/Video/FrameBufferObject.h"
#include "Managers/ResourceManager.h"
#include "Vegetation/Vegetation.h"

class Shader;
class Quadtree;
class VertexBufferObject;

class Terrain : public GraphicResource
{
public:

	Terrain();
	Terrain(vec3 pos, vec2 scale);
	~Terrain() {destroy();}

	bool load(const string& heightmap);
	bool unload(){destroy(); if(!m_pGroundVBO) return true; else return false;}
	bool computeBoundingBox();
	
	void destroy();
	int  drawGround(bool drawInReflexion) const;
	void draw() const;
	int  drawObjects() const;
	void terrainSetParameters(vec3& pos, vec2& scale);

	vec3  getPosition(F32 x_clampf, F32 z_clampf) const;
	vec3  getNormal(F32 x_clampf, F32 z_clampf) const;
	vec3  getTangent(F32 x_clampf, F32 z_clampf) const;
	vec2  getDimensions(){return vec2((F32)terrainWidth, (F32)terrainHeight);}
	Quadtree& getQuadtree() {return *terrain_Quadtree;}
	const BoundingBox&	getBoundingBox() const	{return terrain_BBox;}

	void  terrainSmooth(F32 k);
	FrameBufferObject*	m_fboDepthMapFromLight[2];
	void setShader(Shader *s){terrainShader = s;}
	void addTexture(Texture2D *t){m_tTextures.push_back(t);}
	void setDiffuse(Texture2D *t){m_pTerrainDiffuseMap = t;}
	void setLoaded(bool loaded){if(!_loaded) _wasLoaded = _loaded; _loaded = loaded;} 
	void restoreLoaded(){_loaded = _wasLoaded;}
	void addVegetation(Vegetation* veg, string grassShader){_veg = veg; _grassShader = grassShader;} 
	void initializeVegetation() { _veg->initialize(_grassShader);}
	void toggleVegetation(bool state){ _veg->toggleRendering(state); }
	void toggleRenderingParams(bool state,vec4& ambientcolor){_drawInReflexion = state; _ambientColor = ambientcolor; }
	bool postLoad();
	bool isPostLoaded() {return _postLoaded;}
private:
	

	BoundingBox				terrain_BBox;
	U32			terrainWidth, terrainHeight;
	Quadtree*				terrain_Quadtree;
	Texture2D*              _lightmap;
	VertexBufferObject*		m_pGroundVBO;
	
	F32 terrainScaleFactor, terrainHeightScaleFactor;
	bool	m_bShowDetail, _loaded,_wasLoaded,_drawInReflexion,_postLoaded;
	int detailId;


	Shader *terrainShader;
	vector<Texture2D*>	m_tTextures;
	Texture2D*				m_pTerrainDiffuseMap;
	Vegetation*             _veg;
	string                  _grassShader;
	vec4                    _ambientColor;
	
};

#endif
