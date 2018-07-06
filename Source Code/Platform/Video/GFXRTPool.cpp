#include "Headers/GFXRTPool.h"
#include "Headers/GFXDevice.h"

#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"

namespace Divide {

namespace {
    U32 g_maxAdditionalRenderTargets = 128;
};

GFXRTPool::GFXRTPool(GFXDevice& parent)
    : _parent(parent),
      _poolSize(0),
      _poolIndex(0)
{
    STUBBED("ToDo: Change this to only keep frameHistory per player to reduce VRAM usage! "
            "No need to store per-frame render targets as they get overriden anyway. -Ionut");

    resize(1);
}

GFXRTPool::~GFXRTPool()
{
    clear();
}

void GFXRTPool::clear() {
    // Delete all of our rendering targets
    for (U8 i = 0; i < _poolSize; ++i) {
        poolIndex(i);
        for (U8 j = 0; j < to_const_ubyte(RenderTargetUsage::COUNT); ++j) {
            for (U32 k = 0; k < to_uint(_renderTargets[i][j].size()); ++k) {
                set(RenderTargetID(static_cast<RenderTargetUsage>(j), k), nullptr);
            }
        }
    }
    _renderTargets.clear();

    resize(1);
}

void GFXRTPool::duplicateTargets(U8 targetIndex, const RenderTargets& src, RenderTargets& dest) {
    for (U8 i = 0; i < to_const_ubyte(RenderTargetUsage::COUNT); ++i) {
        const TargetsPerUsage& srcTargets = src[i];
        TargetsPerUsage& destTargets = dest[i];

        for (U8 j = 0; j < src[i].size(); ++j) {
            RenderTarget* srcRT = srcTargets[j];
            RenderTarget*& destRT = destTargets[j];

            if (srcRT != nullptr) {
                if (destRT == nullptr) {
                    destRT = _parent.newRT(Util::StringFormat("%s_%d", srcRT->getName().c_str(), targetIndex));
                }
                destRT->copy(*srcRT);
            }
        }
    }
}

void GFXRTPool::resize(U8 size) {
    if (size == 0) {
        return;
    }

    while (_poolSize < size) {
        _renderTargets.emplace_back();
        
        for (U32 i = 0; i < to_const_uint(RenderTargetUsage::COUNT); ++i) {
            RenderTargets& targets = _renderTargets.back();
            targets[to_uint(RenderTargetUsage::SCREEN)].resize(1, nullptr);
            targets[to_uint(RenderTargetUsage::SHADOW)].resize(to_const_uint(ShadowType::COUNT), nullptr);
            targets[to_uint(RenderTargetUsage::REFLECTION)].resize(Config::MAX_REFLECTIVE_NODES_IN_VIEW, nullptr);
            targets[to_uint(RenderTargetUsage::REFRACTION)].resize(Config::MAX_REFRACTIVE_NODES_IN_VIEW, nullptr);
            targets[to_uint(RenderTargetUsage::ENVIRONMENT)].resize(1, nullptr);
            targets[to_uint(RenderTargetUsage::OTHER)].resize(g_maxAdditionalRenderTargets, nullptr);
        }
        if (++_poolSize > 1) {
            duplicateTargets(_poolSize, _renderTargets.front(), _renderTargets.back());
        }
    }

    while (_poolSize > size) {
        RenderTargets& targets = _renderTargets.back();
        for (U32 j = 0; j < to_const_uint(RenderTargetUsage::COUNT); ++j) {
            for (U32 k = 0; k < to_uint(targets[j].size()); ++k) {
                set(RenderTargetID(static_cast<RenderTargetUsage>(j), k), nullptr);
            }
        }
        _renderTargets.pop_back();

        --_poolSize;
        if (_poolIndex == _poolSize) {
            poolIndex(_poolIndex - 1);
        }
    }
}

void GFXRTPool::resizeTargets(RenderTargetUsage target, U16 width, U16 height) {
    for (RenderTargets& targets : _renderTargets) {
        for (RenderTarget* rt : targets[to_uint(target)]) {
            if (rt) {
                rt->create(width, height);
            }
        }
    }
}

void GFXRTPool::set(RenderTargetID target, RenderTarget* newTarget) {
    RenderTarget*& oldTarget = _renderTargets[_poolIndex][to_uint(target._usage)][target._index];
    if (oldTarget) {
        //overriding old target
    }

    oldTarget = newTarget;
}

RenderTargetHandle GFXRTPool::add(RenderTargetUsage targetUsage, RenderTarget* newTarget) {
    vectorImpl<RenderTarget*>& rts = _renderTargets[_poolIndex][to_uint(targetUsage)];

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