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

U16 TerrainTessellator::getRenderDepth() const {
    return _renderDepth;
}

U16 TerrainTessellator::updateRenderData() {
    U16 renderDepth = 0;
    renderRecursive(_tree.front(), renderDepth);

    _renderDepth = renderDepth;
    return _renderDepth;
}

const TerrainTessellator::RenderDataVector& TerrainTessellator::renderData() const {
    return _renderData;
}

void TerrainTessellator::clearTree() {
    memset(_tree.data(), 0, sizeof(TessellatedTerrainNode) * _tree.size());
    _numNodes = 0;
}

bool TerrainTessellator::checkDivide(TessellatedTerrainNode &node) {
    // Distance from current origin to camera
    F32 d = std::abs(_cameraEyeCache.xz().distance(vec2<F32>{node.origin[0], node.origin[2]}));

    // Check base case:
    // Distance to camera is greater than twice the length of the diagonal
    // from current origin to corner of current square.
    // OR
    // Max recursion level has been hit
    if (d > 2.5f * Sqrt((std::pow(0.5f * node.width, 2.0f) +
                         std::pow(0.5f * node.height, 2.0f))) ||
        node.width < TERRAIN_CUTOFF_DISTANCE)
    {
        return false;
    }

    return true;
}

bool TerrainTessellator::divideNode(TessellatedTerrainNode &node) {
    // Subdivide
    F32 w_new = 0.5f * node.width;
    F32 h_new = 0.5f * node.height;

    // Create the child nodes
    node.c1 = createNode(node, 1u, node.origin[0] - 0.5f * w_new, node.origin[1], node.origin[2] - 0.5f * h_new, w_new, h_new);
    node.c2 = createNode(node, 2u, node.origin[0] + 0.5f * w_new, node.origin[1], node.origin[2] - 0.5f * h_new, w_new, h_new);
    node.c3 = createNode(node, 3u, node.origin[0] + 0.5f * w_new, node.origin[1], node.origin[2] + 0.5f * h_new, w_new, h_new);
    node.c4 = createNode(node, 4u, node.origin[0] - 0.5f * w_new, node.origin[1], node.origin[2] + 0.5f * h_new, w_new, h_new);

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
    bool div1 = checkDivide(*node.c1);
    bool div2 = checkDivide(*node.c2);
    bool div3 = checkDivide(*node.c3);
    bool div4 = checkDivide(*node.c4);

    if (div1) {
        divideNode(*node.c1);
    }
    if (div2) {
        divideNode(*node.c2);
    }
    if (div3) {
        divideNode(*node.c3);
    }
    if (div4) {
        divideNode(*node.c4);
    }

    return div1 || div2 || div3 || div4;
}

void TerrainTessellator::createTree(const vec3<F32>& camPos, const vec3<F32>& origin, const vec2<U16>& terrainDimensions) {
    _cameraEyeCache.set(camPos);
    _originiCache.set(origin);

    clearTree();

    TessellatedTerrainNode& root = _tree[_numNodes];
    root.type = 0u; // Root node
    root.origin[0] = origin[0];
    root.origin[1] = origin[1];
    root.origin[2] = origin[2];
    root.width = terrainDimensions.width;
    root.height = terrainDimensions.height;
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
    divideNode(root);
}

TessellatedTerrainNode* TerrainTessellator::createNode(TessellatedTerrainNode &parent, U8 type, F32 x, F32 y, F32 z, F32 width, F32 height) {
    if (_numNodes >= Terrain::MAX_RENDER_NODES) {
        return nullptr;
    }

    _numNodes++;

    TessellatedTerrainNode& node = _tree[_numNodes];
    node.type = type;
    node.origin[0] = x;
    node.origin[1] = y;
    node.origin[2] = z;
    node.width = width;
    node.height = height;
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

void TerrainTessellator::renderNode(TessellatedTerrainNode& node, U16 renderDepth) {
    // Calculate the tess scale factor
    calcTessScale(node);

    _renderData[renderDepth].set({
                                    node.origin[0],
                                    node.origin[1],
                                    node.origin[2]
                                 },
                                 node.width * 0.5f,
                                 node.tscale_negx,
                                 node.tscale_negz,
                                 node.tscale_posx,
                                 node.tscale_posz);
}

void TerrainTessellator::renderRecursive(TessellatedTerrainNode& node, U16& renderDepth) {
    if (renderDepth >= _maxRenderDepth) {
        return;
    }

    // If all children are null, render this node
    if (!node.c1 && !node.c2 && !node.c3 && !node.c4)
    {
        renderNode(node, renderDepth);
        renderDepth++;
        return;
    }

    // Otherwise, recurse to the children.
    if (node.c1) {
        renderRecursive(*node.c1, renderDepth);
    }
    if (node.c2) {
        renderRecursive(*node.c2, renderDepth);
    }
    if (node.c3) {
        renderRecursive(*node.c3, renderDepth);
    }
    if (node.c4) {
        renderRecursive(*node.c4, renderDepth);
    }
}

void TerrainTessellator::calcTessScale(TessellatedTerrainNode& node) {
    TessellatedTerrainNode* t = nullptr;

    TessellatedTerrainNode& root = _tree[0];

    // Positive Z (north)
    t = find(root, node.origin[0], node.origin[2] + 1 + node.width * 0.5f);
    if (t->width > node.width) {
        node.tscale_posz = 2.0;
    }

    // Positive X (east)
    t = find(root, node.origin[0] + 1 + node.width * 0.5f, node.origin[2]);
    if (t->width > node.width) {
        node.tscale_posx = 2.0;
    }

    // Negative Z (south)
    t = find(root, node.origin[0], node.origin[2] - 1 - node.width * 0.5f);
    if (t->width > node.width) {
        node.tscale_negz = 2.0;
    }

    // Negative X (west)
    t = find(root, node.origin[0] - 1 - node.width * 0.5f, node.origin[2]);
    if (t->width > node.width) {
        node.tscale_negx = 2.0;
    }
}

TessellatedTerrainNode* TerrainTessellator::find(TessellatedTerrainNode& n, F32 x, F32 z) {
    if (COMPARE(n.origin[0], x) && COMPARE(n.origin[2], z)) {
        return &n;
    }

    if (!n.c1 && !n.c2 && !n.c3 && !n.c4) {
        return &n;
    }

    if (IS_GEQUAL(n.origin[0], x) && IS_GEQUAL(n.origin[2], z) && n.c1) {
        return find(*n.c1, x, z);
    } else if (IS_LEQUAL(n.origin[0], x) && IS_GEQUAL(n.origin[2], z) && n.c2) {
        return find(*n.c2, x, z);
    } else if (IS_LEQUAL(n.origin[0], x) && IS_LEQUAL(n.origin[2], z) && n.c3) {
        return find(*n.c3, x, z);
    } else if (IS_GEQUAL(n.origin[0], x) && IS_LEQUAL(n.origin[2], z) && n.c4) {
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
