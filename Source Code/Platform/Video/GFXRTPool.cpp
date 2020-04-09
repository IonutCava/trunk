#include "stdafx.h"

#include "Headers/GFXRTPool.h"
#include "Headers/GFXDevice.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/Configuration.h"

#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"

namespace Divide {

namespace {
    // Used to delete resources
    struct DeleteRT {
        void operator()(RenderTarget* res) {
            if (res != nullptr) {
                res->destroy();
            }
        }
    };

    U32 g_maxAdditionalRenderTargets = 128;
};

GFXRTPool::GFXRTPool(GFXDevice& parent)
    : _parent(parent)
{
    if_constexpr(Config::Build::ENABLE_EDITOR) {
        _renderTargets[to_U32(RenderTargetUsage::EDITOR)].resize(1, nullptr);
    }

    _renderTargets[to_U32(RenderTargetUsage::SCREEN)].resize(1, nullptr);
    _renderTargets[to_U32(RenderTargetUsage::SCREEN_MS)].resize(1, nullptr);

    _renderTargets[to_U32(RenderTargetUsage::OIT)].resize(1, nullptr);
    _renderTargets[to_U32(RenderTargetUsage::OIT_MS)].resize(1, nullptr);
    _renderTargets[to_U32(RenderTargetUsage::OIT_REFLECT)].resize(1, nullptr);
    
    _renderTargets[to_U32(RenderTargetUsage::HI_Z)].resize(1, nullptr);
    _renderTargets[to_U32(RenderTargetUsage::HI_Z_REFLECT)].resize(1, nullptr);

    _renderTargets[to_U32(RenderTargetUsage::SHADOW)].resize(to_base(ShadowType::COUNT), nullptr);

    _renderTargets[to_U32(RenderTargetUsage::REFLECTION_PLANAR)].resize(Config::MAX_REFLECTIVE_NODES_IN_VIEW, nullptr);
    _renderTargets[to_U32(RenderTargetUsage::REFRACTION_PLANAR)].resize(Config::MAX_REFRACTIVE_NODES_IN_VIEW, nullptr);
    _renderTargets[to_U32(RenderTargetUsage::REFLECTION_CUBE)].resize(1, nullptr);

    _renderTargets[to_U32(RenderTargetUsage::REFLECTION_PLANAR_BLUR)].resize(1, nullptr);

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
        for (U16 j = 0; j < to_U16(_renderTargets[i].size()); ++j) {
            set(RenderTargetID(static_cast<RenderTargetUsage>(i), j), nullptr);
        }
    }
}

void GFXRTPool::resizeTargets(RenderTargetUsage target, U16 width, U16 height) {
    for (const std::shared_ptr<RenderTarget>& rt : _renderTargets[to_U32(target)]) {
        if (rt) {
            rt->resize(width, height);
        }
    }
}

void GFXRTPool::updateSampleCount(RenderTargetUsage target, U8 sampleCount) {
    for (const std::shared_ptr<RenderTarget>& rt : _renderTargets[to_U32(target)]) {
        if (rt) {
            rt->updateSampleCount(sampleCount);
        }
    }
}

void GFXRTPool::set(RenderTargetID target, const std::shared_ptr<RenderTarget>& newTarget) {
    _renderTargets[to_U32(target._usage)][target._index] = newTarget;
}

RenderTargetHandle GFXRTPool::add(RenderTargetUsage targetUsage, const std::shared_ptr<RenderTarget>& newTarget) {
    vectorSTD<std::shared_ptr<RenderTarget>>& rts = _renderTargets[to_U32(targetUsage)];

    for (U16 i = 0; i < to_U16(rts.size()); ++i) {
        if (rts[i] == nullptr) {
            rts[i] = newTarget;
            return RenderTargetHandle(RenderTargetID(targetUsage, i), newTarget.get());
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
    std::shared_ptr<RenderTarget> rt(Attorney::GFXDeviceGFXRTPool::newRT(_parent, descriptor), DeleteRT());
    return add(targetUsage, rt);
}

const RenderTargetID GFXRTPool::screenTargetID() const noexcept {
    const RenderTargetUsage screenRT = _parent.context().config().rendering.MSAAsamples > 0 ? RenderTargetUsage::SCREEN_MS : RenderTargetUsage::SCREEN;
    return RenderTargetID(screenRT);
}

const RenderTarget& GFXRTPool::screenTarget() const noexcept {
    return renderTarget(screenTargetID());
}

}; //namespace Divide