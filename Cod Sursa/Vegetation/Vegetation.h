#ifndef VEGETATION_H_
#define VEGETATION_H_

#include "Utility/Headers/ParamHandler.h"
#include "Managers/ResourceManager.h"

using namespace std;

class Terrain;
class VertexBufferObject;

class Vegetation
{
public:
	Vegetation(Terrain& terrain,U32 billboardCount, D32 grassDensity, F32 grassScale, D32 treeDensity, F32 treeScale, ImageTools::ImageData& map, vector<Texture2D*>& grassBillboards): 
	  _terrain(terrain),
	  _billboardCount(billboardCount),
	  _grassDensity(grassDensity),
	  _grassScale(grassScale),
	  _treeScale(treeScale),
	  _treeDensity(treeDensity),
	  _map(map),  
      _grassBillboards(grassBillboards),
	  _render(false),
	  _success(false),
	  res(ResourceManager::getInstance()),
	  par(ParamHandler::getInstance()){}

	void initialize(string grassShader, string treeShader);
	void toggleRendering(bool state){_render = state;}
	void draw(bool drawInReflexion);

private:
	//variables
	bool _render, _success ;                      //Toggle vegetation rendering On/Off
	Terrain& _terrain;
	D32 _grassDensity, _treeDensity, _billboardCount;          //Vegetation cumulated density
	F32 _grassScale, _treeScale;
	ParamHandler& par;
	ResourceManager& res;
	ImageTools::ImageData _map;  //Dispersion map for vegetation placement
	vector<Texture2D*>	_grassBillboards;
	Shader *_grassShader, *_treeShader;

	bool generateTrees();			   //True = Everything OK, False = Error. Check _errorCode
	bool generateGrass(int index);     //index = current grass type (billboard, vbo etc)
	inline vector<GLuint>&		getGrassIndiceArray()		{return _grassIndice;}

	vector<string>     _treeNames;
	
	vector<VertexBufferObject*>	_grassVBO;
	vector<GLuint>	_grassIndice;
	
		
	void DrawGrass(int index,bool drawInReflexion);
	void DrawTrees(bool drawInReflexion);
};

#endif