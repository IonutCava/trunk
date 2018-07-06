// With huge thanks to:
//    Mikko Mononen http://groups.google.com/group/recastnavigation
// And thanks to  Daniel Buckmaster, 2011 for his
// T3D 1.1 implementation of Recast's DebugUtils.

#include "Headers/NavMeshDebugDraw.h"
#include "Headers/NavMesh.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/IMPrimitive.h"

namespace Divide {
namespace AI {
namespace Navigation {

NavMeshDebugDraw::NavMeshDebugDraw()
    : _overrideColor(false),
      _dirty(true),
      _paused(false),
      _color(0),
      _primitive(nullptr)
{
    // Generate a render state
    RenderStateBlock navigationDebugStateBlock;
    navigationDebugStateBlock.setCullMode(CullMode::NONE);
    navigationDebugStateBlock.setBlend(true);
    _navMeshStateBlockHash = navigationDebugStateBlock.getHash();
}

NavMeshDebugDraw::~NavMeshDebugDraw() {
    // Allow the primitive to be deleted
    if (_primitive) {
        _primitive->_canZombify = true;
    }
}

void NavMeshDebugDraw::paused(bool state) {
    _paused = state;
    if (_primitive) {
        _primitive->paused(_paused);
    }
}

void NavMeshDebugDraw::depthMask(bool state) {
    RenderStateBlock newDepthDesc(
        GFX_DEVICE.getRenderStateBlock(_navMeshStateBlockHash));

    newDepthDesc.setZWrite(state);
    _navMeshStateBlockHash = newDepthDesc.getHash();
}

void NavMeshDebugDraw::beginBatch() {
    if (!_primitive) {
        _dirty = true;
        _primitive = GFX_DEVICE.getOrCreatePrimitive(false);
        _primitive->stateHash(_navMeshStateBlockHash);
    }

    assert(_primitive != nullptr);

    if (_dirty) {
        _primitive->beginBatch(true, 1024);
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

    _primitive->attribute4f(to_uint(AttribLocation::VERTEX_COLOR), vec4<F32>(1.0f, 1.0f, 1.0f, 0.5f));
    _primitive->begin(_primType);
}

void NavMeshDebugDraw::vertex(const F32 x, const F32 y, const F32 z,
                              U32 color) {
    if (!_dirty || !_primitive) {
        return;
    }
    if (_overrideColor) {
        color = _color;
    }

    vec4<U8> colorVec;
    rcCol(color, colorVec.r, colorVec.g, colorVec.b, colorVec.a);
    colorVec.a = 64;

    _primitive->attribute4f(to_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(colorVec));
    _primitive->vertex(x, y, z);
}

void NavMeshDebugDraw::end() {
    if (_dirty && _primitive) {
        _primitive->end();
    }
}

void NavMeshDebugDraw::overrideColor(U32 col) {
    _overrideColor = true;
    _color = col;
}
};  // namespace Navigation
};  // namespace AI
};  // namespace Divide