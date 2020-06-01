#include "stdafx.h"

#include "Headers/TerrainTessellator.h"

#include "Headers/Terrain.h"

#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

namespace {
    FORCE_INLINE bool hasChildren(TessellatedTerrainNode& node) {
        for (U8 i = 0; i < 4; ++i) {
            if (node.c[i] != nullptr) {
                return true;
            }
        }
        return false;
    }
};

const vec3<F32>& TerrainTessellator::getOrigin() const noexcept {
    return _originCache;
}

const vec3<F32>& TerrainTessellator::getEyeCache() const noexcept {
    return _cameraEyeCache;
}

const Frustum& TerrainTessellator::getFrustum() const noexcept {
    return _frustumCache;
}

U16 TerrainTessellator::getRenderDepth() const noexcept {
    return _renderDepth;
}


const TerrainTessellator::RenderData& TerrainTessellator::updateAndGetRenderData(const Frustum& frust, const vec3<F32>& eye, U16& renderDepth) {
    _frustumCache.set(frust);
    _cameraEyeCache.set(eye);

    _renderDepth = 0;
    renderRecursive(_tree.data(), eye);
    renderDepth = _renderDepth;
    return _renderData;
}

const TerrainTessellator::RenderData& TerrainTessellator::renderData() const noexcept {
    return _renderData;
}

bool TerrainTessellator::checkDivide(TessellatedTerrainNode* node, const vec3<F32>& eye) {
    // Check base case:
    // Distance to camera is greater than twice the length of the diagonal
    // from current origin to corner of current square.
    // OR
    // Max recursion level has been hit
    // Distance from current origin to camera
    const F32 d = std::abs(Sqrt(SQUARED(eye.x - node->origin.x) + SQUARED(eye.z - node->origin.z)));
    if (d > 2.5f * Sqrt(SQUARED(0.5f * node->dim.width) + SQUARED(0.5f * node->dim.height))) {
        return false;
    }

    return node->dim.width >= _config._minPatchSize;
}

bool TerrainTessellator::divideNode(TessellatedTerrainNode* node, const vec3<F32>& eye) {
    // Subdivide
    const F32 w_new = node->dim.width * 0.5f;
    const F32 h_new = node->dim.height * 0.5f;

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
            if (checkDivide(node->c[i], eye)) {
                divideNode(node->c[i], eye);
                divided = true;
            }
        }
    }

    return divided;
}

const TerrainTessellator::RenderData& TerrainTessellator::createTree(const Frustum& frust, const vec3<F32>& camPos, const vec3<F32>& origin, const F32 maxDistance, U16& renderDepth) {
    _originCache.set(origin);
    _maxDistanceSQ = SQUARED(maxDistance);

    _numNodes = 0;
    _tree.resize(std::max(_config._terrainDimensions.width, _config._terrainDimensions.height) + 4);

    TessellatedTerrainNode& terrainTree = _tree[_numNodes];
    terrainTree.type = TessellatedTerrainNodeType::ROOT; // Root node
    terrainTree.origin = origin;
    terrainTree.dim.set(_config._terrainDimensions.width, _config._terrainDimensions.height);
    std::memset(terrainTree.c, 0, sizeof(terrainTree.c));

    terrainTree.p = nullptr;
    terrainTree.n = nullptr;
    terrainTree.s = nullptr;
    terrainTree.e = nullptr;
    terrainTree.w = nullptr;
    
    // Recursively subdivide the terrain
    divideNode(&terrainTree, camPos);

    return updateAndGetRenderData(frust, camPos, renderDepth);
}

TessellatedTerrainNode* TerrainTessellator::createNode(TessellatedTerrainNode* parent, TessellatedTerrainNodeType type, F32 x, F32 y, F32 z, F32 width, F32 height) {
    _numNodes++;

    TessellatedTerrainNode& terrainTreeTail = _tree[_numNodes];

    terrainTreeTail.type = type;
    terrainTreeTail.origin.set(x, y, z);
    terrainTreeTail.dim.set(width, height);
    std::memset(terrainTreeTail.c, 0, sizeof(terrainTreeTail.c));

    terrainTreeTail.p = parent;
    terrainTreeTail.n = nullptr;
    terrainTreeTail.s = nullptr;
    terrainTreeTail.e = nullptr;
    terrainTreeTail.w = nullptr;

    return &terrainTreeTail;
}

void TerrainTessellator::renderRecursive(TessellatedTerrainNode* node, const vec3<F32>& eye) {
    assert(node != nullptr);

    // If all children are null, render this node
    if (!hasChildren(*node)) {
        if (eye.xz().distanceSquared(node->origin.xz()) > _maxDistanceSQ) {
            return;
        }

        const vec3<F32>& origin = node->origin;
        const vec2<F32>& altRange = _config._altitudeRange;
        const F32 halfWidth = node->dim.width * 0.5f;
        const F32 halfHeight = node->dim.height * 0.5f;
        const BoundingBox bb 
        {
            origin - vec3<F32>(halfWidth, -altRange.min, halfHeight),
            origin + vec3<F32>(halfWidth,  altRange.max, halfHeight)
        };

        if (_frustumCache.ContainsBoundingBox(bb, _lastPlaneIndex) != Frustum::FrustCollision::FRUSTUM_OUT)
        {
            TessellatedNodeData& data = _renderData[_renderDepth++];
            data._positionAndTileScale.set(node->origin, node->dim.width * 0.5f);
            data._tScale.set(calcTessScale(node));
        }
    } else {
        // Otherwise, recurse to the children.
        for (U8 i = 0; i < 4; ++i) {
            renderRecursive(node->c[i], eye);
        }
    }
}

vec4<F32> TerrainTessellator::calcTessScale(TessellatedTerrainNode* node) {
    vec4<F32> ret = VECTOR4_UNIT;

    TessellatedTerrainNode* t = nullptr;
    TessellatedTerrainNode* const data = _tree.data();

    bool found = false;
    // Positive Y (north)
    t = find(data, node->origin.x, node->origin.z + 1 + node->dim.width * 0.5f, found);
    ret[3] = found ? (t->dim.width / node->dim.width) : 1.0f;
    assert(!found || IS_IN_RANGE_INCLUSIVE(ret[3], 0.5f, 2.0f));

    // Positive X (east)
    t = find(data, node->origin.x + 1 + node->dim.width * 0.5f, node->origin.z, found);
    ret[0] = found ? (t->dim.width / node->dim.width) : 1.0f;
    assert(!found || IS_IN_RANGE_INCLUSIVE(ret[0], 0.5f, 2.0f));

    // Negative Y (south)
    t = find(data, node->origin.x, node->origin.z - 1 - node->dim.width * 0.5f, found);
    //if (t->dim.width > node->dim.width)
    ret[1] = found ? (t->dim.width / node->dim.width) : 1.0f;
    assert(!found || IS_IN_RANGE_INCLUSIVE(ret[1], 0.5f, 2.0f));

    // Negative X (west)
    t = find(data, node->origin.x - 1 - node->dim.width * 0.5f, node->origin.z, found);
    ret[2] = found ? (t->dim.width / node->dim.width) : 1.0f;
    assert(!found || IS_IN_RANGE_INCLUSIVE(ret[2], 0.5f, 2.0f));

    return ret;
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
