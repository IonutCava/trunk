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

#include "core.h"
#include "Utility/Headers/Event.h"
#include "NavigationMeshLoader.h" 

#include <ReCast/Headers/Recast.h>
#include <Detour/Headers/DetourNavMesh.h>
#include <Detour/Headers/DetourNavMeshQuery.h>
#include <Detour/Headers/DetourNavMeshBuilder.h>

class SceneGraphNode;

namespace Navigation {

	static const int NAVMESHSET_MAGIC = 'M'<<24 | 'S'<<16 | 'E'<<8 | 'T'; //'MSET';
	static const int NAVMESHSET_VERSION = 1;

	struct NavMeshSetHeader {
		int magic;
		int version;
		int numTiles;
		dtNavMeshParams params;
	};

	struct NavMeshTileHeader {
		dtTileRef tileRef;
		int dataSize;
	};

	/// @class NavigationMesh
	/// Represents a set of bounds within which a Recast navigation mesh is generated.

	class NavigationMesh/* : public SceneObject */{
	      friend class NavigationPath;

	public:
		/// @name NavigationMesh build
		/// @{

		/// Do we build in a separate thread?
		bool _buildThreaded;

		/// Initiates the NavigationMesh build process, which includes notifying the
		/// clients and posting an event.
		bool build(SceneGraphNode* const sgn);
		/// Save the NavigationMesh to a file.
		bool save();

		/// Load a saved NavigationMesh from a file.
		bool load();

		/// Data file to store this nav mesh in. (From engine executable dir.)
		std::string _fileName;

		/// Cell width and height.
		F32 _cellSize, _cellHeight;
		/// @name Actor data
		/// @{
		F32 _walkableHeight,
			_walkableClimb,
			_walkableRadius,
			_walkableSlope;
		/// @}
		/// @name Generation data
		/// @{
		U32 _borderSize;
		F32 _detailSampleDist, _detailSampleMaxError;
		U32 _maxEdgeLen;
		F32 _maxSimplificationError;
		static const U32 _maxVertsPerPoly;
		U32 _minRegionArea;
		U32 _mergeRegionArea;
		U32 _tileSize;
		/// @}

		/// Save imtermediate NavigationMesh creation data?
		bool _saveIntermediates;

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
		bool createPolyMesh(rcConfig &cfg, NavModelData &data, rcContext *ctx);
		/// Performs the Detour part of the build process.
		bool createNavigationMesh(dtNavMeshCreateParams &params);

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
		std::tr1::shared_ptr<Event> mThread;
		/// A mutex for NavigationMesh builds.
		boost::mutex _buildLock;
		/// A mutex for accessing our actual NavigationMesh.
		boost::mutex _navigationMeshLock;
		/// A simple flag to say we are building.
		bool _building;
		/// Thread launch function.
		static void launchThreadedBuild(void *data);
		///SceneGraphNode from which to build
		SceneGraphNode* _sgn;
	};
};

#endif