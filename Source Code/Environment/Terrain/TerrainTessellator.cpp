#include "stdafx.h"

#include "Headers/TerrainTessellator.h"

#include "Headers/Terrain.h"

#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

namespace {
    // The size of a patch in meters at which point to stop subdividing a terrain patch once 
    // it's width is less than the cutoff
    constexpr F32 TERRAIN_CUTOFF_DISTANCE = 10.0f;
};

TerrainTessellator::TerrainTessellator()
   : _numNodes(0),
     _renderDepth(0),
     _maxRenderDepth(Terrain::MAX_RENDER_NODES)
{
    _tree.resize(Terrain::MAX_RENDER_NODES);
    _renderData.resize(Terrain::MAX_RENDER_NODES);

    clearTree();
}

TerrainTessellator::~TerrainTessellator()
{
    _tree.clear();
    _renderData.clear();
}

const vec3<F32>& TerrainTessellator::getEye() const {
    return _cameraEyeCache;
}

const vec3<F32>& TerrainTessellator::getOrigin() const {
    return _originiCache;
}

void TerrainTessellator::updateRenderData() {
    _renderDepth = 0;
    renderRecursive(_tree.front());
}

U16 TerrainTessellator::renderDepth() const {
    return _renderDepth;
}

const TerrainTessellator::RenderDataVector& TerrainTessellator::renderData() const {
    return _renderData;
}

void TerrainTessellator::clearTree() {
    for (TessellatedTerrainNode& node : _tree) {
        node.reset();
    }

    _numNodes = 0;
}

bool TerrainTessellator::checkDivide(const vec3<F32>& camPos, TessellatedTerrainNode &node) {
    // Distance from current origin to camera
    F32 d = camPos.xz().distanceSquared(node.origin.xz());

    // Check base case:
    // Distance to camera is greater than twice the length of the diagonal
    // from current origin to corner of current square.
    // OR
    // Max recursion level has been hit
    if (d > 2.5f * (std::pow(0.5f * node.dimensions.width, 2.0f) +
                    std::pow(0.5f * node.dimensions.height, 2.0f)) ||
        node.dimensions.width < TERRAIN_CUTOFF_DISTANCE)
    {
        return false;
    }

    return true;
}

bool TerrainTessellator::divideNode(const vec3<F32>& camPos, TessellatedTerrainNode &node) {
    // Subdivide
    F32 w_new = 0.5f * node.dimensions.width;
    F32 h_new = 0.5f * node.dimensions.height;

    // Create the child nodes
    node.c1 = createNode(node, 1u, node.origin.x - 0.5f * w_new, node.origin.y, node.origin.z - 0.5f * h_new, w_new, h_new);
    node.c2 = createNode(node, 2u, node.origin.x + 0.5f * w_new, node.origin.y, node.origin.z - 0.5f * h_new, w_new, h_new);
    node.c3 = createNode(node, 3u, node.origin.x + 0.5f * w_new, node.origin.y, node.origin.z + 0.5f * h_new, w_new, h_new);
    node.c4 = createNode(node, 4u, node.origin.x - 0.5f * w_new, node.origin.y, node.origin.z + 0.5f * h_new, w_new, h_new);

    // Assign neighbors
    switch (node.type) {
        case 1: {
            node.e = node.p->c2;
            node.n = node.p->c4;
        } break;
        case 2: {
            node.w = node.p->c1;
            node.n = node.p->c3;
        } break;
        case 3: {
            node.s = node.p->c2;
            node.w = node.p->c4;
        } break;
        case 4: {
            node.s = node.p->c1;
            node.e = node.p->c3;
        }break;
        default:
            break;
    };

    // Check if each of these four child nodes will be subdivided.
    bool div1 = checkDivide(camPos, *node.c1);
    bool div2 = checkDivide(camPos, *node.c2);
    bool div3 = checkDivide(camPos, *node.c3);
    bool div4 = checkDivide(camPos, *node.c4);

    if (div1) {
        divideNode(camPos, *node.c1);
    }
    if (div2) {
        divideNode(camPos, *node.c2);
    }
    if (div3) {
        divideNode(camPos, *node.c3);
    }
    if (div4) {
        divideNode(camPos, *node.c4);
    }

    return div1 || div2 || div3 || div4;
}

void TerrainTessellator::createTree(const vec3<F32>& camPos, const vec3<F32>& origin, const vec2<U16>& terrainDimensions) {
    clearTree();

    TessellatedTerrainNode& root = _tree[_numNodes];
    root.type = 0u; // Root node
    root.origin.set(origin);
    root.dimensions.set(terrainDimensions.width, terrainDimensions.height);
    root.tscale_negx = 1.0;
    root.tscale_negz = 1.0;
    root.tscale_posx = 1.0;
    root.tscale_posz = 1.0;
    root.p = nullptr;
    root.n = nullptr;
    root.s = nullptr;
    root.e = nullptr;
    root.w = nullptr;

    // Recursively subdivide the terrain
    divideNode(camPos, root);

    _cameraEyeCache.set(camPos);
    _originiCache.set(origin);
}

TessellatedTerrainNode* TerrainTessellator::createNode(TessellatedTerrainNode &parent, U8 type, F32 x, F32 y, F32 z, F32 width, F32 height) {
    if (_numNodes >= Terrain::MAX_RENDER_NODES) {
        return nullptr;
    }

    _numNodes++;

    TessellatedTerrainNode& node = _tree[_numNodes];
    node.type = type;
    node.origin.set(x, y, z);
    node.dimensions.set(width, height);
    node.tscale_negx = 1.0;
    node.tscale_negz = 1.0;
    node.tscale_posx = 1.0;
    node.tscale_posz = 1.0;
    node.p = &parent;
    node.n = nullptr;
    node.s = nullptr;
    node.e = nullptr;
    node.w = nullptr;
    return &node;
}

void TerrainTessellator::renderNode(TessellatedTerrainNode& node) {
    // Calculate the tess scale factor
    calcTessScale(node);

    _renderData[_renderDepth].set(node.origin,
                                  node.dimensions.width * 0.5f,
                                  node.tscale_negx,
                                  node.tscale_negz,
                                  node.tscale_posx,
                                  node.tscale_posz);
}

void TerrainTessellator::renderRecursive(TessellatedTerrainNode& node) {
    if (_renderDepth >= _maxRenderDepth) {
        return;
    }

    // If all children are null, render this node
    if (!node.c1 && !node.c2 && !node.c3 && !node.c4)
    {
        renderNode(node);
        _renderDepth++;
        return;
    }

    // Otherwise, recurse to the children.
    if (node.c1) {
        renderRecursive(*node.c1);
        renderRecursive(*node.c2);
        renderRecursive(*node.c3);
        renderRecursive(*node.c4);
    }
}

void TerrainTessellator::calcTessScale(TessellatedTerrainNode& node) {
    TessellatedTerrainNode* t;

    TessellatedTerrainNode& root = _tree[0];

    // Positive Z (north)
    t = find(root, node.origin.x, node.origin.z + 1 + node.dimensions.width * 0.5f);
    if (t->dimensions.width > node.dimensions.width) {
        node.tscale_posz = 2.0;
    }

    // Positive X (east)
    t = find(root, node.origin.x + 1 + node.dimensions.width * 0.5f, node.origin.z);
    if (t->dimensions.width > node.dimensions.width) {
        node.tscale_posx = 2.0;
    }

    // Negative Z (south)
    t = find(root, node.origin.x, node.origin.z - 1 - node.dimensions.width * 0.5f);
    if (t->dimensions.width > node.dimensions.width) {
        node.tscale_negz = 2.0;
    }

    // Negative X (west)
    t = find(root, node.origin.x - 1 - node.dimensions.width * 0.5f, node.origin.z);
    if (t->dimensions.width > node.dimensions.width) {
        node.tscale_negx = 2.0;
    }
}

TessellatedTerrainNode* TerrainTessellator::find(TessellatedTerrainNode& n, F32 x, F32 z) {
    if (COMPARE(n.origin.x, x) && COMPARE(n.origin.z, z)) {
        return &n;
    }

    if (!n.c1 && !n.c2 && !n.c3 && !n.c4) {
        return &n;
    }

    if (IS_GEQUAL(n.origin.x, x) && IS_GEQUAL(n.origin.z, z) && n.c1) {
        return find(*n.c1, x, z);
    } else if (IS_LEQUAL(n.origin.x, x) && IS_GEQUAL(n.origin.z, z) && n.c2) {
        return find(*n.c2, x, z);
    } else if (IS_LEQUAL(n.origin.x, x) && IS_LEQUAL(n.origin.z, z) && n.c3) {
        return find(*n.c3, x, z);
    } else if (IS_GEQUAL(n.origin.x, x) && IS_LEQUAL(n.origin.z, z) && n.c4) {
        return find(*n.c4, x, z);
    }

    return &n;
}

void TerrainTessellator::initTessellationPatch(VertexBuffer* vb) {
    vb->resizeVertexCount(4);
    vb->modifyPositionValue(0, -1.0f, 0.0f, -1.0f);
    vb->modifyPositionValue(1, 1.0f, 0.0f, -1.0f);
    vb->modifyPositionValue(2, 1.0f, 0.0f, 1.0f);
    vb->modifyPositionValue(3, -1.0f, 0.0f, 1.0f);

    vb->modifyColourValue(0, Util::ToByteColour(vec3<F32>(0.18f, 0.91f, 0.46f)));
    vb->modifyColourValue(1, Util::ToByteColour(vec3<F32>(0.18f, 0.91f, 0.46f)));
    vb->modifyColourValue(2, Util::ToByteColour(vec3<F32>(0.18f, 0.91f, 0.46f)));
    vb->modifyColourValue(3, Util::ToByteColour(vec3<F32>(0.18f, 0.91f, 0.46f)));

    vb->modifyNormalValue(0, 0.0f, 1.0f, 0.0f);
    vb->modifyNormalValue(1, 0.0f, 1.0f, 0.0f);
    vb->modifyNormalValue(2, 0.0f, 1.0f, 0.0f);
    vb->modifyNormalValue(3, 0.0f, 1.0f, 0.0f);

    vb->modifyTexCoordValue(0, 0.0f, 2.0f);
    vb->modifyTexCoordValue(1, 2.0f, 2.0f);
    vb->modifyTexCoordValue(2, 2.0f, 0.0f);
    vb->modifyTexCoordValue(3, 0.0f, 0.0f);

    vb->addIndex(0);
    vb->addIndex(1);
    vb->addIndex(2);
    vb->addIndex(3);
}

}; //namespace Divide
