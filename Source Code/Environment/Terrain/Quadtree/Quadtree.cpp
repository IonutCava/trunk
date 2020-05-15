#include "stdafx.h"

#include "Headers/Quadtree.h"
#include "Headers/QuadtreeNode.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderPackage.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

Quadtree::Quadtree(GFXDevice& context)
    : _context(context),
      _root(eastl::make_unique<QuadtreeNode>(context, this))
{
    RenderStateBlock primitiveRenderState;
    PipelineDescriptor pipeDesc;
    pipeDesc._stateHash = primitiveRenderState.getHash();
    pipeDesc._shaderProgramHandle = ShaderProgram::defaultShader()->getGUID();
    _bbPipeline = _context.newPipeline(pipeDesc);
}

Quadtree::~Quadtree()
{
    if (_bbPrimitive) {
        _context.destroyIMP(_bbPrimitive);
    }
}

void Quadtree::toggleBoundingBoxes() {
    _drawBBoxes = !_drawBBoxes;
    if (_drawBBoxes) {
        _bbPrimitive = _context.newIMP();
        _bbPrimitive->name("QuadtreeBoundingBox");
        _bbPrimitive->pipeline(*_bbPipeline);
    } else {
        _context.destroyIMP(_bbPrimitive);
    }

    _root->toggleBoundingBoxes();
}

void Quadtree::drawBBox(RenderPackage& packageOut) {
    if (!_drawBBoxes) {
        return;
    }

    assert(_root && _bbPrimitive);
    _root->drawBBox(packageOut);
 
    _bbPrimitive->fromBox(_root->getBoundingBox().getMin(),
                          _root->getBoundingBox().getMax(),
                          UColour4(0, 64, 255, 255));

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

void Quadtree::build(BoundingBox& terrainBBox,
                     const vec2<U16>& HMsize,
                     U32 targetChunkDimension,
                     Terrain* const terrain) {

    _root->setBoundingBox(terrainBBox);
    _root->build(0, vec2<U16>(0u), HMsize, targetChunkDimension, terrain, _chunkCount);
}

const BoundingBox& Quadtree::computeBoundingBox() {
    assert(_root);
    BoundingBox rootBB = _root->getBoundingBox();
    _root->computeBoundingBox(rootBB);
    return _root->getBoundingBox();
}
};