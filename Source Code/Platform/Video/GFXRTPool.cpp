#include "Headers/GFXRTPool.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"

namespace Divide {

namespace {
    U32 g_maxAdditionalRenderTargets = 128;
};

GFXRTPool::GFXRTPool()
{
    _renderTargets[to_uint(RenderTargetID::SCREEN)].resize(1, nullptr);
    _renderTargets[to_uint(RenderTargetID::SCREEN_PREV)].resize(1, nullptr);
    _renderTargets[to_uint(RenderTargetID::SHADOW)].resize(to_const_uint(ShadowType::COUNT), nullptr);
    _renderTargets[to_uint(RenderTargetID::REFLECTION)].resize(Config::MAX_REFLECTIVE_NODES_IN_VIEW, nullptr);
    _renderTargets[to_uint(RenderTargetID::REFRACTION)].resize(Config::MAX_REFRACTIVE_NODES_IN_VIEW, nullptr);
    _renderTargets[to_uint(RenderTargetID::ENVIRONMENT)].resize(1, nullptr);
    _renderTargets[to_uint(RenderTargetID::OTHER)].resize(g_maxAdditionalRenderTargets, nullptr);
}

GFXRTPool::~GFXRTPool()
{
    clear();
}

void GFXRTPool::clear() {
    // Delete all of our rendering targets
    for (U32 i = 0; i < to_const_uint(RenderTargetID::COUNT); ++i) {
        for (U32 j = 0; j < _renderTargets[i].size(); ++j) {
            set(static_cast<RenderTargetID>(i), j, nullptr);
        }
        _renderTargets[i].clear();
    }
}

void GFXRTPool::swap(RenderTargetID lhs, RenderTargetID rhs) {
    std::swap(_renderTargets[to_uint(lhs)], _renderTargets[to_uint(rhs)]);
}


void GFXRTPool::set(RenderTargetID target, U32 index, RenderTarget* newTarget) {
    RenderTarget*& existingTarget = _renderTargets[to_uint(target)][index];
    if (existingTarget != nullptr) {
        MemoryManager::DELETE(existingTarget);
    }

    existingTarget = newTarget;
}

RenderTargetHandle GFXRTPool::add(RenderTargetID targetID, RenderTarget* newTarget) {
    vectorImpl<RenderTarget*>& rts = _renderTargets[to_uint(targetID)];

    for (U32 i = 0; i < rts.size(); ++i) {
        if (rts[i] == nullptr) {
            rts[i] = newTarget;
            return RenderTargetHandle(targetID, i, newTarget);
        }
    }

    DIVIDE_UNEXPECTED_CALL("No more render targets available!");
    return RenderTargetHandle();
}

bool GFXRTPool::remove(RenderTargetHandle& handle) {
    bool state = false;
    if (handle._targetID != RenderTargetID::COUNT) {
        set(handle._targetID, handle._targetIndex, nullptr);
    } else {
        state = handle._rt == nullptr;
    }

    handle = RenderTargetHandle();
    return state;
}

}; //namespace Divide