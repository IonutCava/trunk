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

#ifndef _MESH_H_
#define _MESH_H_

/*
DIVIDE-Engine: 21.10.2010 (Ionut Cava)

Mesh class. This class wraps all of the renderable geometry drawn by the engine. 
The only exceptions are: Terrain (including TerrainChunk) and Vegetation.

Meshes are composed of at least 1 submesh that contains vertex data, texture info and so on.
A mesh has a name, position, rotation, scale and a boolean value that enables or disables rendering 
across the network and one that disables rendering alltogheter;

Note: all transformations applied to the mesh affect every submesh that compose the mesh. 
*/

#include "resource.h"
#include "SubMesh.h"

class Mesh : public Object3D
{
	typedef unordered_map<std::string, SceneGraphNode*> childrenNodes;
public:
	Mesh() : Object3D(MESH), _visibleToNetwork(true), _loaded(false) { _refreshVBO = false;}
	inline void addSubMesh(const std::string& subMesh){_subMeshes.push_back(subMesh);}

	bool computeBoundingBox(SceneGraphNode* const node);
	inline std::vector<std::string>&   getSubMeshes()   {return _subMeshes;}

	bool load(const std::string& file);
	void postLoad(SceneGraphNode* const node);
	void render(SceneGraphNode* const node){};
	void createCopy();
	void removeCopy();

protected:
	void computeNormals(){}
	void computeTangents(){}	
	bool						 _visibleToNetwork, _loaded;
	std::vector<std::string >	 _subMeshes;
};

#endif