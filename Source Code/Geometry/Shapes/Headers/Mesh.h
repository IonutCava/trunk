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

#ifndef _MESH_H_
#define _MESH_H_

/**
DIVIDE-Engine: 21.10.2010 (Ionut Cava)

Mesh class. This class wraps all of the renderable geometry drawn by the engine. 
The only exceptions are: Terrain (including TerrainChunk) and Vegetation.

Meshes are composed of at least 1 submesh that contains vertex data, texture info and so on.
A mesh has a name, position, rotation, scale and a boolean value that enables or disables rendering 
across the network and one that disables rendering alltogheter;

Note: all transformations applied to the mesh affect every submesh that compose the mesh. 
*/

#include "core.h"
#include "Object3D.h"
#include <assimp/anim.h>

class Mesh : public Object3D {
public:

	Mesh(PrimitiveFlag flag = PRIMITIVE_FLAG_NONE) : Object3D(MESH,flag),
													 _visibleToNetwork(true)
	{
		_refreshVBO = false;
	}

	virtual ~Mesh() {}
		
	bool computeBoundingBox(SceneGraphNode* const sgn);
	virtual void updateTransform(SceneGraphNode* const sgn);
	virtual void updateBBatCurrentFrame(SceneGraphNode* const sgn);

	/// Called from SceneGraph "sceneUpdate"
	virtual void sceneUpdate(U32 sceneTime);
	virtual void postLoad(SceneGraphNode* const sgn);
	virtual	void onDraw();
	inline  void render(SceneGraphNode* const sgn){};
	virtual void preFrameDrawEnd() {}
	virtual void setSpecialShaderConstants(ShaderProgram* const shader) {}

	inline vectorImpl<std::string>&   getSubMeshes()   {return _subMeshes;}

	inline void  addSubMesh(const std::string& subMesh){_subMeshes.push_back(subMesh);}

protected:
	void computeTangents(){}	

protected:
	typedef Unordered_map<std::string, SceneGraphNode*> childrenNodes;
	typedef Unordered_map<U32, SubMesh*> subMeshRefMap;

	bool _visibleToNetwork;

	vectorImpl<std::string > _subMeshes;
	subMeshRefMap            _subMeshRefMap;
};

#endif