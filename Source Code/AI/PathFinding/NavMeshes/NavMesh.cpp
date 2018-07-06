#include "Headers/NavMesh.h"
#include "Headers/NavMeshDebugDraw.h"
#include "../Headers/DivideRecast.h"

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
        _debugDraw = false;
        _renderConnections = false;
        _renderMode = RENDER_NAVMESH;
        _debugDrawInterface = New NavMeshDebugDraw();
        assert(_debugDrawInterface != NULL);
        _heightField  = NULL;
        _compactHeightField = NULL;
        _countourSet  = NULL;
        _polyMesh  = NULL;
        _polyMeshDetail = NULL;
        _navMesh  = NULL;
        _tempNavMesh = NULL;
        _navQuery = NULL;
        _building = false;
    }

    NavigationMesh::~NavigationMesh(){
        if(_buildThread)
            _buildThread->stopTask();

        freeIntermediates(true);
        dtFreeNavMesh(_navMesh);
        dtFreeNavMesh(_tempNavMesh);
        _navMesh = NULL;
        _tempNavMesh = NULL;
        if(_navQuery){
            dtFreeNavMeshQuery(_navQuery);
            _navQuery = 0 ;
        }
        SAFE_DELETE(_debugDrawInterface);
    }

    void NavigationMesh::freeIntermediates(bool freeAll){
        _navigationMeshLock.lock();

        rcFreeHeightField(_heightField);
        rcFreeCompactHeightfield(_compactHeightField);
        _heightField = NULL;
        _compactHeightField = NULL;

        if(!_saveIntermediates || freeAll)	{
            rcFreeContourSet(_countourSet);
            rcFreePolyMesh(_polyMesh);
            rcFreePolyMeshDetail(_polyMeshDetail);
            _countourSet = NULL;
            _polyMesh = NULL;
            _polyMeshDetail = NULL;
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

    bool NavigationMesh::build(SceneGraphNode* const sgn, CreationCallback creationCompleteCallback, bool threaded){
        if(!loadConfigFromFile()){
            ERROR_FN(Locale::get("NAV_MESH_CONFIG_NOT_FOUND"));
            return false;
        }

        _sgn = (sgn != NULL) ? sgn : _sgn = GET_ACTIVE_SCENEGRAPH()->getRoot();
        _loadCompleteClbk = creationCompleteCallback;

        if(_buildThreaded && threaded){
            return buildThreaded();
        }

        return buildProcess();
    }

    bool NavigationMesh::buildThreaded(){
        boost::mutex::scoped_lock sl(_buildLock);
        if(!sl.owns_lock()) 
            return false;

        if(_buildThread) 
            _buildThread->stopTask();

        Kernel* kernel = Application::getInstance().getKernel();
        _buildThread.reset(New Task(kernel->getThreadPool(),
                                    3,
                                    true,
                                    true,
                                    DELEGATE_BIND(&NavigationMesh::buildProcess,this)));

        return true;
    }
    
    bool NavigationMesh::buildProcess(){
        _building = true;
        // Create mesh
        D32 timeStart = GETTIME();
        bool success = generateMesh();
        D32 endTime = GETTIME(true) - timeStart;

        if(success){
            PRINT_FN(Locale::get("NAV_MESH_GENERATION_COMPLETE"),  endTime);
        }else{
            ERROR_FN(Locale::get("NAV_MESH_GENERATION_INCOMPLETE"),endTime);
        }

        _navigationMeshLock.lock();
        // Copy new NavigationMesh into old.
        dtNavMesh *old = _navMesh;
        // I am trusting that this is atomic.
        _navMesh = _tempNavMesh;
        dtFreeNavMesh(old);
        _debugDrawInterface->setDirty(true);
        _tempNavMesh = NULL;
        bool navQueryComplete = createNavigationQuery();
        assert(navQueryComplete);
        _navigationMeshLock.unlock();

        // Free structs used during build
        freeIntermediates(false);

        _building = false;

        if(!_loadCompleteClbk.empty())
            _loadCompleteClbk(this);

        return success;
    }

    bool NavigationMesh::generateMesh(){
        assert(_sgn != NULL);

        std::string nodeName((_sgn->getNode<SceneNode>()->getType() != TYPE_ROOT) ? "_node_[_" + _sgn->getName() + "_]" : "_root_node");
        // Parse objects from level into RC-compatible format
        _fileName.append(nodeName);
        _fileName.append(".nm");
        PRINT_FN(Locale::get("NAV_MESH_GENERATION_START"),nodeName.c_str());

        NavModelData data;
        std::string geometrySaveFile(_fileName);
        Util::replaceStringInPlace(geometrySaveFile,".nm",".ig");

        data.clear(false);
        data.setName(nodeName);

        if(!NavigationMeshLoader::loadMeshFile(data, geometrySaveFile.c_str())){
            if(!NavigationMeshLoader::parse(_sgn->getBoundingBox(), data, _sgn)){
                ERROR_FN(Locale::get("ERROR_NAV_PARSE_FAILED"), nodeName.c_str());
            }
        }

        // Check for no geometry
        if(!data.getVertCount()){
            data.isValid(false);
            return false;
        }

        // Free intermediate and final results
        freeIntermediates(true);
        // Recast initialisation data
        rcContextDivide ctx(true);

        rcConfig cfg;
        memset(&cfg, 0, sizeof(cfg));

        cfg.cs                     = _configParams.getCellSize();
        cfg.ch                     = _configParams.getCellHeight();
        cfg.walkableHeight         = _configParams.base_getWalkableHeight();
        cfg.walkableClimb          = _configParams.base_getWalkableClimb();
        cfg.walkableRadius         = _configParams.base_getWalkableRadius();
        cfg.walkableSlopeAngle     = _configParams.getAgentMaxSlope();
        cfg.borderSize             = _configParams.base_getWalkableRadius() + (I32)BORDER_PADDING;
        cfg.detailSampleDist       = _configParams.getDetailSampleDist();
        cfg.detailSampleMaxError   = _configParams.getDetailSampleMaxError();
        cfg.maxEdgeLen             = _configParams.getEdgeMaxLen();
        cfg.maxSimplificationError = _configParams.getEdgeMaxError();
        cfg.maxVertsPerPoly        = _configParams.getVertsPerPoly();
        cfg.minRegionArea          = _configParams.getRegionMinSize();
        cfg.mergeRegionArea        = _configParams.getRegionMergeSize();
        cfg.tileSize               = _configParams.getTileSize();

        _saveIntermediates = _configParams.getKeepInterResults();
        rcCalcBounds(data.getVerts(), data.getVertCount(), cfg.bmin, cfg.bmax);
        rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
        PRINT_FN(Locale::get("NAV_MESH_BOUNDS"),cfg.bmax[0],cfg.bmax[1],cfg.bmax[2],cfg.bmin[0],cfg.bmin[1],cfg.bmin[2]);

        _extents = vec3<F32>(cfg.bmax[0] - cfg.bmin[0],
                             cfg.bmax[1] - cfg.bmin[1],
                             cfg.bmax[2] - cfg.bmin[2]);

        if(!createPolyMesh(cfg, data, &ctx)){
            data.isValid(false);
            return false;
        }

        //Detour initialisation data
        dtNavMeshCreateParams params;
        memset(&params, 0, sizeof(params));
        rcVcopy(params.bmax, cfg.bmax);
        rcVcopy(params.bmin, cfg.bmin);

        params.ch = cfg.ch;
        params.cs = cfg.cs;
        params.walkableHeight = cfg.walkableHeight;
        params.walkableRadius = cfg.walkableRadius;
        params.walkableClimb  = cfg.walkableClimb;

        params.tileX     = 0;
        params.tileY     = 0;
        params.tileLayer = 0;
        params.buildBvTree = true;

        params.verts     = _polyMesh->verts;
        params.vertCount = _polyMesh->nverts;
        params.polys     = _polyMesh->polys;
        params.polyAreas = _polyMesh->areas;
        params.polyFlags = _polyMesh->flags;
        params.polyCount = _polyMesh->npolys;
        params.nvp       = _polyMesh->nvp;

        params.detailMeshes     = _polyMeshDetail->meshes;
        params.detailVerts      = _polyMeshDetail->verts;
        params.detailVertsCount = _polyMeshDetail->nverts;
        params.detailTris       = _polyMeshDetail->tris;
        params.detailTriCount   = _polyMeshDetail->ntris;

        if(!createNavigationMesh(params)){
            data.isValid(false);
            return false;
        }

        data.isValid(true);

        return NavigationMeshLoader::saveMeshFile(data, geometrySaveFile.c_str());//input geometry;
    }

    bool NavigationMesh::createNavigationQuery(U32 maxNodes){
        _navQuery = dtAllocNavMeshQuery();
        return (_navQuery->init(_navMesh, maxNodes) == DT_SUCCESS);
    }

    bool NavigationMesh::createPolyMesh(rcConfig &cfg, NavModelData &data, rcContextDivide *ctx){
        if(_fileName.empty())  _fileName = data.getName();

        // Create a heightfield to voxelise our input geometry
        _heightField = rcAllocHeightfield();

        if(!_heightField){
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

        if(!rcCreateHeightfield(ctx, *_heightField, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)){
            ERROR_FN(Locale::get("ERROR_NAV_HEIGHTFIELD"), _fileName.c_str());
            return false;
        }

        U8* areas = New U8[data.getTriCount()];

        if (!areas){
            ERROR_FN(Locale::get("ERROR_NAV_OUT_OF_MEMORY"), "areaFlag allocation", _fileName.c_str());
            return false;
        }

        memset(areas, 0, data.getTriCount()*sizeof(U8));

        // Filter triangles by angle and rasterize
        rcMarkWalkableTriangles(ctx,
                                cfg.walkableSlopeAngle,
                                data.getVerts(),
                                data.getVertCount(),
                                data.getTris(),
                                data.getTriCount(),
                                areas);

        rcRasterizeTriangles(ctx,
                             data.getVerts(),
                             data.getVertCount(),
                             data.getTris(),
                             areas,
                             data.getTriCount(),
                             *_heightField,
                             cfg.walkableClimb);

        if(!_saveIntermediates)	SAFE_DELETE_ARRAY(areas);

        // Filter out areas with low ceilings and other stuff
        rcFilterLowHangingWalkableObstacles(ctx,
                                            cfg.walkableClimb,
                                            *_heightField);

        rcFilterLedgeSpans(ctx,
                           cfg.walkableHeight,
                           cfg.walkableClimb,
                           *_heightField);

        rcFilterWalkableLowHeightSpans(ctx,
                                       cfg.walkableHeight,
                                       *_heightField);

        _compactHeightField = rcAllocCompactHeightfield();

        if(!_compactHeightField || !rcBuildCompactHeightfield(ctx,
                                                              cfg.walkableHeight,
                                                              cfg.walkableClimb,
                                                              *_heightField,
                                                              *_compactHeightField)){
            ERROR_FN(Locale::get("ERROR_NAV_COMPACT_HEIGHTFIELD"), _fileName.c_str());
            return false;
        }

        bool buildState =  rcErodeWalkableArea(ctx,
                                               cfg.walkableRadius,
                                               *_compactHeightField);

        if(!buildState)	{
            ERROR_FN(Locale::get("ERROR_NAV_WALKABLE"), _fileName.c_str());
            return false;
        }

        if(false){
            buildState = rcBuildRegionsMonotone(ctx,
                                                *_compactHeightField,
                                                cfg.borderSize,
                                                cfg.minRegionArea,
                                                cfg.mergeRegionArea);
            if(!buildState){
                ERROR_FN(Locale::get("ERROR_NAV_REGIONS"), _fileName.c_str());
                return false;
            }
        }else{
            buildState = rcBuildDistanceField(ctx,
                                              *_compactHeightField);
            if(!buildState)	return false;

            buildState = rcBuildRegions(ctx,
                                        *_compactHeightField,
                                        cfg.borderSize,
                                        cfg.minRegionArea,
                                        cfg.mergeRegionArea);
            if(!buildState)	return false;
        }

        _countourSet = rcAllocContourSet();
        if(!_countourSet || !rcBuildContours(ctx,
                                             *_compactHeightField,
                                             cfg.maxSimplificationError,
                                             cfg.maxEdgeLen,
                                             *_countourSet)){
            ERROR_FN(Locale::get("ERROR_NAV_COUNTOUR"), _fileName.c_str());
            return false;
        }

        _polyMesh = rcAllocPolyMesh();
        if(!_polyMesh || !rcBuildPolyMesh(ctx,
                                          *_countourSet,
                                          cfg.maxVertsPerPoly,
                                          *_polyMesh)){
            ERROR_FN(Locale::get("ERROR_NAV_POLY_MESH"), _fileName.c_str());
            return false;
        }

        _polyMeshDetail = rcAllocPolyMeshDetail();
        if(!_polyMeshDetail || !rcBuildPolyMeshDetail(ctx,
                                                      *_polyMesh,
                                                      *_compactHeightField,
                                                      cfg.detailSampleDist,
                                                      cfg.detailSampleMaxError,
                                                      *_polyMeshDetail)){
            ERROR_FN(Locale::get("ERROR_NAV_POLY_MESH_DETAIL"), _fileName.c_str());
            return false;
        }

        // Show performance stats.
        ctx->stopTimer(RC_TIMER_TOTAL);
        duLogBuildTimes(*ctx, ctx->getAccumulatedTime(RC_TIMER_TOTAL));
        ctx->log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", _polyMesh->nverts, _polyMesh->npolys);
        F32 totalBuildTimeMs = (F32)ctx->getAccumulatedTime(RC_TIMER_TOTAL)/1000.0f;

        PRINT_FN("[RC_LOG_PROGRESS] Polymesh: %d vertices  %d polygons %5.2f ms\n", _polyMesh->nverts, _polyMesh->npolys,totalBuildTimeMs);

        return true;
    }

    bool NavigationMesh::createNavigationMesh(dtNavMeshCreateParams &params){
        U8 *tileData = NULL;
        I32 tileDataSize = 0;
        if(!dtCreateNavMeshData(&params, &tileData, &tileDataSize)){
            ERROR_FN(Locale::get("ERROR_NAV_MESH_DATA"), _fileName.c_str());
            return false;
        }

        _tempNavMesh = dtAllocNavMesh();
        if(!_tempNavMesh){
            ERROR_FN(Locale::get("ERROR_NAV_DT_OUT_OF_MEMORY"), _fileName.c_str());
            return false;
        }

        dtStatus s = _tempNavMesh->init(tileData, tileDataSize, DT_TILE_FREE_DATA);
        if(dtStatusFailed(s)){
            ERROR_FN(Locale::get("ERROR_NAV_DT_INIT"), _fileName.c_str());
            return false;
        }

        // Initialise all flags to something helpful.
        for(U32 i = 0; i < (U32)_tempNavMesh->getMaxTiles(); ++i){
            const dtMeshTile* tile = ((const dtNavMesh*)_tempNavMesh)->getTile(i);

            if(!tile->header) continue;

            const dtPolyRef base = _tempNavMesh->getPolyRefBase(tile);

            for(U32 j = 0; j < (U32)tile->header->polyCount; ++j) {
                const dtPolyRef ref = base | j;
                U16 f = 0;
                _tempNavMesh->getPolyFlags(ref, &f);
                _tempNavMesh->setPolyFlags(ref, f | 1);
            }
        }

        return true;
    }

    void NavigationMesh::update(const U64 deltaTime) {
       _debugDrawInterface->paused(!_debugDraw);
    }

    void NavigationMesh::render(){
         RenderMode mode = _renderMode;

         if(_building) {
            mode = RENDER_NAVMESH;
            _debugDrawInterface->overrideColor(duRGBA(255, 0, 0, 80));
         }

         _debugDrawInterface->beginBatch();

         _navigationMeshLock.lock();
         switch(mode)
         {
            case RENDER_NAVMESH:
                if(_navMesh)        duDebugDrawNavMesh        (_debugDrawInterface, *_navMesh, 0);
                break;
            case RENDER_CONTOURS:
                if(_countourSet)    duDebugDrawContours       (_debugDrawInterface, *_countourSet);
                break;
            case RENDER_POLYMESH:
                if(_polyMesh)       duDebugDrawPolyMesh       (_debugDrawInterface, *_polyMesh);
                break;
            case RENDER_DETAILMESH:
                if(_polyMeshDetail) duDebugDrawPolyMeshDetail (_debugDrawInterface, *_polyMeshDetail);
                break;
            case RENDER_PORTALS:
                if(_navMesh)        duDebugDrawNavMeshPortals (_debugDrawInterface, *_navMesh);
                break;
         }

         if(!_building){
            if(_countourSet && _renderConnections)   duDebugDrawRegionConnections(_debugDrawInterface, *_countourSet);
         }

         _navigationMeshLock.unlock();

         _debugDrawInterface->endBatch();
    }

    bool NavigationMesh::load(SceneGraphNode* const sgn){
        if(!_fileName.length()) 
            return false;

        std::string file = _fileName;

        if(sgn == NULL){
            file.append("_root_node");
        }else{
            file.append("_node_[_" + sgn->getName() + "_]");
        }

        file.append(".nm");
        // Parse objects from level into RC-compatible format
        FILE* fp = fopen(file.c_str(), "rb");
        if(!fp)
            return false;

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

        if(_navMesh) 
            dtFreeNavMesh(_navMesh);

        _navMesh = dtAllocNavMesh();

        if(!_navMesh)	{
            fclose(fp);
            return false;
        }

        dtStatus status = _navMesh->init(&header.params);

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

            _navMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
        }
        fclose(fp);

        _extents.set(header.extents[0],
                     header.extents[1],
                     header.extents[2]);

        return createNavigationQuery();
    }

    bool NavigationMesh::save(){
        if(!_fileName.length() || !_navMesh)
            return false;

        // Save our NavigationMesh into a file to load from next time
        FILE* fp = fopen(_fileName.c_str(), "wb");
        if(!fp)	return false;

        boost::mutex::scoped_lock(_navigationMeshLock);

        // Store header.
        NavMeshSetHeader header;
        memcpy(header.extents, &_extents[0], sizeof(F32) * 3);

        header.magic = NAVMESHSET_MAGIC;
        header.version = NAVMESHSET_VERSION;
        header.numTiles = 0;

        for(U32 i = 0; i < (U32)_navMesh->getMaxTiles(); ++i)	{
            const dtMeshTile* tile = ((const dtNavMesh*)_navMesh)->getTile(i);

            if (!tile || !tile->header || !tile->dataSize) continue;
            header.numTiles++;
        }

        memcpy(&header.params, _navMesh->getParams(), sizeof(dtNavMeshParams));
        fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);

        // Store tiles.
        for(U32 i = 0; i < (U32)_navMesh->getMaxTiles(); ++i)	{
            const dtMeshTile* tile = ((const dtNavMesh*)_navMesh)->getTile(i);

            if(!tile || !tile->header || !tile->dataSize) continue;

            NavMeshTileHeader tileHeader;
            tileHeader.tileRef = _navMesh->getTileRef(tile);
            tileHeader.dataSize = tile->dataSize;

            fwrite(&tileHeader, sizeof(tileHeader), 1, fp);
            fwrite(tile->data, tile->dataSize, 1, fp);
        }

        fclose(fp);

        return true;
    }
};