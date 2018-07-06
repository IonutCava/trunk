#include "Headers/NavigationMesh.h"
#include "Managers/Headers/SceneManager.h"
namespace Navigation {

	const U32 NavigationMesh::_maxVertsPerPoly = 3;

	NavigationMesh::NavigationMesh() {

		_fileName = std::string("");

		_buildThreaded = true;

		_saveIntermediates = false;

		_cellSize = _cellHeight = 0.3f;
		_walkableHeight = 2.0f;
		_walkableClimb = 1.0f;
		_walkableRadius = 0.5f;
		_walkableSlope = 45.0f;
		_borderSize = 1;
		_detailSampleDist = 6.0f;
		_detailSampleMaxError = 1.0f;
		_maxEdgeLen = 12;
		_maxSimplificationError = 1.3f;
		_minRegionArea = 8;
		_mergeRegionArea = 20;
		_tileSize = 32;

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

		if(mThread)	{
			mThread->interruptEvent();
		}

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

	bool NavigationMesh::build(SceneGraphNode* const sgn){

		if(sgn){
			_sgn = sgn;
		}else{	/// use root node
			_sgn = GET_ACTIVE_SCENE()->getSceneGraph()->getRoot();
		}

		if(_buildThreaded)
		 return buildThreaded();

		boost::mutex::scoped_lock(_buildLock);
		if(!_buildLock.owns_lock()) return false;

		return buildProcess();
	}

	bool NavigationMesh::buildThreaded(){

		boost::mutex::scoped_lock(_buildLock);
		if(!_buildLock.owns_lock()) return false;

		if(mThread) {
			mThread->interruptEvent();
		}

		mThread.reset(New Event(3,true,true,boost::bind(&NavigationMesh::launchThreadedBuild,this)));

		return true;
	}

	void NavigationMesh::launchThreadedBuild(void *data){

		NavigationMesh *pThis = (NavigationMesh*)data;
		pThis->buildProcess();
	}

	bool NavigationMesh::buildProcess(){
		_building = true;
		// Create mesh
		bool success = generateMesh();

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
		// Parse objects from level into RC-compatible format
		NavModelData data = NavigationMeshLoader::parseNode(_sgn);

		// Check for no geometry
		if(!data.getVertCount())
		 return false;

		// Free intermediate and final results
		freeIntermediates(true);


		// Recast initialisation data
		rcContext ctx(false);
		rcConfig cfg;

		memset(&cfg, 0, sizeof(cfg));
		cfg.cs = _cellSize;
		cfg.ch = _cellHeight;
		rcCalcBounds(data.verts, data.getVertCount(), cfg.bmin, cfg.bmax);
		rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

		cfg.walkableHeight         = (F32)ceil(_walkableHeight / _cellHeight);
		cfg.walkableClimb          = (F32)ceil(_walkableClimb  / _cellHeight);
		cfg.walkableRadius         = (F32)ceil(_walkableRadius / _cellSize);
		cfg.walkableSlopeAngle     = _walkableSlope;
		cfg.borderSize             = _borderSize;
		cfg.detailSampleDist       = _detailSampleDist;
		cfg.detailSampleMaxError   = _detailSampleMaxError;
		cfg.maxEdgeLen             = _maxEdgeLen;
		cfg.maxSimplificationError = _maxSimplificationError;
		cfg.maxVertsPerPoly        = _maxVertsPerPoly;
		cfg.minRegionArea          = _minRegionArea;
		cfg.mergeRegionArea        = _mergeRegionArea;
		cfg.tileSize               = _tileSize;


		if(!createPolyMesh(cfg, data, &ctx))
		 return false;

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

		if(!createNavigationMesh(params))
		 return false;

		return true;
	}

	bool NavigationMesh::createPolyMesh(rcConfig &cfg, NavModelData &data, rcContext *ctx){
		// Create a heightfield to voxelise our input geometry
		hf = rcAllocHeightfield();

		if(!hf || !rcCreateHeightfield(ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)){
			ERROR_FN(Locale::get("ERROR_NAV_HEIGHTFIELD"), _fileName.c_str());
			return false;
		}

		U8* areas = New U8[data.getTriCount()];

		if (!areas){
			ERROR_FN(Locale::get("ERROR_NAV_OUT_OF_MEMORY"), _fileName.c_str());
			return false;
		}

		memset(areas, 0, data.getTriCount()*sizeof(U8));

		// Subtract 1 from all indices!
		for(U32 i = 0; i < data.getTriCount(); i++){
			data.tris[i*3]--;
			data.tris[i*3+1]--;
			data.tris[i*3+2]--;
		}

		// Filter triangles by angle and rasterize
		rcMarkWalkableTriangles(ctx, cfg.walkableSlopeAngle,
			data.getVerts(), data.getVertCount(),
			data.getTris(), data.getTriCount(), areas);

		rcRasterizeTriangles(ctx, data.getVerts(), data.getVertCount(),
			data.getTris(), areas, data.getTriCount(),
			*hf, cfg.walkableClimb);

		SAFE_DELETE_ARRAY(areas);

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

	bool NavigationMesh::load(){

		if(!_fileName.length())
		 return false;

		FILE* fp = fopen(_fileName.c_str(), "rb");
		if(!fp) return false;

		// Read header.
		NavMeshSetHeader header;
		fread(&header, sizeof(NavMeshSetHeader), 1, fp);

		if(header.magic != NAVMESHSET_MAGIC){
			fclose(fp);
			return 0;
		}

		if(header.version != NAVMESHSET_VERSION){
			fclose(fp);
			return 0;
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
			if(!tileHeader.tileRef || !tileHeader.dataSize)
				break;

			U8* data = (U8*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);

			if(!data) break;

			memset(data, 0, tileHeader.dataSize);
			fread(data, tileHeader.dataSize, 1, fp);

			nm->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
		}

		fclose(fp);

		return true;
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