#include "stdafx.h"

#include "Headers/TerrainTessellator.h"

#include "Headers/Terrain.h"

#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

namespace {
    constexpr U32 MAX_RENDER_NODES = 256;

    FORCE_INLINE bool hasChildren(TessellatedTerrainNode& node) {
        for (U8 i = 0; i < 4; ++i) {
            if (node.c[i] != nullptr) {
                return true;
            }
        }
        return false;
    }
};

TerrainTessellator::TerrainTessellator()
   : _numNodes(0),
     _renderDepth(0),
     _prevRenderDepth(0),
     _lastFrustumPlaneCache(-1)
{
    _tree.reserve(MAX_TESS_NODES);
    _renderData.resize(MAX_RENDER_NODES);
}

TerrainTessellator::~TerrainTessellator()
{
    _renderData.clear();
}

const vec3<F32>& TerrainTessellator::getOrigin() const {
    return _originCache;
}

const Frustum& TerrainTessellator::getFrustum() const {
    return _frustumCache;
}

U16 TerrainTessellator::getRenderDepth() const {
    return _renderDepth;
}

U16 TerrainTessellator::getPrevRenderDepth() const {
    return _prevRenderDepth;
}

bufferPtr TerrainTessellator::updateAndGetRenderData(U16& renderDepth, U8 LoD) {
    renderDepth = 0;
    renderRecursive(_tree.data(), renderDepth, LoD);
    _prevRenderDepth = _renderDepth;
    _renderDepth = renderDepth;
    return _renderData.data();
}

const TerrainTessellator::RenderDataVector& TerrainTessellator::renderData() const {
    return _renderData;
}

bool TerrainTessellator::inDivideCheck(TessellatedTerrainNode* node) const {
    if (_config._useCameraDistance) {
        // Distance from current origin to camera
        //F32 d = std::abs(_cameraEyeCache.xz().distanceSquared(node->origin.xz()));
        F32 d = std::abs(Sqrt(SQUARED(_cameraEyeCache[0] - node->origin[0]) + SQUARED(_cameraEyeCache[2] - node->origin[2])));
        if (d > 2.5 * Sqrt(SQUARED(0.5f * node->dim.width) + SQUARED(0.5f * node->dim.height)) || node->dim.width < _config._cutoffDistance) {
            return false;
        }
        //return d <= 2.5f * (SQUARED(0.5f * node->dim.width) + SQUARED(0.5f * node->dim.height));
    }

    return true;
}

bool TerrainTessellator::checkDivide(TessellatedTerrainNode* node) {
    if (_numNodes >= MAX_TESS_NODES - 4) {
        return false;
    }
    // Check base case:
    // Distance to camera is greater than twice the length of the diagonal
    // from current origin to corner of current square.
    // OR
    // Max recursion level has been hit
    return inDivideCheck(node);
}

bool TerrainTessellator::divideNode(TessellatedTerrainNode* node) {
    // Subdivide
    F32 w_new = node->dim.width * 0.5f;
    F32 h_new = node->dim.height * 0.5f;

    // Create the child nodes
    node->c[0] = createNode(node, TessellatedTerrainNodeType::CHILD_1, node->origin.x - 0.5f * w_new, node->origin.y, node->origin.z - 0.5f * h_new, w_new, h_new);
    node->c[1] = createNode(node, TessellatedTerrainNodeType::CHILD_2, node->origin.x + 0.5f * w_new, node->origin.y, node->origin.z - 0.5f * h_new, w_new, h_new);
    node->c[2] = createNode(node, TessellatedTerrainNodeType::CHILD_3, node->origin.x + 0.5f * w_new, node->origin.y, node->origin.z + 0.5f * h_new, w_new, h_new);
    node->c[3] = createNode(node, TessellatedTerrainNodeType::CHILD_4, node->origin.x - 0.5f * w_new, node->origin.y, node->origin.z + 0.5f * h_new, w_new, h_new);

    // Assign neighbors
    switch (node->type) {
        case TessellatedTerrainNodeType::CHILD_1: {
            node->e = node->p->c[1];
            node->n = node->p->c[3];
        } break;
        case TessellatedTerrainNodeType::CHILD_2: {
            node->w = node->p->c[0];
            node->n = node->p->c[2];
        } break;
        case TessellatedTerrainNodeType::CHILD_3: {
            node->s = node->p->c[1];
            node->w = node->p->c[3];
        } break;
        case TessellatedTerrainNodeType::CHILD_4: {
            node->s = node->p->c[0];
            node->e = node->p->c[2];
        }break;
        default:
            break;
    };

    // Check if each of these four child nodes will be subdivided.
    bool divided = false;
    if (node->c[0] != nullptr) {
        for (U8 i = 0; i < 4; ++i) {
            if (checkDivide(node->c[i])) {
                divideNode(node->c[i]);
                divided = true;
            }
        }
    }

    return divided;
}

void TerrainTessellator::createTree(const vec3<F32>& camPos, const Frustum& frust, const vec3<F32>& origin, const vec2<U16>& terrainDimensions, const F32 patchSizeInMetres) {
    _cameraEyeCache.set(camPos);
    _originCache.set(origin);
    _frustumCache.set(frust);
    _config._cutoffDistance = patchSizeInMetres;

    _numNodes = 0;
    _tree.resize(std::max(terrainDimensions.width, terrainDimensions.height));

    TessellatedTerrainNode& terrainTree = _tree[_numNodes];
    terrainTree.type = TessellatedTerrainNodeType::ROOT; // Root node
    terrainTree.origin = origin;
    terrainTree.dim.set(terrainDimensions.width, terrainDimensions.height);
    terrainTree.tscale.set(1.0f);
    std::memset(terrainTree.c, 0, sizeof(terrainTree.c));

    terrainTree.p = nullptr;
    terrainTree.n = nullptr;
    terrainTree.s = nullptr;
    terrainTree.e = nullptr;
    terrainTree.w = nullptr;
    
    // Recursively subdivide the terrain
    divideNode(&terrainTree);
}

TessellatedTerrainNode* TerrainTessellator::createNode(TessellatedTerrainNode* parent, TessellatedTerrainNodeType type, F32 x, F32 y, F32 z, F32 width, F32 height) {
    _numNodes++;

    TessellatedTerrainNode& terrainTreeTail = _tree[_numNodes];

    terrainTreeTail.type = type;
    terrainTreeTail.origin.set(x, y, z);
    terrainTreeTail.dim.set(width, height);
    terrainTreeTail.tscale.set(1.0f);
    std::memset(terrainTreeTail.c, 0, sizeof(terrainTreeTail.c));

    terrainTreeTail.p = parent;
    terrainTreeTail.n = nullptr;
    terrainTreeTail.s = nullptr;
    terrainTreeTail.e = nullptr;
    terrainTreeTail.w = nullptr;

    return &terrainTreeTail;
}

void TerrainTessellator::renderRecursive(TessellatedTerrainNode* node, U16& renderDepth, U8 LoD) {
    assert(node != nullptr);

    // If all children are null, render this node
    if (!hasChildren(*node)) {
        // We used a N x sized buffer, so we should allow for some margin in frustum culling
        const F32 radiusAdjustmentFactor = 1.25f;

        assert(renderDepth < MAX_RENDER_NODES);

        // half of the diagonal of the rectangle
        F32 radius = Sqrt(SQUARED(node->dim.width * 0.5f) + SQUARED(node->dim.height * 0.5f));
        radius *= radiusAdjustmentFactor;

        bool inFrustum = true;
        if (_config._useFrustumCulling) {
            inFrustum = _frustumCache.ContainsSphere(node->origin, radius, _lastFrustumPlaneCache) != Frustum::FrustCollision::FRUSTUM_OUT;
        }

        calcTessScale(node);

        if (inFrustum) {
            TessellatedNodeData& data = _renderData[renderDepth++];
            data._positionAndTileScale.set(node->origin, node->dim.width * 0.5f);
            data._tScale.set(node->tscale);
        }
    } else {
        // Otherwise, recurse to the children.
        for (U8 i = 0; i < 4; ++i) {
            renderRecursive(node->c[i], renderDepth, LoD);
        }
    }
}
void TerrainTessellator::calcTessScale(TessellatedTerrainNode* node) {
    TessellatedTerrainNode* t = nullptr;
    TessellatedTerrainNode* const data = _tree.data();

    bool found = false;
    // Positive Y (north)
    t = find(data, node->origin.x, node->origin.z + 1 + node->dim.width * 0.5f, found);
    node->tscale[3] = found ? (t->dim.width / node->dim.width) : 1.0f;
    assert(!found || IS_IN_RANGE_INCLUSIVE(node->tscale[3], 0.5f, 2.0f));

    // Positive X (east)
    t = find(data, node->origin.x + 1 + node->dim.width * 0.5f, node->origin.z, found);
    node->tscale[0] = found ? (t->dim.width / node->dim.width) : 1.0f;
    assert(!found || IS_IN_RANGE_INCLUSIVE(node->tscale[0], 0.5f, 2.0f));

    // Negative Y (south)
    t = find(data, node->origin.x, node->origin.z - 1 - node->dim.width * 0.5f, found);
    //if (t->dim.width > node->dim.width)
    node->tscale[1] = found ? (t->dim.width / node->dim.width) : 1.0f;
    assert(!found || IS_IN_RANGE_INCLUSIVE(node->tscale[1], 0.5f, 2.0f));

    // Negative X (west)
    t = find(data, node->origin.x - 1 - node->dim.width * 0.5f, node->origin.z, found);
    node->tscale[2] = found ? (t->dim.width / node->dim.width) : 1.0f;
    assert(!found || IS_IN_RANGE_INCLUSIVE(node->tscale[2], 0.5f, 2.0f));
}

TessellatedTerrainNode* TerrainTessellator::find(TessellatedTerrainNode* const n, F32 x, F32 z, bool& found) {
    if (COMPARE(n->origin.x, x) && COMPARE(n->origin.z, z)) {
        found = true;
        return n;
    }

    if (n->c[0] == nullptr && n->c[1] == nullptr && n->c[2] == nullptr && n->c[3] == nullptr) {
        found = true;
        return n;
    }

    if        ((n->origin.x >= x) && (n->origin.z >= z) && n->c[0] != nullptr) {
        return find(n->c[0], x, z, found);
    } else if ((n->origin.x <= x) && (n->origin.z >= z) && n->c[1] != nullptr) {
        return find(n->c[1], x, z, found);
    } else if ((n->origin.x <= x) && (n->origin.z <= z) && n->c[2] != nullptr) {
        return find(n->c[2], x, z, found);
    } else if ((n->origin.x >= x) && (n->origin.z <= z) && n->c[3] != nullptr) {
        return find(n->c[3], x, z, found);
    }

    found = false;
    return n;
}

}; //namespace Divide
