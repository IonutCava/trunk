#include "stdafx.h"

#include "Headers/NavMesh.h"
#include "Headers/NavMeshDebugDraw.h"
#include "../Headers/DivideRecast.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Managers/Headers/SceneManager.h"

#include "Platform/File/Headers/FileManagement.h"

#include <ReCast/DebugUtils/Include/RecastDump.h>
#include <ReCast/DebugUtils/Include/DetourDebugDraw.h>
#include <ReCast/DebugUtils/Include/RecastDebugDraw.h>

#include <SimpleINI/include/SimpleIni.h>

namespace Divide {
namespace AI {
namespace Navigation {

NavigationMesh::NavigationMesh(PlatformContext& context)
    : GUIDWrapper(),
      PlatformContextComponent(context),
      _buildJobGUID(-1)
{
    ParamHandler& par = ParamHandler::instance();
    stringImpl path(Paths::g_xmlDataLocation + Paths::g_scenesLocation);
    path.append(par.getParam<stringImpl>(_ID("currentScene")));

    _debugDrawInterface = MemoryManager_NEW NavMeshDebugDraw(context.gfx());
    _filePath = path + "/" + Paths::g_navMeshesLocation;
    
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
    _sgn = nullptr;
}

NavigationMesh::~NavigationMesh()
{
    unload();
    MemoryManager::DELETE(_debugDrawInterface);
}

bool NavigationMesh::unload() {
    stopThreadedBuild();

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

void NavigationMesh::stopThreadedBuild() {
    if (_buildJobGUID != -1){
        assert(_buildTask._task);
        Stop(*_buildTask._task);
        Wait(*_buildTask._task);
    }
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
inline I32 charToInt(const char* val) {
    return Util::ConvertData<I32, const char*>(val);
}

inline F32 charToFloat(const char* val) {
    return Util::ConvertData<F32, const char*>(val);
}

inline bool charToBool(const char* val) {
    return _stricmp(val, "true") == 0;
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
        charToInt(ini.GetValue("Rasterization", "iTileSize", "48")));
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
        charToInt(ini.GetValue("Region", "fMergeSize", "20")));
    _configParams.setRegionMinSize(
        charToInt(ini.GetValue("Region", "fMinSize", "50")));
    // Load all key-value pairs for the "Polygonization" section
    _configParams.setEdgeMaxLen(
        charToInt(ini.GetValue("Polygonization", "fEdgeMaxLength", "12")));
    _configParams.setEdgeMaxError(
        charToFloat(ini.GetValue("Polygonization", "fEdgeMaxError", "1.3")));
    _configParams.setVertsPerPoly(
        charToInt(ini.GetValue("Polygonization", "iVertsPerPoly", "6")));
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
        Console::errorfn(Locale::get(_ID("NAV_MESH_CONFIG_NOT_FOUND")));
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
    stopThreadedBuild();

    _buildTask = CreateTask(_context,
                            [this](const Task& parentTask) {
                                buildInternal(parentTask);
                            }).startTask();

    return true;
}

void NavigationMesh::buildInternal(const Task& parentTask) {
    _building = true;
    // Create mesh
    Time::ProfileTimer importTimer;
    importTimer.start();
    bool state = generateMesh();
    importTimer.stop();
    if (state) {
        Console::printfn(Locale::get(_ID("NAV_MESH_GENERATION_COMPLETE")),
                         Time::MicrosecondsToSeconds<F32>(importTimer.get()));

        {
            std::lock_guard<std::mutex> lock (_navigationMeshLock);
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

        // Free structs used during build
        freeIntermediates(false);

        if (_loadCompleteClbk) {
            _loadCompleteClbk(this);
        }

        _building = false;
    } else {
        Console::errorfn(Locale::get(_ID("NAV_MESH_GENERATION_INCOMPLETE")),
                         Time::MicrosecondsToSeconds<F32>(importTimer.get()));
        return;
    }
}

bool NavigationMesh::buildProcess() {
    _building = true;
    // Create mesh
    Time::ProfileTimer importTimer;
    importTimer.start();
    bool success = generateMesh();
    importTimer.stop();
    if (!success) {
        Console::errorfn(Locale::get(_ID("NAV_MESH_GENERATION_INCOMPLETE")),
                         Time::MicrosecondsToSeconds<F32>(importTimer.get()));
        return false;
    }

    Console::printfn(Locale::get(_ID("NAV_MESH_GENERATION_COMPLETE")),
                     Time::MicrosecondsToSeconds<F32>(importTimer.get()));

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

    stringImpl nodeName(generateMeshName(*_sgn));

    // Parse objects from level into RC-compatible format
    _fileName.append(nodeName);
    _fileName.append(".nm");
    Console::printfn(Locale::get(_ID("NAV_MESH_GENERATION_START")),
                     nodeName.c_str());

    NavModelData data;
    stringImpl geometrySaveFile(_fileName);
    Util::ReplaceStringInPlace(geometrySaveFile, ".nm", ".ig");

    data.clear(false);
    data.name(nodeName);

    if (!NavigationMeshLoader::loadMeshFile(data, _filePath.c_str(), geometrySaveFile.c_str())) {
        if (!NavigationMeshLoader::parse(_sgn->get<BoundsComponent>()->getBoundingBox(), data, *_sgn)) {
            Console::errorfn(Locale::get(_ID("ERROR_NAV_PARSE_FAILED")),
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
    Console::printfn(Locale::get(_ID("NAV_MESH_BOUNDS")), cfg.bmax[0], cfg.bmax[1],
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
    params.walkableHeight = to_F32(cfg.walkableHeight);
    params.walkableRadius = to_F32(cfg.walkableRadius);
    params.walkableClimb  = to_F32(cfg.walkableClimb);

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

    
    if (_navMesh) {
        dtFreeNavMesh(_navMesh);
    }

    load(*_sgn);
    if (_navMesh == nullptr) {
        createNavigationMesh(params);
    }

    if (_navMesh == nullptr) {
        data.isValid(false);
        return false;
    }

    data.isValid(true);
    save(*_sgn);

    return NavigationMeshLoader::saveMeshFile(data, _filePath.c_str(), geometrySaveFile.c_str());  // input geometry;
}

bool NavigationMesh::createNavigationQuery(U32 maxNodes) {
    _navQuery = dtAllocNavMeshQuery();
    return (_navQuery->init(_navMesh, maxNodes) == DT_SUCCESS);
}

bool NavigationMesh::createPolyMesh(rcConfig& cfg, NavModelData& data, rcContextDivide* ctx) {
    if (_fileName.empty()) {
        _fileName = data.name();
    }

    // Create a heightfield to voxelise our input geometry
    _heightField = rcAllocHeightfield();

    if (!_heightField) {
        Console::errorfn(Locale::get(_ID("ERROR_NAV_OUT_OF_MEMORY")), "rcAllocHeightfield", _fileName.c_str());
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
        Console::errorfn(Locale::get(_ID("ERROR_NAV_HEIGHTFIELD")),  _fileName.c_str());
        return false;
    }

    U8* areas = MemoryManager_NEW U8[data.getTriCount()];

    if (!areas) {
        Console::errorfn(Locale::get(_ID("ERROR_NAV_OUT_OF_MEMORY")), "areaFlag allocation", _fileName.c_str());
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
        Console::errorfn(Locale::get(_ID("ERROR_NAV_COMPACT_HEIGHTFIELD")), _fileName.c_str());
        return false;
    }

    if (!rcErodeWalkableArea(ctx, cfg.walkableRadius, *_compactHeightField)) {
        Console::errorfn(Locale::get(_ID("ERROR_NAV_WALKABLE")), _fileName.c_str());
        return false;
    }

    if (false) {
        if (!rcBuildRegionsMonotone(ctx, *_compactHeightField, cfg.borderSize,
                                    cfg.minRegionArea, cfg.mergeRegionArea)) {
            Console::errorfn(Locale::get(_ID("ERROR_NAV_REGIONS")), _fileName.c_str());
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
        Console::errorfn(Locale::get(_ID("ERROR_NAV_COUNTOUR")), _fileName.c_str());
        return false;
    }

    _polyMesh = rcAllocPolyMesh();
    if (!_polyMesh ||
        !rcBuildPolyMesh(ctx, *_countourSet, cfg.maxVertsPerPoly, *_polyMesh)) {
        Console::errorfn(Locale::get(_ID("ERROR_NAV_POLY_MESH")), _fileName.c_str());
        return false;
    }

    _polyMeshDetail = rcAllocPolyMeshDetail();
    if (!_polyMeshDetail ||
        !rcBuildPolyMeshDetail(ctx, *_polyMesh, *_compactHeightField,
                               cfg.detailSampleDist, cfg.detailSampleMaxError,
                               *_polyMeshDetail)) {
        Console::errorfn(Locale::get(_ID("ERROR_NAV_POLY_MESH_DETAIL")), _fileName.c_str());
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
        to_F32(ctx->getAccumulatedTime(RC_TIMER_TOTAL) / 1000.0f));

    return true;
}

bool NavigationMesh::createNavigationMesh(dtNavMeshCreateParams& params) {
    
    U8* tileData = nullptr;
    I32 tileDataSize = 0;
    if (!dtCreateNavMeshData(&params, &tileData, &tileDataSize)) {
        Console::errorfn(Locale::get(_ID("ERROR_NAV_MESH_DATA")), _fileName.c_str());
        return false;
    }

    _tempNavMesh = dtAllocNavMesh();
    if (!_tempNavMesh) {
        Console::errorfn(Locale::get(_ID("ERROR_NAV_DT_OUT_OF_MEMORY")), _fileName.c_str());
        return false;
    }

    dtStatus s = _tempNavMesh->init(tileData, tileDataSize, DT_TILE_FREE_DATA);
    if (dtStatusFailed(s)) {
        Console::errorfn(Locale::get(_ID("ERROR_NAV_DT_INIT")), _fileName.c_str());
        return false;
    }

    // Initialise all flags to something helpful.
    for (U32 i = 0; i < to_U32(_tempNavMesh->getMaxTiles()); ++i) {
        const dtMeshTile* tile = ((const dtNavMesh*)_tempNavMesh)->getTile(i);

        if (!tile->header) {
            continue;
        }

        const dtPolyRef base = _tempNavMesh->getPolyRefBase(tile);

        for (U32 j = 0; j < to_U32(tile->header->polyCount); ++j) {
            const dtPolyRef ref = base | j;
            U16 f = 0;
            _tempNavMesh->getPolyFlags(ref, &f);
            _tempNavMesh->setPolyFlags(ref, f | 1);
        }
    }

    return true;
}

void NavigationMesh::update(const U64 deltaTimeUS) {
    _debugDrawInterface->paused(!_debugDraw);
}

GFX::CommandBuffer& NavigationMesh::draw() {
    RenderMode mode = _renderMode;

    if (_building) {
        mode = RenderMode::RENDER_NAVMESH;
        _debugDrawInterface->overrideColour(duRGBA(255, 0, 0, 80));
    }

    _debugDrawInterface->beginBatch();

    _navigationMeshLock.lock();

    switch (mode) {
        case RenderMode::RENDER_NAVMESH:
            if (_navMesh) {
                duDebugDrawNavMesh(_debugDrawInterface, *_navMesh, 0);
            }
            break;
        case RenderMode::RENDER_CONTOURS:
            if (_countourSet) {
                duDebugDrawContours(_debugDrawInterface, *_countourSet);
            }
            break;
        case RenderMode::RENDER_POLYMESH:
            if (_polyMesh) {
                duDebugDrawPolyMesh(_debugDrawInterface, *_polyMesh);
            }
            break;
        case RenderMode::RENDER_DETAILMESH:
            if (_polyMeshDetail) {
                duDebugDrawPolyMeshDetail(_debugDrawInterface, *_polyMeshDetail);
            }
            break;
        case RenderMode::RENDER_PORTALS:
            if (_navMesh) {
                duDebugDrawNavMeshPortals(_debugDrawInterface, *_navMesh);
            }
            break;
    }

    if (!_building) {
        if (_countourSet && _renderConnections) {
            duDebugDrawRegionConnections(_debugDrawInterface, *_countourSet);
        }
    }

    _navigationMeshLock.unlock();

    _debugDrawInterface->endBatch();

    return _debugDrawInterface->toCommandBuffer();
}


bool NavigationMesh::load(SceneGraphNode& sgn) {
    if (!_fileName.length()) {
        return false;
    }

    dtNavMesh* temp = nullptr;
    stringImpl file = _fileName;

    stringImpl nodeName(generateMeshName(sgn));
    
    // Parse objects from level into RC-compatible format
    file.append(nodeName);
    file.append(".nm");

    // Parse objects from level into RC-compatible format
    FILE* fp = fopen((_filePath + file).c_str(), "rb");
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

    std::lock_guard<std::mutex> lock(_navigationMeshLock);
    temp = dtAllocNavMesh();

    if (!temp) {
        fclose(fp);
        return false;
    }

    dtStatus status = temp->init(&header.params);

    if (dtStatusFailed(status)) {
        fclose(fp);
        return false;
    }

    // Read tiles.
    for (U32 i = 0; i < to_U32(header.numTiles); ++i) {
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

        temp->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA,
                     tileHeader.tileRef, 0);
    }
    fclose(fp);

    _extents.set(header.extents[0], header.extents[1], header.extents[2]);
    _navMesh = temp;
    return createNavigationQuery();
}

bool NavigationMesh::save(SceneGraphNode& sgn) {
    if (!_fileName.length() || !_navMesh) {
        return false;
    }

    stringImpl file = _fileName;
    stringImpl nodeName(generateMeshName(sgn));
    // Parse objects from level into RC-compatible format
    file.append(nodeName);
    file.append(".nm");


    // Save our NavigationMesh into a file to load from next time
    FILE* fp = fopen((_filePath + file).c_str(), "wb");
    if (!fp) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_navigationMeshLock);

    // Store header.
    NavMeshSetHeader header;
    memcpy(header.extents, &_extents[0], sizeof(F32) * 3);

    header.magic = NAVMESHSET_MAGIC;
    header.version = NAVMESHSET_VERSION;
    header.numTiles = 0;

    for (U32 i = 0; i < to_U32(_navMesh->getMaxTiles()); ++i) {
        const dtMeshTile* tile = ((const dtNavMesh*)_navMesh)->getTile(i);

        if (!tile || !tile->header || !tile->dataSize) {
            continue;
        }
        header.numTiles++;
    }

    memcpy(&header.params, _navMesh->getParams(), sizeof(dtNavMeshParams));
    fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);

    // Store tiles.
    for (U32 i = 0; i < to_U32(_navMesh->getMaxTiles()); ++i) {
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

stringImpl NavigationMesh::generateMeshName(SceneGraphNode& sgn) {
    return (sgn.getNode()->type() != SceneNodeType::TYPE_ROOT)
               ? "_node_[_" + sgn.name() + "_]"
               : "_root_node";
}

bool NavigationMesh::getClosestPosition(const vec3<F32>& destination,
                                        const vec3<F32>& extents,
                                        F32 delta,
                                        vec3<F32>& result) const {
    dtPolyRef resultPoly;
    return Navigation::DivideRecast::instance().findNearestPointOnNavmesh(
        *this, destination, extents, delta, result, resultPoly);
}

bool NavigationMesh::getRandomPosition(vec3<F32>& result) const {
    return Navigation::DivideRecast::instance().getRandomNavMeshPoint(
        *this, result);
}

bool NavigationMesh::getRandomPositionInCircle(const vec3<F32>& center,
                                               F32 radius,
                                               const vec3<F32>& extents,
                                               vec3<F32>& result,
                                               U8 maxIters) const {
    return Navigation::DivideRecast::instance().getRandomPointAroundCircle(
        *this, center, radius, extents, result, maxIters);
}
};  // namespace Navigation
};  // namespace AI
};  // namespace Divide