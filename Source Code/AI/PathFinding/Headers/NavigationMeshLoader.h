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

#include "Utility/Headers/BoundingBox.h"
#include "Graphs/Headers/SceneNode.h"

namespace Navigation {
	/// These are just sample areas to use consistent values across the samples.
	/// The use should specify these base on his needs.
	enum SamplePolyAreas
	{
		SAMPLE_POLYAREA_GROUND,
		SAMPLE_POLYAREA_WATER,
		SAMPLE_POLYAREA_ROAD,
		SAMPLE_POLYAREA_DOOR,
		SAMPLE_POLYAREA_GRASS,
		SAMPLE_POLYAREA_JUMP,
	};

	enum SamplePolyFlags
	{
		SAMPLE_POLYFLAGS_WALK		= 0x01,		// Ability to walk (ground, grass, road)
		SAMPLE_POLYFLAGS_SWIM		= 0x02,		// Ability to swim (water).
		SAMPLE_POLYFLAGS_DOOR		= 0x04,		// Ability to move through doors.
		SAMPLE_POLYFLAGS_JUMP		= 0x08,		// Ability to jump.
		SAMPLE_POLYFLAGS_DISABLED	= 0x10,		// Disabled polygon
		SAMPLE_POLYFLAGS_ALL		= 0xffff	// All abilities.
	};

	///Converting data to ReCast friendly format 

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
		}

		void clear(bool del = true)  {
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
		}

		F32* verts;
		U32  vert_cap;
		U32  vert_ct;
		F32* normals;
		U32  tri_ct;
		U32  tri_cap;
		I32* tris;

		inline const F32* getVerts() const { return verts; }
		inline const F32* getNormals() const { return normals; }
		inline const I32* getTris() const { return tris; }
		inline U32 getVertCount() const { return vert_ct; }
		inline U32 getTriCount() const { return tri_ct; }
	};

	enum MESH_DETAIL_LEVEL{
		DETAIL_ABSOLUTE,
		DETAIL_HIGH,
		DETAIL_MEDIUM,
		DETAIL_LOW,
		DETAIL_BOUNDINGBOX
	};

	class NavigationMeshLoader {

	public:
		NavigationMeshLoader();
		~NavigationMeshLoader();

		static NavModelData loadMeshFile(const char* fileName);
		static NavModelData mergeModels(NavModelData a, NavModelData b, bool delOriginals = false);
		static bool saveModelData(NavModelData data, const char* filename, const char* activeScene = NULL);
		static NavModelData parseNode(SceneGraphNode* sgn = NULL);

	private:
		static NavModelData parse(const BoundingBox& box, NavModelData& data, SceneGraphNode* set = NULL);
		static void addVertex(NavModelData* modelData, F32 x, F32 y, F32 z);
		static void addTriangle(NavModelData* modelData, I32 a, I32 b, I32 c);
		static char* parseRow(char* buf, char* bufEnd, char* row, I32 len);
		static I32 parseFace(char* row, I32* data, I32 n, I32 vcnt);
	};

};
#endif