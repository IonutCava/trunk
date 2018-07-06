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

//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Changes, Additions and Refactoring : Copyright (c) 2010-2011 Lethal Concept, LLC
// Changes, Additions and Refactoring Author: Simon Wittenberg (MD)
// The above license is fully inherited.
//

#ifndef _NAVIGATION_MESH_LOADER_H_
#define _NAVIGATION_MESH_LOADER_H_

#include "NavMeshConfig.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Graphs/Headers/SceneNode.h"
#include <functional>

namespace Navigation {

	// This struct contains the vertices and triangles in recast coords
	class NavModelData {

	public:
		NavModelData() : _vertices(0),
						 _vertexCount(0),
						 _vertexCapacity(0),
						 _normals(0),
						 _triangleCount(0),
						 _triangleCapacity(0),
						 _triangles(0),
						 _valid(false)
		{
		}

		void clear(bool del = true)  {
            _valid = false;

			_vertexCount = _triangleCount = 0;
			_vertexCapacity = _vertexCount = 0;
			_triangleCapacity = _triangleCount = 0;

			if(del)	SAFE_DELETE_ARRAY(_vertices)
			else	_vertices = 0;
    		
			if(del)	SAFE_DELETE_ARRAY(_triangles)
			else    _triangles = 0;
			
			if(del)	SAFE_DELETE_ARRAY(_normals)
			else _normals = 0;

            _triangleAreaType.clear();
            _navMeshName = "";
		}

		inline bool  isValid()           const {return _valid;}
		inline void  isValid(bool state)       {_valid = state;}

        inline void  setName(const std::string& name)      {_navMeshName = name;}
        inline const std::string& getName()          const {return _navMeshName;}

		inline const F32* getVerts()     const { return _vertices; }
		inline const F32* getNormals()   const { return _normals; }
		inline const I32* getTris()      const { return _triangles; }
		inline       U32  getVertCount() const { return _vertexCount; }
		inline       U32  getTriCount()  const { return _triangleCount; }

        inline vectorImpl<SamplePolyAreas >& getAreaTypes() {return _triangleAreaType;}
	
		F32* _vertices;
		F32* _normals;
		I32* _triangles;

		U32  _vertexCapacity;
		U32  _vertexCount;
		U32  _triangleCount;
		U32  _triangleCapacity;

	private:
        bool _valid;
		std::string _navMeshName;
		vectorImpl<SamplePolyAreas > _triangleAreaType;
	};


	namespace NavigationMeshLoader {

		enum MeshDetailLevel {
			DETAIL_ABSOLUTE    = 0,
			DETAIL_HIGH        = 1,
			DETAIL_MEDIUM      = 2,
			DETAIL_LOW         = 3,
			DETAIL_BOUNDINGBOX = 4
		};

		NavModelData loadMeshFile(const char* fileName);
		bool         saveMeshFile(const NavModelData& data, const char* filename, const std::string& activeSceneName = "");

		NavModelData mergeModels(NavModelData& a,NavModelData& b, bool delOriginals = false);

		        NavModelData parseNode(SceneGraphNode* sgn = NULL, const std::string& navMeshName = "");

		NavModelData parse(const BoundingBox& box, NavModelData& data, SceneGraphNode* set = NULL);
		void addVertex(NavModelData* modelData, const vec3<F32>& vertex);
		void addTriangle(NavModelData* modelData,const vec3<U32>& triangleIndices, const SamplePolyAreas& areaType = SAMPLE_POLYAREA_GROUND);
		char* parseRow(char* buf, char* bufEnd, char* row, I32 len);
		I32 parseFace(char* row, I32* data, I32 n, I32 vcnt);
	};
};
#endif