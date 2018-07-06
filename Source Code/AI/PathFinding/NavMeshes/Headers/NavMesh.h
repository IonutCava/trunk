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
		bool build(SceneGraphNode* const sgn,bool threaded = true);
		/// Save the NavigationMesh to a file.
		bool save();

		/// Load a saved NavigationMesh from a file.
		bool load(SceneGraphNode* const node);
        void render();
        inline void debugDraw(bool state) {_debugDraw = state;}
        inline bool debugDraw() {return _debugDraw;}
        inline void setRenderMode(RenderMode mode) {_renderMode = mode;}
        inline void setRenderConnections(bool state) {_renderConnections = state;}

		NavigationMesh();
		~NavigationMesh();

	protected:

		dtNavMesh const* getNavigationMesh() { return nm; }

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
		bool loadConfigFromFile(const std::string& configPath);

	private:
		bool _saveIntermediates;
		NavigationMeshConfig _configParams;
		/// @name NavigationMesh build
		/// @{
		/// Do we build in a separate thread?
		bool _buildThreaded;
		/// @name NavigationMesh build
		/// @{
		/// Do we have a valid configuration file for the current scene?
		bool _configFileValid;
		/// @name Intermediate data
		/// @{
		rcHeightfield        *hf;
		rcCompactHeightfield *chf;
		rcContourSet         *cs;
		rcPolyMesh           *pm;
		rcPolyMeshDetail     *pmd;
		dtNavMesh            *nm;
		dtNavMesh            *tnm;
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
		///SceneGraphNode from which to build
		SceneGraphNode* _sgn;
        boost::atomic<bool> _debugDraw;
        boost::atomic<bool> _renderConnections;
        RenderMode _renderMode;
		///DebugDraw interface
		NavMeshDebugDraw *_debugDrawInterface;
	};
};

#endif