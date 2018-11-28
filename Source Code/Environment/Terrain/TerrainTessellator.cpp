#include "stdafx.h"

#include "Headers/TerrainTessellator.h"

#include "Headers/Terrain.h"

#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

namespace {
    // The size of a patch in meters at which point to stop subdividing a terrain patch once 
    // it's width is less than the cutoff
    constexpr U32 TERRAIN_CUTOFF_DISTANCE = 100;

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
     _renderDepth(0)
{
    _tree.resize(Terrain::MAX_RENDER_NODES + 1);
    _renderData.resize(Terrain::MAX_RENDER_NODES + 1);

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
    F32 d = _cameraEyeCache.xz().distanceSquared(node.origin.xz());

    // Check base case:
    // Distance to camera is greater than twice the length of the diagonal
    // from current origin to corner of current square.
    // OR
    // Max recursion level has been hit
    if (/*d > 2.5f * (std::pow(0.5f * node.dim.width, 2.0f) + std::pow(0.5f * node.dim.height, 2.0f)) ||
        */node.dim.width < TERRAIN_CUTOFF_DISTANCE)
    {
        return false;
    }

    return true;
}

bool TerrainTessellator::divideNode(TessellatedTerrainNode &node) {
    // Subdivide
    F32 w_new = node.dim.width * 0.5f;
    F32 h_new = node.dim.height * 0.5f;

    // Create the child nodes
    node.c[0] = createNode(node, TessellatedTerrainNodeType::CHILD_1, node.origin.x - 0.5f * w_new, node.origin.y, node.origin.z - 0.5f * h_new, w_new, h_new);
    node.c[1] = createNode(node, TessellatedTerrainNodeType::CHILD_2, node.origin.x + 0.5f * w_new, node.origin.y, node.origin.z - 0.5f * h_new, w_new, h_new);
    node.c[2] = createNode(node, TessellatedTerrainNodeType::CHILD_3, node.origin.x + 0.5f * w_new, node.origin.y, node.origin.z + 0.5f * h_new, w_new, h_new);
    node.c[3] = createNode(node, TessellatedTerrainNodeType::CHILD_4, node.origin.x - 0.5f * w_new, node.origin.y, node.origin.z + 0.5f * h_new, w_new, h_new);

    // Assign neighbors
    switch (node.type) {
        case TessellatedTerrainNodeType::CHILD_1: {
            node.e = node.p->c[1];
            node.n = node.p->c[3];
        } break;
        case TessellatedTerrainNodeType::CHILD_2: {
            node.w = node.p->c[0];
            node.n = node.p->c[2];
        } break;
        case TessellatedTerrainNodeType::CHILD_3: {
            node.s = node.p->c[1];
            node.w = node.p->c[3];
        } break;
        case TessellatedTerrainNodeType::CHILD_4: {
            node.s = node.p->c[0];
            node.e = node.p->c[2];
        }break;
        default:
            break;
    };

    // Check if each of these four child nodes will be subdivided.
    bool divided = false;
    for (U8 i = 0; i < 4; ++i) {
        if (checkDivide(*node.c[i])) {
            divideNode(*node.c[i]);
            divided = true;
        }
    }

    return divided;
}

void TerrainTessellator::createTree(const vec3<F32>& camPos, const vec3<F32>& origin, const vec2<U16>& terrainDimensions) {
    _cameraEyeCache.set(camPos);
    _originiCache.set(origin);

    clearTree();

    TessellatedTerrainNode& root = _tree[_numNodes];
    root.type = TessellatedTerrainNodeType::ROOT; // Root node
    root.origin = origin;
    root.dim.set(terrainDimensions.width, terrainDimensions.height);
    root.tscale.set(1.0f);
    root.p = nullptr;
    root.n = nullptr;
    root.s = nullptr;
    root.e = nullptr;
    root.w = nullptr;

    // Recursively subdivide the terrain
    divideNode(root);
}

TessellatedTerrainNode* TerrainTessellator::createNode(TessellatedTerrainNode &parent, TessellatedTerrainNodeType type, F32 x, F32 y, F32 z, F32 width, F32 height) {
    if (_numNodes >= Terrain::MAX_RENDER_NODES) {
        return nullptr;
    }

    _numNodes++;

    TessellatedTerrainNode& node = _tree[_numNodes];
    node.type = type;
    node.origin.set(x, y, z);
    node.dim.set(width, height);
    node.tscale.set(1.0f);
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

    TessellatedNodeData& data = _renderData[renderDepth];
    data._positionAndTileScale.set(node.origin, node.dim.width * 0.5f);
    data._tScale.set(node.tscale);
}

void TerrainTessellator::renderRecursive(TessellatedTerrainNode& node, U16& renderDepth) {
    // If all children are null, render this node
    if (!hasChildren(node)) {
        renderNode(node, renderDepth);
        renderDepth++;
    } else {
        // Otherwise, recurse to the children.
        for (U8 i = 0; i < 4; ++i) {
            assert(node.c[i] != nullptr);
            renderRecursive(*node.c[i], renderDepth);
        }
    }
}

void TerrainTessellator::calcTessScale(TessellatedTerrainNode& node) {
    TessellatedTerrainNode& root = _tree[0];
    TessellatedTerrainNode* t = nullptr;
    // Positive Z (north)
    t = find(root, node.origin.x, node.origin.z + 1 + node.dim.width * 0.5f);
    if (t->dim.width > node.dim.width) {
        node.tscale[3] = 2.0;
    }

    // Positive X (east)
    t = find(root, node.origin.x + 1 + node.dim.width * 0.5f, node.origin.z);
    if (t->dim.width > node.dim.width) {
        node.tscale[1] = 2.0;
    }

    // Negative Z (south)
    t = find(root, node.origin.x, node.origin.z - 1 - node.dim.width * 0.5f);
    if (t->dim.width > node.dim.width) {
        node.tscale[2] = 2.0;
    }

    // Negative X (west)
    t = find(root, node.origin.x - 1 - node.dim.width * 0.5f, node.origin.z);
    if (t->dim.width > node.dim.width) {
        node.tscale[0] = 2.0;
    }
}

TessellatedTerrainNode* TerrainTessellator::find(TessellatedTerrainNode& n, F32 x, F32 z) {
    if (COMPARE(n.origin.x, x) && COMPARE(n.origin.z, z)) {
        return &n;
    }

    if (!n.c[0] && !n.c[1] && !n.c[2] && !n.c[3]) {
        return &n;
    }

    if (IS_GEQUAL(n.origin.x, x) && IS_GEQUAL(n.origin.z, z) && n.c[0]) {
        return find(*n.c[0], x, z);
    } else if (IS_LEQUAL(n.origin.x, x) && IS_GEQUAL(n.origin.z, z) && n.c[1]) {
        return find(*n.c[1], x, z);
    } else if (IS_LEQUAL(n.origin.x, x) && IS_LEQUAL(n.origin.z, z) && n.c[2]) {
        return find(*n.c[2], x, z);
    } else if (IS_GEQUAL(n.origin.x, x) && IS_LEQUAL(n.origin.z, z) && n.c[3]) {
        return find(*n.c[3], x, z);
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
