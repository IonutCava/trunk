#include "stdafx.h"

#include "Headers/Quadtree.h"
#include "Headers/QuadtreeNode.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

Quadtree::Quadtree() : _parentVB(nullptr),
                       _root(nullptr),
                       _bbPrimitive(nullptr)
{
    _chunkCount = 0;
}

Quadtree::~Quadtree()
{
    MemoryManager::DELETE(_root);
}

void Quadtree::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                           SceneState& sceneState) {
    assert(_root);
    _root->sceneUpdate(deltaTime, sgn, sceneState);
}

void Quadtree::drawBBox(GFXDevice& context, GenericDrawCommands& commandsOut) {
    assert(_root);
    _root->drawBBox(context, commandsOut);
    
    if (!_bbPrimitive) {
        _bbPrimitive = context.newIMP();
        _bbPrimitive->name("QuadtreeBoundingBox");
        RenderStateBlock primitiveRenderState;
        PipelineDescriptor pipeDesc;
        pipeDesc._stateHash = primitiveRenderState.getHash();
        _bbPrimitive->pipeline(context.newPipeline(pipeDesc));
    }

    _bbPrimitive->fromBox(_root->getBoundingBox().getMin(),
                          _root->getBoundingBox().getMax(),
                          vec4<U8>(0, 64, 255, 255));

    commandsOut.push_back(_bbPrimitive->toDrawCommand());
}

QuadtreeNode* Quadtree::findLeaf(const vec2<F32>& pos) {
    assert(_root);

    QuadtreeNode* node = _root;
    QuadtreeNode* child = nullptr;
    while (!node->isALeaf()) {
        U32 i = 0;
        for (i = 0; i < 4; i++) {
            child = &(node->getChild(i));
            const BoundingBox& bb = child->getBoundingBox();
            if (bb.containsPoint(vec3<F32>(pos.x, bb.getCenter().y, pos.y))) {
                node = child;
                break;
            }
        }

        if (i >= 4) {
            return nullptr;
        }
    }

    return node;
}

void Quadtree::Build(GFXDevice& context,
                     BoundingBox& terrainBBox,
                     const vec2<U32>& HMsize,
                     U32 targetChunkDimension,
                     Terrain* const terrain) {
    _root = MemoryManager_NEW QuadtreeNode();
    _root->setBoundingBox(terrainBBox);

    _root->Build(context, 0, vec2<U32>(0, 0), HMsize, targetChunkDimension, terrain, _chunkCount);

    // Generate index buffer
    const U32 terrainWidth = HMsize.x;
    const U32 terrainHeight = HMsize.y;
    vectorImpl<vec3<U32>>& triangles = terrain->getTriangles();
    triangles.reserve(terrainHeight * terrainWidth * 2);

    I32 vertexIndex = -1;
    for (U32 j = 0; j < (terrainHeight - 1); ++j) {
        for (U32 i = 0; i < (terrainWidth - 1); ++i) {
            vertexIndex = (j * terrainWidth) + i;
            // Top triangle (T0)
            triangles.emplace_back(vertexIndex, vertexIndex + terrainWidth + 1, vertexIndex + 1);
            // Bottom triangle (T1)
            triangles.emplace_back(vertexIndex, vertexIndex + terrainWidth, vertexIndex + terrainWidth + 1);
        }
    }
}

BoundingBox& Quadtree::computeBoundingBox() {
    assert(_root);
    _root->computeBoundingBox();
    return _root->getBoundingBox();
}
};