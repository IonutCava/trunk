#include "stdafx.h"

#include "Headers/Quadtree.h"
#include "Headers/QuadtreeNode.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderPackage.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

Quadtree::Quadtree() : _parentVB(nullptr),
                       _bbPrimitive(nullptr)
{
    _root = std::make_unique<QuadtreeNode>();
    _chunkCount = 0;
}

Quadtree::~Quadtree()
{
}

void Quadtree::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn,
                           SceneState& sceneState) {
    assert(_root);
    _root->sceneUpdate(deltaTimeUS, sgn, sceneState);
}

void Quadtree::drawBBox(GFXDevice& context, RenderPackage& packageOut) {
    assert(_root);
    _root->drawBBox(context, packageOut);
    
    if (!_bbPrimitive) {
        _bbPrimitive = context.newIMP();
        _bbPrimitive->name("QuadtreeBoundingBox");
        RenderStateBlock primitiveRenderState;
        PipelineDescriptor pipeDesc;
        pipeDesc._stateHash = primitiveRenderState.getHash();
        pipeDesc._shaderProgramHandle = ShaderProgram::defaultShader()->getID();
        _bbPrimitive->pipeline(*context.newPipeline(pipeDesc));
    }

    _bbPrimitive->fromBox(_root->getBoundingBox().getMin(),
                          _root->getBoundingBox().getMax(),
                          UColour(0, 64, 255, 255));

    packageOut.addCommandBuffer(_bbPrimitive->toCommandBuffer());
}

QuadtreeNode* Quadtree::findLeaf(const vec2<F32>& pos) {
    assert(_root);

    QuadtreeNode* node = _root.get();
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

void Quadtree::build(GFXDevice& context,
                     BoundingBox& terrainBBox,
                     const vec2<U16>& HMsize,
                     U32 targetChunkDimension,
                     Terrain* const terrain) {

    _root->setBoundingBox(terrainBBox);
    _root->build(context, 0, vec2<U16>(0u), HMsize, targetChunkDimension, terrain, _chunkCount);
}

const BoundingBox& Quadtree::computeBoundingBox() {
    assert(_root);
    _root->computeBoundingBox();
    return _root->getBoundingBox();
}
};