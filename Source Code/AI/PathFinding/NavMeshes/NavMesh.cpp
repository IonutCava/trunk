#include "Headers/NavMesh.h"
#include "Headers/NavMeshDebugDraw.h"

#include <SimpleIni.h>
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"

#include <DebugUtils/Include/RecastDump.h>
#include <DebugUtils/Include/DetourDebugDraw.h>
#include <DebugUtils/Include/RecastDebugDraw.h>

namespace Navigation {

    NavigationMesh::NavigationMesh() : GUIDWrapper() {
        ParamHandler& par = ParamHandler::getInstance();
		std::string path = par.getParam<std::string>("scriptLocation") + "/" +
				     par.getParam<std::string>("scenesLocation") + "/" +
					 par.getParam<std::string>("currentScene");

        _fileName   =  path + "/navMeshes/";
		_configFile = path + "/navMeshConfig.ini";
		_buildThreaded = true;
        _debugDraw = true;
        _renderConnections = true;
        _renderMode = RENDER_NAVMESH;
		_debugDrawInterface = New NavMeshDebugDraw();
		assert(_debugDrawInterface != NULL);
		hf  = NULL;
		chf = NULL;
		cs  = NULL;
		pm  = NULL;
		pmd = NULL;
		nm  = NULL;
		tnm = NULL;
		_building = false;
	}

	NavigationMesh::~NavigationMesh(){
		if(_buildThread) _buildThread->stopTask();
		
		freeIntermediates(true);
		dtFreeNavMesh(nm);
		nm = NULL;
		dtFreeNavMesh(tnm);
		tnm = NULL;
	}

	void NavigationMesh::freeIntermediates(bool freeAll){
		_navigationMeshLock.lock();

		rcFreeHeightField(hf);          hf = NULL;
		rcFreeCompactHeightfield(chf); chf = NULL;

		if(!_saveIntermediates || freeAll)	{
		 rcFreeContourSet(cs);        cs = NULL;
		 rcFreePolyMesh(pm);          pm = NULL;
		 rcFreePolyMeshDetail(pmd);  pmd = NULL;
		}

		_navigationMeshLock.unlock();
	}

	bool NavigationMesh::loadConfigFromFile(){
		//Use SimpleIni library for cross-platform INI parsing
		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(_configFile.c_str());

        if(!ini.GetSection("Rasterization") ||
		   !ini.GetSection("Agent") ||
		   !ini.GetSection("Region") ||
		   !ini.GetSection("Polygonization") ||
		   !ini.GetSection("DetailMesh")) return false;
		
		//Load all key-value pairs for the "Rasterization" section
		_configParams.setCellSize(Util::convertData<F32,const char*>(ini.GetValue("Rasterization", "fCellSize", "0.3")));
		_configParams.setCellHeight(Util::convertData<F32, const char*>(ini.GetValue("Rasterization", "fCellHeight", "0.2")));
		_configParams.setTileSize(Util::convertData<I32,const char*>(ini.GetValue("Rasterization","iTileSize","48")));
		//Load all key-value pairs for the "Agent" section
		_configParams.setAgentHeight(Util::convertData<F32,const char*>(ini.GetValue("Agent", "fAgentHeight", "2.5")));
        _configParams.setAgentRadius(Util::convertData<F32,const char*>(ini.GetValue("Agent", "fAgentRadius", "0.5")));
		_configParams.setAgentMaxClimb(Util::convertData<F32,const char*>(ini.GetValue("Agent", "fAgentMaxClimb", "1")));
		_configParams.setAgentMaxSlope(Util::convertData<F32,const char*>(ini.GetValue("Agent", "fAgentMaxSlope", "20")));
		//Load all key-value pairs for the "Region" section
		_configParams.setRegionMergeSize(Util::convertData<F32,const char*>(ini.GetValue("Region", "fMergeSize", "20")));
		_configParams.setRegionMinSize(Util::convertData<F32,const char*>(ini.GetValue("Region", "fMinSize", "50")));
		//Load all key-value pairs for the "Polygonization" section
		_configParams.setEdgeMaxLen(Util::convertData<F32,const char*>(ini.GetValue("Polygonization", "fEdgeMaxLength", "12")));
		_configParams.setEdgeMaxError(Util::convertData<F32,const char*>(ini.GetValue("Polygonization", "fEdgeMaxError", "1.3")));
		_configParams.setVertsPerPoly(Util::convertData<I32,const char*>(ini.GetValue("Polygonization", "iVertsPerPoly", "6")));
		//Load all key-value pairs for the "DetailMesh" section
		_configParams.setDetailSampleDist(Util::convertData<F32,const char*>(ini.GetValue("DetailMesh", "fDetailSampleDist", "6")));
		_configParams.setDetailSampleMaxError(Util::convertData<F32,const char*>(ini.GetValue("DetailMesh", "fDetailSampleMaxError", "1")));
		_configParams.setKeepInterResults(Util::convertData<bool,const char*>(ini.GetValue("DetailMesh", "bKeepInterResults", "false")));

		return true;
	}

	bool NavigationMesh::build(SceneGraphNode* const sgn,bool threaded){
		if(!loadConfigFromFile()) return false;

		if(sgn){
			_sgn = sgn;
		}else{	/// use root node
			_sgn = GET_ACTIVE_SCENE()->getSceneGraph()->getRoot();
		}

		if(_buildThreaded && threaded) return buildThreaded();

		boost::mutex::scoped_lock(_buildLock);
        if(_buildThreaded && threaded){
		    if(!_buildLock.owns_lock()) return false;
        }else{
			return buildProcess();
		}
		return true;
	}

	bool NavigationMesh::buildThreaded(){
		boost::mutex::scoped_lock(_buildLock);
		if(!_buildLock.owns_lock()) return false;

		if(_buildThread) _buildThread->stopTask();
		
		Kernel* kernel = Application::getInstance().getKernel();
		_buildThread.reset(New Task(kernel->getThreadPool(),3,true,true,DELEGATE_BIND(&NavigationMesh::launchThreadedBuild,this)));

		return true;
	}

	void NavigationMesh::launchThreadedBuild(void *data){
		NavigationMesh *pThis = (NavigationMesh*)data;
		pThis->buildProcess();
	}

	bool NavigationMesh::buildProcess(){
		_building = true;
		// Create mesh
        U32 timeStart = GETMSTIME();
		bool success = generateMesh();
        U32 timeEnd = GETMSTIME();
        U32 timeDiff = timeEnd - timeStart;
        if(success){
            PRINT_FN(Locale::get("NAV_MESH_GENERATION_COMPLETE"),getMsToSec(timeDiff));
        }else{
            ERROR_FN(Locale::get("NAV_MESH_GENERATION_INCOMPLETE"),getMsToSec(timeDiff));
        }
		_navigationMeshLock.lock();
		// Copy new NavigationMesh into old.
		dtNavMesh *old = nm;
		nm = tnm; // I am trusting that this is atomic.
		dtFreeNavMesh(old);
		tnm = NULL;
		_navigationMeshLock.unlock();

		// Free structs used during build
		freeIntermediates(false);

		_building = false;

		return success;
	}

	bool NavigationMesh::generateMesh(){
		assert(_sgn != NULL);
        std::string nodeName((_sgn->getNode<SceneNode>()->getType() != TYPE_ROOT) ? "node_[_" + _sgn->getName() + "_]" : "root_node");
		// Parse objects from level into RC-compatible format
        _fileName.append(nodeName);
        PRINT_FN(Locale::get("NAV_MESH_GENERATION_START"),nodeName.c_str());
		NavModelData data = NavigationMeshLoader::parseNode(_sgn,nodeName);

		// Check for no geometry
		if(!data.getVertCount()) return false;
		// Free intermediate and final results
		freeIntermediates(true);
		// Recast initialisation data
		rcContextDivide ctx(true);
		ctx.enableTimer(true);

		rcConfig cfg;

		memset(&cfg, 0, sizeof(cfg));
		_saveIntermediates = _configParams.getKeepInterResults();
		cfg.cs = _configParams.getCellSize();
		cfg.ch = _configParams.getCellHeight();
		rcCalcBounds(data.verts, data.getVertCount(), cfg.bmin, cfg.bmax);
		rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
		PRINT_FN(Locale::get("NAV_MESH_BOUNDS"),cfg.bmax[0],cfg.bmax[1],cfg.bmax[2],cfg.bmin[0],cfg.bmin[1],cfg.bmin[2]);

		cfg.walkableHeight         = _configParams.base_getWalkableHeight();
		cfg.walkableClimb          = _configParams.base_getWalkableClimb();
		cfg.walkableRadius         = _configParams.base_getWalkableRadius();
		cfg.walkableSlopeAngle     = _configParams.getAgentMaxSlope();
		cfg.borderSize             = (I32)(_configParams.base_getWalkableRadius() + BORDER_PADDING); // Reserve enough padding
		cfg.detailSampleDist       = _configParams.getDetailSampleDist();
		cfg.detailSampleMaxError   = _configParams.getDetailSampleMaxError();
		cfg.maxEdgeLen             = _configParams.getEdgeMaxLen();
		cfg.maxSimplificationError = _configParams.getEdgeMaxError();
		cfg.maxVertsPerPoly        = _configParams.getVertsPerPoly();
		cfg.minRegionArea          = _configParams.getRegionMinSize();
		cfg.mergeRegionArea        = _configParams.getRegionMergeSize();
		cfg.tileSize               = _configParams.getTileSize();

        if(!createPolyMesh(cfg, data, &ctx)){
            data.valid = false;
		    return false;
        }
		//Detour initialisation data
		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));

		params.walkableHeight = cfg.walkableHeight;
		params.walkableRadius = cfg.walkableRadius;
		params.walkableClimb = cfg.walkableClimb;
		params.tileX = 0;
		params.tileY = 0;
		params.tileLayer = 0;
		rcVcopy(params.bmax, cfg.bmax);
		rcVcopy(params.bmin, cfg.bmin);
		params.buildBvTree = true;
		params.ch = cfg.ch;
		params.cs = cfg.cs;

		params.verts = pm->verts;
		params.vertCount = pm->nverts;
		params.polys = pm->polys;
		params.polyAreas = pm->areas;
		params.polyFlags = pm->flags;
		params.polyCount = pm->npolys;
		params.nvp = pm->nvp;

		params.detailMeshes = pmd->meshes;
		params.detailVerts = pmd->verts;
		params.detailVertsCount = pmd->nverts;
		params.detailTris = pmd->tris;
		params.detailTriCount = pmd->ntris;

        if(!createNavigationMesh(params)){
            data.valid = false;
		    return false;
        }
        data.valid = true;
		return true;
	}

	bool NavigationMesh::createPolyMesh(rcConfig &cfg, NavModelData &data, rcContextDivide *ctx){
        if(_fileName.empty()){
            _fileName = data.getName();
        }
		// Create a heightfield to voxelise our input geometry
		hf = rcAllocHeightfield();
		if(!hf){
			ERROR_FN(Locale::get("ERROR_NAV_OUT_OF_MEMORY"), "rcAllocHeightfield", _fileName.c_str());
			return false;
		}

        // Reset build times gathering.
	    ctx->resetTimers();
    	// Start the build process.
	    ctx->startTimer(RC_TIMER_TOTAL);
		ctx->log(RC_LOG_PROGRESS, "Building navigation:");
	    ctx->log(RC_LOG_PROGRESS, " - %d x %d cells", cfg.width, cfg.height);
        ctx->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris", data.getVertCount()/1000.0f, data.getTriCount()/1000.0f);

		if(!rcCreateHeightfield(ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)){
			ERROR_FN(Locale::get("ERROR_NAV_HEIGHTFIELD"), _fileName.c_str());
			return false;
		}

		U8* areas = New U8[data.getTriCount()];

		if (!areas){
			ERROR_FN(Locale::get("ERROR_NAV_OUT_OF_MEMORY"), "areaFlag allocation", _fileName.c_str());
			return false;
		}

		memset(areas, 0, data.getTriCount()*sizeof(U8));

		// Subtract 1 from all indices!
		/*for(U32 i = 0; i < data.getTriCount(); i++){
			data.tris[i*3]--;
			data.tris[i*3+1]--;
			data.tris[i*3+2]--;
		}*/

		// Filter triangles by angle and rasterize
		rcMarkWalkableTriangles(ctx, cfg.walkableSlopeAngle, data.getVerts(), data.getVertCount(),	data.getTris(), data.getTriCount(), areas);
		rcRasterizeTriangles(ctx, data.getVerts(), data.getVertCount(), data.getTris(), areas, data.getTriCount(), *hf, cfg.walkableClimb);

		if(!_saveIntermediates){
			SAFE_DELETE_ARRAY(areas);
		}

		// Filter out areas with low ceilings and other stuff
		rcFilterLowHangingWalkableObstacles(ctx, cfg.walkableClimb, *hf);
		rcFilterLedgeSpans(ctx, cfg.walkableHeight, cfg.walkableClimb, *hf);
		rcFilterWalkableLowHeightSpans(ctx, cfg.walkableHeight, *hf);

		chf = rcAllocCompactHeightfield();
		if(!chf || !rcBuildCompactHeightfield(ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf)){
			ERROR_FN(Locale::get("ERROR_NAV_COMPACT_HEIGHTFIELD"), _fileName.c_str());
			return false;
		}

		if(!rcErodeWalkableArea(ctx, cfg.walkableRadius, *chf))	{
			ERROR_FN(Locale::get("ERROR_NAV_WALKABLE"), _fileName.c_str());
			return false;
		}

		if(false){
			if(!rcBuildRegionsMonotone(ctx, *chf, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea)){
				ERROR_FN(Locale::get("ERROR_NAV_REGIONS"), _fileName.c_str());
				return false;
			}
		}else{
			if(!rcBuildDistanceField(ctx, *chf))
				return false;
			if(!rcBuildRegions(ctx, *chf, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
				return false;
		}

		cs = rcAllocContourSet();
		if(!cs || !rcBuildContours(ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cs)){
			ERROR_FN(Locale::get("ERROR_NAV_COUNTOUR"), _fileName.c_str());
			return false;
		}

		pm = rcAllocPolyMesh();
		if(!pm || !rcBuildPolyMesh(ctx, *cs, cfg.maxVertsPerPoly, *pm))	{
			ERROR_FN(Locale::get("ERROR_NAV_POLY_MESH"), _fileName.c_str());
			return false;
		}

		pmd = rcAllocPolyMeshDetail();
		if(!pmd || !rcBuildPolyMeshDetail(ctx, *pm, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *pmd)){
			ERROR_FN(Locale::get("ERROR_NAV_POLY_MESH_DETAIL"), _fileName.c_str());
			return false;
		}
        ctx->stopTimer(RC_TIMER_TOTAL);
    	// Show performance stats.
	    duLogBuildTimes(*ctx, ctx->getAccumulatedTime(RC_TIMER_TOTAL));
	    ctx->log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", pm->nverts, pm->npolys);
	    F32 totalBuildTimeMs = ctx->getAccumulatedTime(RC_TIMER_TOTAL)/1000.0f;

	    PRINT_FN("[RC_LOG_PROGRESS] Polymesh: %d vertices  %d polygons %5.2f ms\n", pm->nverts, pm->npolys,totalBuildTimeMs);
		return true;
	}

	bool NavigationMesh::createNavigationMesh(dtNavMeshCreateParams &params){
		U8 *tileData = NULL;
		I32 tileDataSize = 0;
		if(!dtCreateNavMeshData(&params, &tileData, &tileDataSize)){
			ERROR_FN(Locale::get("ERROR_NAV_MESH_DATA"), _fileName.c_str());
			return false;
		}

		tnm = dtAllocNavMesh();
		if(!tnm){
			ERROR_FN(Locale::get("ERROR_NAV_DT_OUT_OF_MEMORY"), _fileName.c_str());
			return false;
		}

		dtStatus s = tnm->init(tileData, tileDataSize, DT_TILE_FREE_DATA);
		if(dtStatusFailed(s)){
			ERROR_FN(Locale::get("ERROR_NAV_DT_INIT"), _fileName.c_str());
			return false;
		}

		// Initialise all flags to something helpful.
		for(U32 i = 0; i < (U32)tnm->getMaxTiles(); ++i){
			const dtMeshTile* tile = ((const dtNavMesh*)tnm)->getTile(i);

			if(!tile->header) continue;

			const dtPolyRef base = tnm->getPolyRefBase(tile);

			for(U32 j = 0; j < (U32)tile->header->polyCount; ++j) {
				const dtPolyRef ref = base | j;
				U16 f = 0;
				tnm->getPolyFlags(ref, &f);
				tnm->setPolyFlags(ref, f | 1);
			}
		}

		return true;
	}
	
	void NavigationMesh::render(){

         RenderMode mode = _renderMode;
         if(_building)
         {
            mode = RENDER_NAVMESH;
            _debugDrawInterface->overrideColor(duRGBA(255, 0, 0, 80));
         }

         _navigationMeshLock.lock();
		 _debugDrawInterface->beginBatch();
         switch(mode)
         {
            case RENDER_NAVMESH:    if(nm) duDebugDrawNavMesh          (_debugDrawInterface, *nm, 0); break;
            case RENDER_CONTOURS:   if(cs) duDebugDrawContours         (_debugDrawInterface, *cs);    break;
            case RENDER_POLYMESH:   if(pm) duDebugDrawPolyMesh         (_debugDrawInterface, *pm);    break;
            case RENDER_DETAILMESH: if(pmd)duDebugDrawPolyMeshDetail   (_debugDrawInterface, *pmd);   break;
            case RENDER_PORTALS:    if(nm) duDebugDrawNavMeshPortals   (_debugDrawInterface, *nm);    break;
         }
         if(cs && _renderConnections && !_building)   duDebugDrawRegionConnections(_debugDrawInterface, *cs);
		 _debugDrawInterface->endBatch();
         _navigationMeshLock.unlock();
    }

#pragma message("ToDo: Enable file support for navMeshes! - Ionut")
	bool NavigationMesh::load(SceneGraphNode* const sgn){
		return false;
		/*
		if(!_fileName.length())
		 return false;
        std::string file = _fileName;
        if(!sgn){
            file.append("root_node");
        }else{
            file.append("node_[_" + sgn->getName() + "_]");
        }
		// Parse objects from level into RC-compatible format

		FILE* fp = fopen(file.c_str(), "rb");
		if(!fp) return false;

		// Read header.
		NavMeshSetHeader header;
		fread(&header, sizeof(NavMeshSetHeader), 1, fp);

		if(header.magic != NAVMESHSET_MAGIC){
			fclose(fp);
			return false;
		}

		if(header.version != NAVMESHSET_VERSION){
			fclose(fp);
			return false;
		}

		boost::mutex::scoped_lock(_navigationMeshLock);

		if(nm)	 dtFreeNavMesh(nm);

		nm = dtAllocNavMesh();

		if(!nm)	{
			fclose(fp);
			return false;
		}

		dtStatus status = nm->init(&header.params);

		if(dtStatusFailed(status)){
			fclose(fp);
			return false;
		}

		// Read tiles.
		for(U32 i = 0; i < (U32)header.numTiles; ++i){
			NavMeshTileHeader tileHeader;
			fread(&tileHeader, sizeof(tileHeader), 1, fp);
			if(!tileHeader.tileRef || !tileHeader.dataSize) return false;//break;

			U8* data = (U8*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);

			if(!data) return false;//break;

			memset(data, 0, tileHeader.dataSize);
			fread(data, tileHeader.dataSize, 1, fp);

			nm->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
		}

		fclose(fp);

		return true;*/
	}

	bool NavigationMesh::save(){
		if(!_fileName.length() || !nm)
		 return false;

		// Save our NavigationMesh into a file to load from next time
		FILE* fp = fopen(_fileName.c_str(), "wb");
		if(!fp)	 return false;

		boost::mutex::scoped_lock(_navigationMeshLock);

		// Store header.
		NavMeshSetHeader header;
		header.magic = NAVMESHSET_MAGIC;
		header.version = NAVMESHSET_VERSION;
		header.numTiles = 0;

		for(U32 i = 0; i < (U32)nm->getMaxTiles(); ++i)	{
			 const dtMeshTile* tile = ((const dtNavMesh*)nm)->getTile(i);
			if (!tile || !tile->header || !tile->dataSize) continue;
			header.numTiles++;
		}

		memcpy(&header.params, nm->getParams(), sizeof(dtNavMeshParams));
		fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);

		// Store tiles.
		for(U32 i = 0; i < (U32)nm->getMaxTiles(); ++i)	{
			const dtMeshTile* tile = ((const dtNavMesh*)nm)->getTile(i);
			if(!tile || !tile->header || !tile->dataSize) continue;

			NavMeshTileHeader tileHeader;
			tileHeader.tileRef = nm->getTileRef(tile);
			tileHeader.dataSize = tile->dataSize;
			fwrite(&tileHeader, sizeof(tileHeader), 1, fp);
			fwrite(tile->data, tile->dataSize, 1, fp);
		}

		fclose(fp);

		return true;
	}
};