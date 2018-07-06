#include "Headers/GFXRTPool.h"
#include "Headers/GFXDevice.h"

#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"

namespace Divide {

namespace {
    U32 g_maxAdditionalRenderTargets = 128;
};

GFXRTPool::GFXRTPool(GFXDevice& parent)
    : _parent(parent)
{
    _renderTargets[to_uint(RenderTargetUsage::SCREEN)].resize(1, nullptr);
    _renderTargets[to_uint(RenderTargetUsage::SHADOW)].resize(to_const_uint(ShadowType::COUNT), nullptr);
    _renderTargets[to_uint(RenderTargetUsage::REFLECTION_PLANAR)].resize(Config::MAX_REFLECTIVE_NODES_IN_VIEW, nullptr);
    _renderTargets[to_uint(RenderTargetUsage::REFRACTION_PLANAR)].resize(Config::MAX_REFRACTIVE_NODES_IN_VIEW, nullptr);

    _renderTargets[to_uint(RenderTargetUsage::REFLECTION_CUBE)].resize(1, nullptr);
    _renderTargets[to_uint(RenderTargetUsage::REFRACTION_CUBE)].resize(1, nullptr);
    _renderTargets[to_uint(RenderTargetUsage::ENVIRONMENT)].resize(1, nullptr);

    _renderTargets[to_uint(RenderTargetUsage::OTHER)].resize(g_maxAdditionalRenderTargets, nullptr);
}

GFXRTPool::~GFXRTPool()
{
    clear();
}

void GFXRTPool::clear() {
    // Delete all of our rendering targets
    for (U8 i = 0; i < to_const_ubyte(RenderTargetUsage::COUNT); ++i) {
        for (U32 j = 0; j < to_uint(_renderTargets[i].size()); ++j) {
            set(RenderTargetID(static_cast<RenderTargetUsage>(i), j), nullptr);
        }
    }
}

void GFXRTPool::resizeTargets(RenderTargetUsage target, U16 width, U16 height) {
    for (RenderTarget* rt : _renderTargets[to_uint(target)]) {
        if (rt) {
            rt->create(width, height);
        }
    }
}

void GFXRTPool::set(RenderTargetID target, RenderTarget* newTarget) {
    RenderTarget*& oldTarget = _renderTargets[to_uint(target._usage)][target._index];
    if (oldTarget) {
        //overriding old target
    }

    oldTarget = newTarget;
}

RenderTargetHandle GFXRTPool::add(RenderTargetUsage targetUsage, RenderTarget* newTarget) {
    vectorImpl<RenderTarget*>& rts = _renderTargets[to_uint(targetUsage)];

    for (U32 i = 0; i < to_uint(rts.size()); ++i) {
        if (rts[i] == nullptr) {
            rts[i] = newTarget;
            return RenderTargetHandle(RenderTargetID(targetUsage, i), newTarget);
        }
    }

    DIVIDE_UNEXPECTED_CALL("No more render targets available!");
    return RenderTargetHandle();
}

bool GFXRTPool::remove(RenderTargetHandle& handle) {
    bool state = false;
    if (handle._targetID._usage != RenderTargetUsage::COUNT) {
        set(handle._targetID, nullptr);
    } else {
        state = handle._rt == nullptr;
    }

    handle = RenderTargetHandle();
    return state;
}

}; //namespace Divide