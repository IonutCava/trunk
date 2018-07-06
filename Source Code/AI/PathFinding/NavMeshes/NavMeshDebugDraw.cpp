// With huge thanks to:
//    Mikko Mononen http://groups.google.com/group/recastnavigation
// And thanks to  Daniel Buckmaster, 2011 for his
// T3D 1.1 implementation of Recast's DebugUtils.

#include "stdafx.h"

#include "Headers/NavMeshDebugDraw.h"
#include "Headers/NavMesh.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {
namespace AI {
namespace Navigation {

NavMeshDebugDraw::NavMeshDebugDraw(GFXDevice& context)
    : _context(context),
      _overrideColour(false),
      _dirty(true),
      _paused(false),
      _colour(0),
      _primitive(nullptr),
      _primType(PrimitiveType::COUNT),
      _vertCount(0)
{
}

NavMeshDebugDraw::~NavMeshDebugDraw()
{
	_primitive->clear();
}

void NavMeshDebugDraw::paused(bool state) {
    _paused = state;
    if (_primitive) {
        _primitive->paused(_paused);
    }
}

void NavMeshDebugDraw::beginBatch() {
    if (!_primitive) {
        _dirty = true;
        _primitive = _context.newIMP();

        // Generate a render state
        RenderStateBlock navigationDebugStateBlock;
        navigationDebugStateBlock.setCullMode(CullMode::NONE);

        PipelineDescriptor pipeDesc;
        pipeDesc._stateHash = navigationDebugStateBlock.getHash();

        Pipeline pipeline = _context.newPipeline(pipeDesc);
        _primitive->pipeline(pipeline);
    }

    assert(_primitive != nullptr);

    if (_dirty) {
        _primitive->beginBatch(true, 1024, 1);
    }
}

void NavMeshDebugDraw::endBatch() {
    if (!_dirty) {
        return;
    }

    if (_primitive) {
        _primitive->endBatch();
    }

    _dirty = false;
}

void NavMeshDebugDraw::begin(duDebugDrawPrimitives prim, F32 size) {
    if (!_dirty || !_primitive) {
        return;
    }

    switch (prim) {
        default:
        case DU_DRAW_TRIS:
            _primType = PrimitiveType::TRIANGLES;
            break;
        case DU_DRAW_POINTS:
            _primType = PrimitiveType::API_POINTS;
            break;
        case DU_DRAW_LINES:
            _primType = PrimitiveType::LINES;
            break;
        case DU_DRAW_QUADS: /*_primType = PrimitiveType::QUADS;*/
            assert(prim == DU_DRAW_QUADS);
    }

    _primitive->attribute4f(to_base(AttribLocation::VERTEX_COLOR), vec4<F32>(1.0f, 1.0f, 1.0f, 0.5f));
    _primitive->begin(_primType);
}

void NavMeshDebugDraw::vertex(const F32 x, const F32 y, const F32 z,
                              U32 colour) {
    if (!_dirty || !_primitive) {
        return;
    }
    if (_overrideColour) {
        colour = _colour;
    }

    UColour colourVec;
    rcCol(colour, colourVec.r, colourVec.g, colourVec.b, colourVec.a);
    colourVec.a = 64;

    _primitive->attribute4f(to_base(AttribLocation::VERTEX_COLOR), Util::ToFloatColour(colourVec));
    _primitive->vertex(x, y, z);
}

void NavMeshDebugDraw::end() {
    if (_dirty && _primitive) {
        _primitive->end();
    }
}

GFX::CommandBuffer NavMeshDebugDraw::toCommandBuffer() const {
    return _primitive->toCommandBuffer();
}

void NavMeshDebugDraw::overrideColour(U32 col) {
    _overrideColour = true;
    _colour = col;
}
};  // namespace Navigation
};  // namespace AI
};  // namespace Divide