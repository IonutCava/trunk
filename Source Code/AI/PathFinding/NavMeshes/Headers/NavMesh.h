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
//
// T3D 1.1 NavMesh class and supporting methods.
// Daniel Buckmaster, 2011
// With huge thanks to:
//    Lethal Concept http://www.lethalconcept.com/
//    Mikko Mononen http://code.google.com/p/recastnavigation/
//

#ifndef _NAVIGATION_MESH_H_
#define _NAVIGATION_MESH_H_

#include "Utility/Headers/GUIDWrapper.h"
#include "Hardware/Platform/Headers/Task.h"

#include "NavMeshConfig.h"
#include "NavMeshLoader.h"
#include "NavMeshContext.h"

class SceneGraphNode;

namespace Navigation {

	static const I32 NAVMESHSET_MAGIC = 'M'<<24 | 'S'<<16 | 'E'<<8 | 'T'; //'MSET';
	static const I32 NAVMESHSET_VERSION = 1;

	struct NavMeshSetHeader {
		I32 magic;
		I32 version;
		I32 numTiles;
		dtNavMeshParams params;
	};

	struct NavMeshTileHeader {
		dtTileRef tileRef;
		I32 dataSize;
	};

    /// @class NavigationMesh
	/// Represents a set of bounds within which a Recast navigation mesh is generated.
	class NavMeshDebugDraw;

	class NavigationMesh : public GUIDWrapper/*,public SceneObject */{
	      friend class NavigationPath;
    protected:

         enum RenderMode {
            RENDER_NAVMESH,
            RENDER_CONTOURS,
            RENDER_POLYMESH,
            RENDER_DETAILMESH,
            RENDER_PORTALS,
        };

	public:
        inline void setFileName(const std::string& fileName) {_fileName.append(fileName);}
		/// Initiates the NavigationMesh build process, which includes notifying the
		/// clients and posting a task.
		bool build(SceneGraphNode* const sgn, bool threaded = true);
		/// Save the NavigationMesh to a file.
		bool save();
		/// Load a saved NavigationMesh from a file.
		bool load(SceneGraphNode* const node);

        void render();
        inline void debugDraw(bool state) {_debugDraw = state;}
        inline bool debugDraw() {return _debugDraw;}

        inline void setRenderMode(const RenderMode& mode) {_renderMode = mode;}
        inline void setRenderConnections(bool state)      {_renderConnections = state;}
		
		NavigationMesh();
		~NavigationMesh();

	protected:

		dtNavMesh const* getNavigationMesh() { return _navMesh; }

	private:

		/// Initiates the build process in a separate thread.
		bool buildThreaded();
		/// Runs the build process. Not threadsafe,. so take care to synchronise
		/// calls to this method properly!
		bool buildProcess();
		/// Generates a navigation mesh for the collection of objects in this
		/// mesh. Returns true if successful. Stores the created mesh in tnm.
		bool generateMesh();
		/// Performs the Recast part of the build process.
		bool createPolyMesh(rcConfig &cfg, NavModelData &data, rcContextDivide *ctx);
		/// Performs the Detour part of the build process.
		bool createNavigationMesh(dtNavMeshCreateParams &params);
		/// Load nav mesh configuration from file
		bool loadConfigFromFile();

	private:
		bool _saveIntermediates;
		NavigationMeshConfig _configParams;
		/// @name NavigationMesh build
		/// @{
		/// Do we build in a separate thread?
		bool _buildThreaded;
		/// @name Intermediate data
		/// @{
		rcHeightfield        *_heightField;
		rcCompactHeightfield *_compactHeightField;
		rcContourSet         *_countourSet;
		rcPolyMesh           *_polyMesh;
		rcPolyMeshDetail     *_polyMeshDetail;
		dtNavMesh            *_navMesh;
		dtNavMesh            *_tempNavMesh;
		/// Free all stored working data.
		/// @param freeAll Force all data to be freed, retain none.
		void freeIntermediates(bool freeAll);
		/// @}

		/// A thread for us to update in.
		std::tr1::shared_ptr<Task> _buildThread;
		/// A mutex for NavigationMesh builds.
		boost::mutex _buildLock;
		/// A mutex for accessing our actual NavigationMesh.
		boost::mutex _navigationMeshLock;
		/// A simple flag to say we are building.
        boost::atomic<bool> _building;
		/// Thread launch function.
		static void launchThreadedBuild(void *data);
        /// Data file to store this nav mesh in. (From engine executable dir.)
		std::string _fileName;
		/// Configuration file
		std::string _configFile;
		///SceneGraphNode from which to build
		SceneGraphNode* _sgn;
        boost::atomic<bool> _debugDraw;
        bool _renderConnections;
        RenderMode _renderMode;
		///DebugDraw interface
		NavMeshDebugDraw *_debugDrawInterface;
	};
};

#endif