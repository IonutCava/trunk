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
	Vegetation(U16 billboardCount, D32 grassDensity, F32 grassScale, D32 treeDensity, F32 treeScale, const std::string& map, std::vector<Texture2D*>& grassBillboards): 
	  _billboardCount(billboardCount),
	  _grassDensity(grassDensity),
	  _grassScale(grassScale),
	  _treeScale(treeScale),
	  _treeDensity(treeDensity),
      _grassBillboards(grassBillboards),
	  _render(false),
	  _success(false)
	  {
		  ImageTools::OpenImage(map,_map);
	  }
	~Vegetation();
	void initialize(const std::string& grassShader, const std::string& terrainName);
	void toggleRendering(bool state){_render = state;}
	void draw(bool drawInReflection);

private:
	//variables
	bool _render, _success ;                      //Toggle vegetation rendering On/Off
	Terrain* _terrain;
	D32 _grassDensity, _treeDensity;
	U16 _billboardCount;          //Vegetation cumulated density
	F32 _grassSize,_grassScale, _treeScale;
	F32 _windX, _windZ, _windS, _time;
	ImageTools::ImageData _map;  //Dispersion map for vegetation placement
	std::vector<Texture2D*>	_grassBillboards;
	Shader*				    _grassShader;

	bool generateTrees();			   //True = Everything OK, False = Error. Check _errorCode
	bool generateGrass(U32 index);     //index = current grass type (billboard, vbo etc)
	inline std::vector<U32>&		getGrassIndiceArray()		{return _grassIndice;}

	std::vector<VertexBufferObject*>	_grassVBO;
	std::vector<U32>					_grassIndice;
	
		
	void DrawGrass(U8 index,bool drawInReflection);
	void DrawTrees(bool drawInReflection);
};

#endif