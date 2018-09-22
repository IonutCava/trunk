#include "stdafx.h"

#include "Headers/NoneWrapper.h"

namespace Divide {
    NONE_API::NONE_API(GFXDevice& context)
    {
    }

    NONE_API::~NONE_API()
    {
    }

    void NONE_API::beginFrame() {}
    void NONE_API::endFrame() {}
    ErrorCode NONE_API::initRenderingAPI(I32 argc, char** argv, Configuration& config) { return ErrorCode::NO_ERR;  }
    void NONE_API::closeRenderingAPI() {}
    void NONE_API::updateClipPlanes(const FrustumClipPlanes& list) {}
    F32 NONE_API::getFrameDurationGPU() const { return 0.f;  }
    size_t NONE_API::setStateBlock(size_t stateBlockHash) { return 0; }
    void NONE_API::flushCommand(const GFX::CommandBuffer::CommandEntry& entry, const GFX::CommandBuffer& commandBuffer) {}
    void NONE_API::postFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer) {}
    vec2<U16> NONE_API::getDrawableSize(const DisplayWindow& window) const { return vec2<U16>(1);  }
    U32 NONE_API::getHandleFromCEGUITexture(const CEGUI::Texture& textureIn) const { return 0u;  }
    bool NONE_API::setViewport(const Rect<I32>& newViewport) { return true; }
}; //namespace Divide