/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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

namespace AI {

namespace Navigation {

    static const I32 NAVMESHSET_MAGIC = 'M'<<24 | 'S'<<16 | 'E'<<8 | 'T'; //'MSET';
    static const I32 NAVMESHSET_VERSION = 1;

    struct NavMeshSetHeader {
        I32 magic;
        I32 version;
        I32 numTiles;
        F32 extents[3];
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
          friend class DivideRecast;
    protected:

         enum RenderMode {
            RENDER_NAVMESH,
            RENDER_CONTOURS,
            RENDER_POLYMESH,
            RENDER_DETAILMESH,
            RENDER_PORTALS,
        };

    public:
		typedef boost::function1<void, NavigationMesh* > CreationCallback;

        inline void setFileName(const std::string& fileName) {_fileName.append(fileName);}
        /// Initiates the NavigationMesh build process, which includes notifying the
        /// clients and posting a task.
        bool build(SceneGraphNode* const sgn, CreationCallback creationCompleteCallback, bool threaded = true);
        /// Save the NavigationMesh to a file.
        bool save();
        /// Load a saved NavigationMesh from a file.
        bool load(SceneGraphNode* const node);
        /// Unload the navmesh reverting the instance to an empty container
        bool unload();
        /// Called once per frame with the time diff between this and the last frame
        void update(const U64 deltaTime);
        /// Render the debug mesh if debug drawing is enabled
        void render();
        inline void debugDraw(bool state)       {_debugDraw = state;}
        inline bool debugDraw()           const {return _debugDraw;}

        inline void setRenderMode(const RenderMode& mode) {_renderMode = mode;}
        inline void setRenderConnections(bool state)      {_renderConnections = state;}

        inline const vec3<F32>& getExtents() const {return _extents;}

        inline const dtNavMeshQuery& getNavQuery() const {return *_navQuery;}

        vec3<F32> getRandomPosition() const;

        NavigationMesh();
        ~NavigationMesh();

    protected:
        friend class DivideDtCrowd;
        dtNavMesh*            getNavigationMesh() const { return _navMesh; }
        const NavigationMeshConfig& getConfigParams()   const { return _configParams; }

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
        /// Create a navigation mesh query to help in pathfinding.
        bool createNavigationQuery(U32 maxNodes = 2048);

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
        std::shared_ptr<Task> _buildThread;
        /// A mutex for NavigationMesh builds.
        boost::mutex _buildLock;
        /// A mutex for accessing our actual NavigationMesh.
        boost::mutex _navigationMeshLock;
        /// A simple flag to say we are building.
        boost::atomic<bool> _building;
		/// A callback function to call after building is complete
		CreationCallback   _loadCompleteClbk;
        /// Data file to store this nav mesh in. (From engine executable dir.)
        std::string _fileName;
        /// Configuration file
        std::string _configFile;
        /// NavMesh extents
        vec3<F32> _extents;
        ///  Query object used for this mesh
        dtNavMeshQuery* _navQuery;
        ///SceneGraphNode from which to build
        SceneGraphNode* _sgn;
        boost::atomic<bool> _debugDraw;
        bool _renderConnections;
        RenderMode _renderMode;
        ///DebugDraw interface
        NavMeshDebugDraw *_debugDrawInterface;
	};
}; // namespace Navigation

}; // namespace AI

#endif