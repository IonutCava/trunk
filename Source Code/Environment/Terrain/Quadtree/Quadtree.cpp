#include "Headers/Quadtree.h"
#include "Headers/QuadtreeNode.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

Quadtree::Quadtree() : _parentVB(nullptr) { _chunkCount = 0; }

Quadtree::~Quadtree() {}

void Quadtree::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                           SceneState& sceneState) {
    assert(_root);
    _root->sceneUpdate(deltaTime, sgn, sceneState);
}

void Quadtree::createDrawCommands(
    const SceneRenderState& sceneRenderState,
    vectorImpl<GenericDrawCommand>& drawCommandsOut) {
    assert(_root);
    U32 options = to_uint(ChunkBit::CHUNK_BIT_TESTCHILDREN);
    if (GFX_DEVICE.getRenderStage() == RenderStage::REFLECTION) {
        options |= to_uint(ChunkBit::CHUNK_BIT_WATERREFLECTION);
    } else if (GFX_DEVICE.getRenderStage() == RenderStage::SHADOW) {
        options |= to_uint(ChunkBit::CHUNK_BIT_SHADOWMAP);
    }
    _root->createDrawCommand(options, sceneRenderState, drawCommandsOut);
}

void Quadtree::drawBBox() const {
    assert(_root);
    _root->drawBBox();
    
    IMPrimitive* primitive = GFX_DEVICE.getOrCreatePrimitive();
    primitive->name("QuadtreeBoundingBox");
    RenderStateBlock primitiveRenderState;
    primitiveRenderState.setLineWidth(8.0f);
    primitive->stateHash(primitiveRenderState.getHash());

    GFX_DEVICE.drawBox3D(*primitive,
                         _root->getBoundingBox().getMin(),
                         _root->getBoundingBox().getMax(),
                         vec4<U8>(0, 64, 255, 255));
}

QuadtreeNode* Quadtree::findLeaf(const vec2<F32>& pos) {
    assert(_root);
    QuadtreeNode* node = _root.get();
    QuadtreeNode* child = nullptr;
    while (!node->isALeaf()) {
        U32 i = 0;
        for (i = 0; i < 4; i++) {
            child = node->getChild(i);
            const BoundingBox& bb = child->getBoundingBox();
            if (bb.ContainsPoint(vec3<F32>(pos.x, bb.getCenter().y, pos.y))) {
                node = child;
                break;
            }
        }

        if (i >= 4) return nullptr;
    }

    return node;
}

void Quadtree::Build(BoundingBox& terrainBBox, const vec2<U32>& HMsize,
                     U32 minHMSize, Terrain* const terrain) {
    _root.reset(new QuadtreeNode());
    _root->setBoundingBox(terrainBBox);

    _root->Build(0, vec2<U32>(0, 0), HMsize, minHMSize, terrain, _chunkCount);

    // Generate index buffer
    const U32 terrainWidth = HMsize.x;
    const U32 terrainHeight = HMsize.y;
    vec3<U32> firstTri, secondTri;
    I32 vertexIndex = -1;
    for (U32 j = 0; j < (terrainHeight - 1); ++j) {
        for (U32 i = 0; i < (terrainWidth - 1); ++i) {
            vertexIndex = (j * terrainWidth) + i;
            // Top triangle (T0)
            firstTri.set(vertexIndex, vertexIndex + terrainWidth + 1,
                         vertexIndex + 1);
            // Bottom triangle (T1)
            secondTri.set(vertexIndex, vertexIndex + terrainWidth,
                          vertexIndex + terrainWidth + 1);
            terrain->addTriangle(firstTri);
            terrain->addTriangle(secondTri);
        }
    }
}

BoundingBox& Quadtree::computeBoundingBox() {
    assert(_root);
    _root->computeBoundingBox();
    return _root->getBoundingBox();
}
};