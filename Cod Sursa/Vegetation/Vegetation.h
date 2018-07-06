#ifndef VEGETATION_H_
#define VEGETATION_H_

#include "TextureManager/ImageTools.h"

class ImageTools::ImageData;
class VertexBufferObject;
class Terrain;
class Shader;
class Texture;
typedef Texture Texture2D;
class Vegetation
{
public:
	Vegetation(Terrain& terrain,U16 billboardCount, D32 grassDensity, F32 grassScale, D32 treeDensity, F32 treeScale, ImageTools::ImageData& map, std::vector<Texture2D*>& grassBillboards): 
	  _terrain(terrain),
	  _billboardCount(billboardCount),
	  _grassDensity(grassDensity),
	  _grassScale(grassScale),
	  _treeScale(treeScale),
	  _treeDensity(treeDensity),
	  _map(map),  
      _grassBillboards(grassBillboards),
	  _render(false),
	  _success(false){}

	void initialize(const std::string& grassShader);
	void toggleRendering(bool state){_render = state;}
	void draw(bool drawInReflexion);

private:
	//variables
	bool _render, _success ;                      //Toggle vegetation rendering On/Off
	Terrain& _terrain;
	D32 _grassDensity, _treeDensity;
	U16 _billboardCount;          //Vegetation cumulated density
	F32 _grassSize,_grassScale, _treeScale;
	F32 _windX, _windZ, _windS, _time;
	ImageTools::ImageData _map;  //Dispersion map for vegetation placement
	std::vector<Texture2D*>	_grassBillboards;
	Shader *_grassShader;

	bool generateTrees();			   //True = Everything OK, False = Error. Check _errorCode
	bool generateGrass(U32 index);     //index = current grass type (billboard, vbo etc)
	inline std::vector<U32>&		getGrassIndiceArray()		{return _grassIndice;}

	std::vector<VertexBufferObject*>	_grassVBO;
	std::vector<U32>					_grassIndice;
	
		
	void DrawGrass(U8 index,bool drawInReflexion);
	void DrawTrees(bool drawInReflexion);
};

#endif