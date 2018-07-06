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
	struct NavModelData {
		NavModelData() {
			verts = 0;
			vert_ct = 0;
			vert_cap = 0;
			normals = 0;
			tri_ct= 0;
			tri_cap = 0;
			tris = 0;
			_minValues.set(1,1,1);
			_maxValues.set(-1,-1,-1);
            valid = false;
		}

		void clear(bool del = true)  {
            valid = false;
			vert_ct = 0;
			tri_ct = 0;
			if(del)	SAFE_DELETE_ARRAY(verts)
			else verts = 0;
			vert_cap = 0;
			vert_ct = 0;
			if(del)	SAFE_DELETE_ARRAY(tris)
			else tris = 0;
			tri_cap = 0;
			tri_ct = 0;
			if(del)	SAFE_DELETE_ARRAY(normals)
			else normals = 0;
            triangleAreaType.clear();
            navMeshName = "";
		}

		F32* verts;
		U32  vert_cap;
		U32  vert_ct;
		F32* normals;
		U32  tri_ct;
		U32  tri_cap;
		I32* tris;
		///The minimum values on each axis
		vec3<F32> _minValues;
		///The maximum values on each axis
		vec3<F32> _maxValues;
        vectorImpl<SamplePolyAreas > triangleAreaType;
        bool valid;
        std::string navMeshName;

        inline void  setName(const std::string& name) {navMeshName = name;}
        inline const std::string& getName() {return navMeshName;}
		inline const F32* getVerts() const { return verts; }
		inline const F32* getNormals() const { return normals; }
		inline const I32* getTris() const { return tris; }
		inline U32 getVertCount() const { return vert_ct; }
		inline U32 getTriCount() const { return tri_ct; }
        inline vectorImpl<SamplePolyAreas >& getAreaTypes() {return triangleAreaType;}
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
		NavModelData mergeModels(NavModelData a, NavModelData b, bool delOriginals = false);
		bool saveMeshFile(NavModelData data, const char* filename, const char* activeScene = NULL);
        NavModelData parseNode(SceneGraphNode* sgn = NULL, const std::string& navMeshName = "");

		NavModelData parse(const BoundingBox& box, NavModelData& data, SceneGraphNode* set = NULL);
		void addVertex(NavModelData* modelData, const vec3<F32>& vertex);
		void addTriangle(NavModelData* modelData,const vec3<U32>& triangleIndices, const SamplePolyAreas& areaType = SAMPLE_POLYAREA_GROUND);
		char* parseRow(char* buf, char* bufEnd, char* row, I32 len);
		I32 parseFace(char* row, I32* data, I32 n, I32 vcnt);
	};
};
#endif