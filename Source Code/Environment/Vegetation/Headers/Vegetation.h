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

#ifndef VEGETATION_H_
#define VEGETATION_H_

#include "Utility/Headers/ImageTools.h"

class Terrain;
class Texture;
class Transform;
class ShaderProgram;
class SceneGraphNode;
class RenderStateBlock;
class FrameBufferObject;
class VertexBufferObject;
typedef Texture Texture2D;
enum RenderStage;
///Generates grass and trees on the terrain.
///Grass VBO's + all resources are stored locally in the class.
///Trees are added to the SceneGraph and handled by the scene.
class Vegetation{
public:
	Vegetation(U16 billboardCount, D32 grassDensity, F32 grassScale, D32 treeDensity, F32 treeScale, const std::string& map, vectorImpl<Texture2D*>& grassBillboards):
      _grassVBO(NULL),
      _billboardCount(billboardCount),
	  _grassDensity(grassDensity),
	  _grassScale(grassScale),
	  _treeScale(treeScale),
	  _treeDensity(treeDensity),
      _grassBillboards(grassBillboards),
	  _render(false),
	  _success(false),
	  _shadowMapped(true),
	  _terrain(NULL),
	  _terrainSGN(NULL),
	  _grassShader(NULL),
	  _stateRefreshIntervalBuffer(0),
	  _stateRefreshInterval(1000) ///<Every second?
	  {
		  bool alpha = false;
		  ImageTools::OpenImage(map,_map,alpha);
	  }
	~Vegetation();
	void initialize(const std::string& grassShader, Terrain* const terrain,SceneGraphNode* const terrainSGN);
	inline void toggleRendering(bool state){_render = state;}
	///parentTransform: the transform of the parent terrain node
	void draw(const RenderStage& currentStage, Transform* const parentTransform);
	void sceneUpdate(const U32 sceneTime,SceneGraphNode* const sgn);

private:
	bool generateTrees();			   ///< True = Everything OK, False = Error. Check _errorCode
	bool generateGrass(U32 index, U32 size);     ///< index = current grass type (billboard, vbo etc)
                                                 ///< size = the available vertex count

private:
	//variables
	bool _render; ///< Toggle vegetation rendering On/Off
	bool _success ;
	SceneGraphNode* _terrainSGN;
	Terrain*        _terrain;
	D32 _grassDensity, _treeDensity;
	U16 _billboardCount;          ///< Vegetation cumulated density
	F32 _grassSize,_grassScale, _treeScale;
	F32 _windX, _windZ, _windS, _time;
	F32 _stateRefreshInterval;
	F32 _stateRefreshIntervalBuffer;
	ImageTools::ImageData _map;  ///< Dispersion map for vegetation placement
	vectorImpl<Texture2D*>	_grassBillboards;
	ShaderProgram*		    _grassShader;

	bool _shadowMapped;
    vectorImpl<U32>     _grassVBOBillboardIndice;
	VertexBufferObject*	_grassVBO;
	RenderStateBlock*   _grassStateBlock;
};

#endif