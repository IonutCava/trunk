#include "stdafx.h"

#include "Headers/GFXRTPool.h"
#include "Headers/GFXDevice.h"

#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"

namespace Divide {

namespace {
    U32 g_maxAdditionalRenderTargets = 128;
};

GFXRTPool::GFXRTPool(GFXDevice& parent)
    : _parent(parent),
      _activeRenderTarget(nullptr)
{
    _renderTargets[to_U32(RenderTargetUsage::SCREEN)].resize(1, nullptr);
    _renderTargets[to_U32(RenderTargetUsage::OIT)].resize(1, nullptr);
    _renderTargets[to_U32(RenderTargetUsage::SHADOW)].resize(to_base(ShadowType::COUNT), nullptr);
    _renderTargets[to_U32(RenderTargetUsage::REFLECTION_PLANAR)].resize(Config::MAX_REFLECTIVE_NODES_IN_VIEW, nullptr);
    _renderTargets[to_U32(RenderTargetUsage::REFRACTION_PLANAR)].resize(Config::MAX_REFRACTIVE_NODES_IN_VIEW, nullptr);

    _renderTargets[to_U32(RenderTargetUsage::REFLECTION_CUBE)].resize(1, nullptr);
    _renderTargets[to_U32(RenderTargetUsage::REFRACTION_CUBE)].resize(1, nullptr);
    _renderTargets[to_U32(RenderTargetUsage::ENVIRONMENT)].resize(1, nullptr);

    _renderTargets[to_U32(RenderTargetUsage::OTHER)].resize(g_maxAdditionalRenderTargets, nullptr);
}

GFXRTPool::~GFXRTPool()
{
    clear();
}

void GFXRTPool::clear() {
    // Delete all of our rendering targets
    for (U8 i = 0; i < to_base(RenderTargetUsage::COUNT); ++i) {
        for (U32 j = 0; j < to_U32(_renderTargets[i].size()); ++j) {
            set(RenderTargetID(static_cast<RenderTargetUsage>(i), j), nullptr);
        }
    }
}

void GFXRTPool::resizeTargets(RenderTargetUsage target, U16 width, U16 height) {
    for (RenderTarget* rt : _renderTargets[to_U32(target)]) {
        if (rt) {
            rt->resize(width, height);
        }
    }
}

void GFXRTPool::set(RenderTargetID target, RenderTarget* newTarget) {
    RenderTarget*& oldTarget = _renderTargets[to_U32(target._usage)][target._index];
    if (oldTarget) {
        //overriding old target
    }

    oldTarget = newTarget;
}

RenderTargetHandle GFXRTPool::add(RenderTargetUsage targetUsage, RenderTarget* newTarget) {
    vectorImpl<RenderTarget*>& rts = _renderTargets[to_U32(targetUsage)];

    for (U32 i = 0; i < to_U32(rts.size()); ++i) {
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

RenderTargetHandle GFXRTPool::allocateRT(RenderTargetUsage targetUsage, const RenderTargetDescriptor& descriptor) {
    return add(targetUsage, Attorney::GFXDeviceGFXRTPool::newRT(_parent, descriptor));
}

}; //namespace Divide