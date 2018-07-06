#include "Headers/NavMesh.h"
#include "Headers/NavMeshDebugDraw.h"
#include "../Headers/DivideRecast.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Managers/Headers/SceneManager.h"

#include <ReCast/DebugUtils/Include/RecastDump.h>
#include <ReCast/DebugUtils/Include/DetourDebugDraw.h>
#include <ReCast/DebugUtils/Include/RecastDebugDraw.h>

#include <SimpleIni.h>

namespace Divide {
namespace AI {
namespace Navigation {

NavigationMesh::NavigationMesh() : GUIDWrapper() {
    ParamHandler& par = ParamHandler::getInstance();
    stringImpl path(par.getParam<stringImpl>("scriptLocation") + "/" +
                    par.getParam<stringImpl>("scenesLocation") + "/" +
                    par.getParam<stringImpl>("currentScene"));

    _debugDrawInterface.reset(new NavMeshDebugDraw());

    _fileName = path + "/navMeshes/";
    _configFile = path + "/navMeshConfig.ini";
    _buildThreaded = true;
    _debugDraw = false;
    _renderConnections = false;
    _renderMode = RenderMode::RENDER_NAVMESH;
    _heightField = nullptr;
    _compactHeightField = nullptr;
    _countourSet = nullptr;
    _polyMesh = nullptr;
    _polyMeshDetail = nullptr;
    _navMesh = nullptr;
    _tempNavMesh = nullptr;
    _navQuery = nullptr;
    _building = false;
}

NavigationMesh::~NavigationMesh() { unload(); }

bool NavigationMesh::unload() {
    if (_buildThread) {
        _buildThread->stopTask();
        while (!_buildThread->isFinished()) {
        }
    }

    if (_navQuery) {
        dtFreeNavMeshQuery(_navQuery);
        _navQuery = 0;
    }

    freeIntermediates(true);
    dtFreeNavMesh(_navMesh);
    dtFreeNavMesh(_tempNavMesh);
    _navMesh = nullptr;
    _tempNavMesh = nullptr;

    return true;
}

void NavigationMesh::freeIntermediates(bool freeAll) {
    _navigationMeshLock.lock();

    rcFreeHeightField(_heightField);
    rcFreeCompactHeightfield(_compactHeightField);
    _heightField = nullptr;
    _compactHeightField = nullptr;

    if (!_saveIntermediates || freeAll) {
        rcFreeContourSet(_countourSet);
        rcFreePolyMesh(_polyMesh);
        rcFreePolyMeshDetail(_polyMeshDetail);
        _countourSet = nullptr;
        _polyMesh = nullptr;
        _polyMeshDetail = nullptr;
    }

    _navigationMeshLock.unlock();
}

namespace {
inline F32 charToFloat(const char* val) {
    return Util::convertData<F32, stringImpl>(stringImpl(val));
}

inline bool charToBool(const char* val) {
    return Util::convertData<bool, stringImpl>(stringImpl(val));
}
};

bool NavigationMesh::loadConfigFromFile() {
    // Use SimpleIni library for cross-platform INI parsing
    CSimpleIniA ini;
    ini.SetUnicode();
    ini.LoadFile(_configFile.c_str());

    if (!ini.GetSection("Rasterization") || !ini.GetSection("Agent") ||
        !ini.GetSection("Region") || !ini.GetSection("Polygonization") ||
        !ini.GetSection("DetailMesh"))
        return false;

    // Load all key-value pairs for the "Rasterization" section
    _configParams.setCellSize(
        charToFloat(ini.GetValue("Rasterization", "fCellSize", "0.3")));
    _configParams.setCellHeight(
        charToFloat(ini.GetValue("Rasterization", "fCellHeight", "0.2")));
    _configParams.setTileSize(
        charToFloat(ini.GetValue("Rasterization", "iTileSize", "48")));
    // Load all key-value pairs for the "Agent" section
    _configParams.setAgentHeight(
        charToFloat(ini.GetValue("Agent", "fAgentHeight", "2.5")));
    _configParams.setAgentRadius(
        charToFloat(ini.GetValue("Agent", "fAgentRadius", "0.5")));
    _configParams.setAgentMaxClimb(
        charToFloat(ini.GetValue("Agent", "fAgentMaxClimb", "1")));
    _configParams.setAgentMaxSlope(
        charToFloat(ini.GetValue("Agent", "fAgentMaxSlope", "20")));
    // Load all key-value pairs for the "Region" section
    _configParams.setRegionMergeSize(
        charToFloat(ini.GetValue("Region", "fMergeSize", "20")));
    _configParams.setRegionMinSize(
        charToFloat(ini.GetValue("Region", "fMinSize", "50")));
    // Load all key-value pairs for the "Polygonization" section
    _configParams.setEdgeMaxLen(
        charToFloat(ini.GetValue("Polygonization", "fEdgeMaxLength", "12")));
    _configParams.setEdgeMaxError(
        charToFloat(ini.GetValue("Polygonization", "fEdgeMaxError", "1.3")));
    _configParams.setVertsPerPoly(
        charToFloat(ini.GetValue("Polygonization", "iVertsPerPoly", "6")));
    // Load all key-value pairs for the "DetailMesh" section
    _configParams.setDetailSampleDist(
        charToFloat(ini.GetValue("DetailMesh", "fDetailSampleDist", "6")));
    _configParams.setDetailSampleMaxError(
        charToFloat(ini.GetValue("DetailMesh", "fDetailSampleMaxError", "1")));
    _configParams.setKeepInterResults(
        charToBool(ini.GetValue("DetailMesh", "bKeepInterResults", "false")));

    return true;
}

bool NavigationMesh::build(SceneGraphNode& sgn,
                           CreationCallback creationCompleteCallback,
                           bool threaded) {
    if (!loadConfigFromFile()) {
        Console::errorfn(Locale::get("NAV_MESH_CONFIG_NOT_FOUND"));
        return false;
    }

    _sgn = &sgn;
    _loadCompleteClbk = creationCompleteCallback;

    if (_buildThreaded && threaded) {
        return buildThreaded();
    }

    return buildProcess();
}

bool NavigationMesh::buildThreaded() {
    boost::mutex::scoped_lock sl(_buildLock);

    if (!sl.owns_lock()) {
        return false;
    }

    if (_buildThread) {
        _buildThread->stopTask();
    }

    Kernel& kernel = Application::getInstance().getKernel();
    _buildThread.reset(kernel.AddTask(
        0, 0, DELEGATE_BIND(&NavigationMesh::buildInternal, this)));
    _buildThread->startTask();
    return true;
}

void NavigationMesh::buildInternal() {
    _building = true;
    // Create mesh
    D32 timeStart = Time::ElapsedSeconds();
    bool success = generateMesh();
    if (!success) {
        Console::errorfn(Locale::get("NAV_MESH_GENERATION_INCOMPLETE"),
                         Time::ElapsedSeconds(true) - timeStart);
        return;
    }

    Console::printfn(Locale::get("NAV_MESH_GENERATION_COMPLETE"),
                     Time::ElapsedSeconds(true) - timeStart);

    _navigationMeshLock.lock();
    {
        // Copy new NavigationMesh into old.
        dtNavMesh* old = _navMesh;
        // I am trusting that this is atomic.
        _navMesh = _tempNavMesh;
        dtFreeNavMesh(old);
        _debugDrawInterface->setDirty(true);
        _tempNavMesh = nullptr;

        bool navQueryComplete = createNavigationQuery();
        DIVIDE_ASSERT(
            navQueryComplete,
            "NavigationMesh Error: Navigation query creation failed!");
    }
    _navigationMeshLock.unlock();

    // Free structs used during build
    freeIntermediates(false);

    _building = false;

    if (_loadCompleteClbk) {
        _loadCompleteClbk(this);
    }
}

bool NavigationMesh::buildProcess() {
    _building = true;
    // Create mesh
    D32 timeStart = Time::ElapsedSeconds();
    bool success = generateMesh();
    if (!success) {
        Console::errorfn(Locale::get("NAV_MESH_GENERATION_INCOMPLETE"),
                         Time::ElapsedSeconds(true) - timeStart);
        return false;
    }

    Console::printfn(Locale::get("NAV_MESH_GENERATION_COMPLETE"),
                     Time::ElapsedSeconds(true) - timeStart);

    _navigationMeshLock.lock();
    {
        // Copy new NavigationMesh into old.
        dtNavMesh* old = _navMesh;
        // I am trusting that this is atomic.
        _navMesh = _tempNavMesh;
        dtFreeNavMesh(old);
        _debugDrawInterface->setDirty(true);
        _tempNavMesh = nullptr;

        bool navQueryComplete = createNavigationQuery();
        DIVIDE_ASSERT(
            navQueryComplete,
            "NavigationMesh Error: Navigation query creation failed!");
    }
    _navigationMeshLock.unlock();

    // Free structs used during build
    freeIntermediates(false);

    _building = false;

    if (_loadCompleteClbk) {
        _loadCompleteClbk(this);
    }

    return success;
}

bool NavigationMesh::generateMesh() {
    assert(_sgn != nullptr);

    stringImpl nodeName((_sgn->getNode()->getType() != SceneNodeType::TYPE_ROOT)
                            ? "_node_[_" + _sgn->getName() + "_]"
                            : "_root_node");

    // Parse objects from level into RC-compatible format
    _fileName.append(nodeName);
    _fileName.append(".nm");
    Console::printfn(Locale::get("NAV_MESH_GENERATION_START"),
                     nodeName.c_str());

    NavModelData data;
    stringImpl geometrySaveFile(_fileName);
    Util::replaceStringInPlace(geometrySaveFile, ".nm", ".ig");

    data.clear(false);
    data.setName(nodeName);

    if (!NavigationMeshLoader::loadMeshFile(data, geometrySaveFile.c_str())) {
        if (!NavigationMeshLoader::parse(_sgn->getBoundingBoxConst(), data,
                                         _sgn)) {
            Console::errorfn(Locale::get("ERROR_NAV_PARSE_FAILED"),
                             nodeName.c_str());
        }
    }

    // Check for no geometry
    if (!data.getVertCount()) {
        data.isValid(false);
        return false;
    }

    // Free intermediate and final results
    freeIntermediates(true);
    // Recast initialisation data
    rcContextDivide ctx(true);

    rcConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.cs = _configParams.getCellSize();
    cfg.ch = _configParams.getCellHeight();
    cfg.walkableHeight = _configParams.base_getWalkableHeight();
    cfg.walkableClimb = _configParams.base_getWalkableClimb();
    cfg.walkableRadius = _configParams.base_getWalkableRadius();
    cfg.walkableSlopeAngle = _configParams.getAgentMaxSlope();
    cfg.borderSize =
        _configParams.base_getWalkableRadius() + (I32)BORDER_PADDING;
    cfg.detailSampleDist = _configParams.getDetailSampleDist();
    cfg.detailSampleMaxError = _configParams.getDetailSampleMaxError();
    cfg.maxEdgeLen = _configParams.getEdgeMaxLen();
    cfg.maxSimplificationError = _configParams.getEdgeMaxError();
    cfg.maxVertsPerPoly = _configParams.getVertsPerPoly();
    cfg.minRegionArea = _configParams.getRegionMinSize();
    cfg.mergeRegionArea = _configParams.getRegionMergeSize();
    cfg.tileSize = _configParams.getTileSize();

    _saveIntermediates = _configParams.getKeepInterResults();
    rcCalcBounds(data.getVerts(), data.getVertCount(), cfg.bmin, cfg.bmax);
    rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
    Console::printfn(Locale::get("NAV_MESH_BOUNDS"), cfg.bmax[0], cfg.bmax[1],
                     cfg.bmax[2], cfg.bmin[0], cfg.bmin[1], cfg.bmin[2]);

    _extents = vec3<F32>(cfg.bmax[0] - cfg.bmin[0], cfg.bmax[1] - cfg.bmin[1],
                         cfg.bmax[2] - cfg.bmin[2]);

    if (!createPolyMesh(cfg, data, &ctx)) {
        data.isValid(false);
        return false;
    }

    // Detour initialisation data
    dtNavMeshCreateParams params;
    memset(&params, 0, sizeof(params));
    rcVcopy(params.bmax, cfg.bmax);
    rcVcopy(params.bmin, cfg.bmin);

    params.ch = cfg.ch;
    params.cs = cfg.cs;
    params.walkableHeight = cfg.walkableHeight;
    params.walkableRadius = cfg.walkableRadius;
    params.walkableClimb = cfg.walkableClimb;

    params.tileX = 0;
    params.tileY = 0;
    params.tileLayer = 0;
    params.buildBvTree = true;

    params.verts = _polyMesh->verts;
    params.vertCount = _polyMesh->nverts;
    params.polys = _polyMesh->polys;
    params.polyAreas = _polyMesh->areas;
    params.polyFlags = _polyMesh->flags;
    params.polyCount = _polyMesh->npolys;
    params.nvp = _polyMesh->nvp;

    params.detailMeshes = _polyMeshDetail->meshes;
    params.detailVerts = _polyMeshDetail->verts;
    params.detailVertsCount = _polyMeshDetail->nverts;
    params.detailTris = _polyMeshDetail->tris;
    params.detailTriCount = _polyMeshDetail->ntris;

    if (!createNavigationMesh(params)) {
        data.isValid(false);
        return false;
    }

    data.isValid(true);

    return NavigationMeshLoader::saveMeshFile(
        data, geometrySaveFile.c_str());  // input geometry;
}

bool NavigationMesh::createNavigationQuery(U32 maxNodes) {
    _navQuery = dtAllocNavMeshQuery();
    return (_navQuery->init(_navMesh, maxNodes) == DT_SUCCESS);
}

bool NavigationMesh::createPolyMesh(rcConfig& cfg, NavModelData& data,
                                    rcContextDivide* ctx) {
    if (_fileName.empty()) {
        _fileName = data.getName();
    }

    // Create a heightfield to voxelise our input geometry
    _heightField = rcAllocHeightfield();

    if (!_heightField) {
        Console::errorfn(Locale::get("ERROR_NAV_OUT_OF_MEMORY"),
                         "rcAllocHeightfield", _fileName.c_str());
        return false;
    }

    // Reset build times gathering.
    ctx->resetTimers();
    // Start the build process.
    ctx->startTimer(RC_TIMER_TOTAL);
    ctx->log(RC_LOG_PROGRESS, "Building navigation:");
    ctx->log(RC_LOG_PROGRESS, " - %d x %d cells", cfg.width, cfg.height);
    ctx->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris",
             data.getVertCount() / 1000.0f, data.getTriCount() / 1000.0f);

    if (!rcCreateHeightfield(ctx, *_heightField, cfg.width, cfg.height,
                             cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)) {
        Console::errorfn(Locale::get("ERROR_NAV_HEIGHTFIELD"),
                         _fileName.c_str());
        return false;
    }

    U8* areas = MemoryManager_NEW U8[data.getTriCount()];

    if (!areas) {
        Console::errorfn(Locale::get("ERROR_NAV_OUT_OF_MEMORY"),
                         "areaFlag allocation", _fileName.c_str());
        return false;
    }

    memset(areas, 0, data.getTriCount() * sizeof(U8));

    // Filter triangles by angle and rasterize
    rcMarkWalkableTriangles(ctx, cfg.walkableSlopeAngle, data.getVerts(),
                            data.getVertCount(), data.getTris(),
                            data.getTriCount(), areas);

    rcRasterizeTriangles(ctx, data.getVerts(), data.getVertCount(),
                         data.getTris(), areas, data.getTriCount(),
                         *_heightField, cfg.walkableClimb);

    if (!_saveIntermediates) {
        MemoryManager::DELETE_ARRAY(areas);
    }

    // Filter out areas with low ceilings and other stuff
    rcFilterLowHangingWalkableObstacles(ctx, cfg.walkableClimb, *_heightField);

    rcFilterLedgeSpans(ctx, cfg.walkableHeight, cfg.walkableClimb,
                       *_heightField);

    rcFilterWalkableLowHeightSpans(ctx, cfg.walkableHeight, *_heightField);

    _compactHeightField = rcAllocCompactHeightfield();

    if (!_compactHeightField ||
        !rcBuildCompactHeightfield(ctx, cfg.walkableHeight, cfg.walkableClimb,
                                   *_heightField, *_compactHeightField)) {
        Console::errorfn(Locale::get("ERROR_NAV_COMPACT_HEIGHTFIELD"),
                         _fileName.c_str());
        return false;
    }

    if (!rcErodeWalkableArea(ctx, cfg.walkableRadius, *_compactHeightField)) {
        Console::errorfn(Locale::get("ERROR_NAV_WALKABLE"), _fileName.c_str());
        return false;
    }

    if (false) {
        if (!rcBuildRegionsMonotone(ctx, *_compactHeightField, cfg.borderSize,
                                    cfg.minRegionArea, cfg.mergeRegionArea)) {
            Console::errorfn(Locale::get("ERROR_NAV_REGIONS"),
                             _fileName.c_str());
            return false;
        }
    } else {
        if (!rcBuildDistanceField(ctx, *_compactHeightField)) {
            return false;
        }

        if (!rcBuildRegions(ctx, *_compactHeightField, cfg.borderSize,
                            cfg.minRegionArea, cfg.mergeRegionArea)) {
            return false;
        }
    }

    _countourSet = rcAllocContourSet();
    if (!_countourSet ||
        !rcBuildContours(ctx, *_compactHeightField, cfg.maxSimplificationError,
                         cfg.maxEdgeLen, *_countourSet)) {
        Console::errorfn(Locale::get("ERROR_NAV_COUNTOUR"), _fileName.c_str());
        return false;
    }

    _polyMesh = rcAllocPolyMesh();
    if (!_polyMesh ||
        !rcBuildPolyMesh(ctx, *_countourSet, cfg.maxVertsPerPoly, *_polyMesh)) {
        Console::errorfn(Locale::get("ERROR_NAV_POLY_MESH"), _fileName.c_str());
        return false;
    }

    _polyMeshDetail = rcAllocPolyMeshDetail();
    if (!_polyMeshDetail ||
        !rcBuildPolyMeshDetail(ctx, *_polyMesh, *_compactHeightField,
                               cfg.detailSampleDist, cfg.detailSampleMaxError,
                               *_polyMeshDetail)) {
        Console::errorfn(Locale::get("ERROR_NAV_POLY_MESH_DETAIL"),
                         _fileName.c_str());
        return false;
    }

    // Show performance stats.
    ctx->stopTimer(RC_TIMER_TOTAL);
    duLogBuildTimes(*ctx, ctx->getAccumulatedTime(RC_TIMER_TOTAL));
    ctx->log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons",
             _polyMesh->nverts, _polyMesh->npolys);

    Console::printfn(
        "[RC_LOG_PROGRESS] Polymesh: %d vertices  %d polygons %5.2f ms\n",
        _polyMesh->nverts, _polyMesh->npolys,
        (F32)ctx->getAccumulatedTime(RC_TIMER_TOTAL) / 1000.0f);

    return true;
}

bool NavigationMesh::createNavigationMesh(dtNavMeshCreateParams& params) {
    U8* tileData = nullptr;
    I32 tileDataSize = 0;
    if (!dtCreateNavMeshData(&params, &tileData, &tileDataSize)) {
        Console::errorfn(Locale::get("ERROR_NAV_MESH_DATA"), _fileName.c_str());
        return false;
    }

    _tempNavMesh = dtAllocNavMesh();
    if (!_tempNavMesh) {
        Console::errorfn(Locale::get("ERROR_NAV_DT_OUT_OF_MEMORY"),
                         _fileName.c_str());
        return false;
    }

    dtStatus s = _tempNavMesh->init(tileData, tileDataSize, DT_TILE_FREE_DATA);
    if (dtStatusFailed(s)) {
        Console::errorfn(Locale::get("ERROR_NAV_DT_INIT"), _fileName.c_str());
        return false;
    }

    // Initialise all flags to something helpful.
    for (U32 i = 0; i < (U32)_tempNavMesh->getMaxTiles(); ++i) {
        const dtMeshTile* tile = ((const dtNavMesh*)_tempNavMesh)->getTile(i);

        if (!tile->header) {
            continue;
        }

        const dtPolyRef base = _tempNavMesh->getPolyRefBase(tile);

        for (U32 j = 0; j < (U32)tile->header->polyCount; ++j) {
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

void NavigationMesh::render() {
    RenderMode mode = _renderMode;

    if (_building) {
        mode = RenderMode::RENDER_NAVMESH;
        _debugDrawInterface->overrideColor(duRGBA(255, 0, 0, 80));
    }

    _debugDrawInterface->beginBatch();

    _navigationMeshLock.lock();

    switch (mode) {
        case RenderMode::RENDER_NAVMESH:
            if (_navMesh) {
                duDebugDrawNavMesh(_debugDrawInterface.get(), *_navMesh, 0);
            }
            break;
        case RenderMode::RENDER_CONTOURS:
            if (_countourSet) {
                duDebugDrawContours(_debugDrawInterface.get(), *_countourSet);
            }
            break;
        case RenderMode::RENDER_POLYMESH:
            if (_polyMesh) {
                duDebugDrawPolyMesh(_debugDrawInterface.get(), *_polyMesh);
            }
            break;
        case RenderMode::RENDER_DETAILMESH:
            if (_polyMeshDetail) {
                duDebugDrawPolyMeshDetail(_debugDrawInterface.get(),
                                          *_polyMeshDetail);
            }
            break;
        case RenderMode::RENDER_PORTALS:
            if (_navMesh) {
                duDebugDrawNavMeshPortals(_debugDrawInterface.get(), *_navMesh);
            }
            break;
    }

    if (!_building) {
        if (_countourSet && _renderConnections) {
            duDebugDrawRegionConnections(_debugDrawInterface.get(),
                                         *_countourSet);
        }
    }

    _navigationMeshLock.unlock();

    _debugDrawInterface->endBatch();
}

bool NavigationMesh::load(SceneGraphNode& sgn) {
    if (!_fileName.length()) {
        return false;
    }
    stringImpl file = _fileName;

    file.append("_node_[_" + sgn.getName() + "_]");

    file.append(".nm");
    // Parse objects from level into RC-compatible format
    FILE* fp = fopen(file.c_str(), "rb");
    if (!fp) {
        return false;
    }
    // Read header.
    NavMeshSetHeader header;
    fread(&header, sizeof(NavMeshSetHeader), 1, fp);

    if (header.magic != NAVMESHSET_MAGIC) {
        fclose(fp);
        return false;
    }

    if (header.version != NAVMESHSET_VERSION) {
        fclose(fp);
        return false;
    }

    boost::mutex::scoped_lock(_navigationMeshLock);

    if (_navMesh) {
        dtFreeNavMesh(_navMesh);
    }

    _navMesh = dtAllocNavMesh();

    if (!_navMesh) {
        fclose(fp);
        return false;
    }

    dtStatus status = _navMesh->init(&header.params);

    if (dtStatusFailed(status)) {
        fclose(fp);
        return false;
    }

    // Read tiles.
    for (U32 i = 0; i < (U32)header.numTiles; ++i) {
        NavMeshTileHeader tileHeader;
        fread(&tileHeader, sizeof(tileHeader), 1, fp);
        if (!tileHeader.tileRef || !tileHeader.dataSize) {
            return false;  // break;
        }
        U8* data = (U8*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);

        if (!data) {
            return false;  // break;
        }

        memset(data, 0, tileHeader.dataSize);
        fread(data, tileHeader.dataSize, 1, fp);

        _navMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA,
                          tileHeader.tileRef, 0);
    }
    fclose(fp);

    _extents.set(header.extents[0], header.extents[1], header.extents[2]);

    return createNavigationQuery();
}

bool NavigationMesh::save() {
    if (!_fileName.length() || !_navMesh) {
        return false;
    }
    // Save our NavigationMesh into a file to load from next time
    FILE* fp = fopen(_fileName.c_str(), "wb");
    if (!fp) {
        return false;
    }

    boost::mutex::scoped_lock(_navigationMeshLock);

    // Store header.
    NavMeshSetHeader header;
    memcpy(header.extents, &_extents[0], sizeof(F32) * 3);

    header.magic = NAVMESHSET_MAGIC;
    header.version = NAVMESHSET_VERSION;
    header.numTiles = 0;

    for (U32 i = 0; i < (U32)_navMesh->getMaxTiles(); ++i) {
        const dtMeshTile* tile = ((const dtNavMesh*)_navMesh)->getTile(i);

        if (!tile || !tile->header || !tile->dataSize) {
            continue;
        }
        header.numTiles++;
    }

    memcpy(&header.params, _navMesh->getParams(), sizeof(dtNavMeshParams));
    fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);

    // Store tiles.
    for (U32 i = 0; i < (U32)_navMesh->getMaxTiles(); ++i) {
        const dtMeshTile* tile = ((const dtNavMesh*)_navMesh)->getTile(i);

        if (!tile || !tile->header || !tile->dataSize) {
            continue;
        }

        NavMeshTileHeader tileHeader;
        tileHeader.tileRef = _navMesh->getTileRef(tile);
        tileHeader.dataSize = tile->dataSize;

        fwrite(&tileHeader, sizeof(tileHeader), 1, fp);
        fwrite(tile->data, tile->dataSize, 1, fp);
    }

    fclose(fp);

    return true;
}

bool NavigationMesh::getClosestPosition(const vec3<F32>& destination,
                                        const vec3<F32>& extents, F32 delta,
                                        vec3<F32>& result) const {
    dtPolyRef resultPoly;
    return Navigation::DivideRecast::getInstance().findNearestPointOnNavmesh(
        *this, destination, extents, delta, result, resultPoly);
}

bool NavigationMesh::getRandomPosition(vec3<F32>& result) const {
    return Navigation::DivideRecast::getInstance().getRandomNavMeshPoint(
        *this, result);
}

bool NavigationMesh::getRandomPositionInCircle(const vec3<F32>& center,
                                               F32 radius,
                                               const vec3<F32>& extents,
                                               vec3<F32>& result,
                                               U8 maxIters) const {
    return Navigation::DivideRecast::getInstance().getRandomPointAroundCircle(
        *this, center, radius, extents, result, maxIters);
}
};  // namespace Navigation
};  // namespace AI
};  // namespace Divide