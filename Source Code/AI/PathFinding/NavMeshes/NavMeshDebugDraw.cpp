// With huge thanks to:
//    Mikko Mononen http://groups.google.com/group/recastnavigation
// And thanks to  Daniel Buckmaster, 2011 for his
// T3D 1.1 implementation of Recast's DebugUtils.

#include "Headers/NavMeshDebugDraw.h"
#include "Headers/NavMesh.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {
namespace AI {
namespace Navigation {
NavMeshDebugDraw::NavMeshDebugDraw()
    : _overrideColor(false),
      _dirty(true),
      _paused(false),
      _color(0),
      _primitive(nullptr) {
    // Generate a render state
    RenderStateBlockDescriptor navigationDebugDesc;
    navigationDebugDesc.setCullMode(CullMode::NONE);
    navigationDebugDesc.setBlend(true);
    _navMeshStateBlockHash =
        GFX_DEVICE.getOrCreateStateBlock(navigationDebugDesc);
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
    RenderStateBlockDescriptor newDepthDesc(
        GFX_DEVICE.getStateBlockDescriptor(_navMeshStateBlockHash));
    newDepthDesc.setZReadWrite(true, state);
    _navMeshStateBlockHash = GFX_DEVICE.getOrCreateStateBlock(newDepthDesc);
}

void NavMeshDebugDraw::beginBatch() {
    if (!_primitive) {
        _dirty = true;
        _primitive = GFX_DEVICE.getOrCreatePrimitive(false);
        _primitive->stateHash(_navMeshStateBlockHash);
    }

    assert(_primitive != nullptr);

    if (_dirty) {
        _primitive->beginBatch();
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

    _primitive->attribute4ub("inColorData", vec4<U8>(255, 255, 255, 128));
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

    U8 colorVec[4];
    rcCol(color, colorVec[0], colorVec[1], colorVec[2], colorVec[3]);
    colorVec[3] = 64;

    _primitive->attribute4ub("inColorData", colorVec[0], colorVec[1],
                             colorVec[2], colorVec[3]);
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