#include "stdafx.h"

#include "Headers/VKWrapper.h"

namespace Divide {
    VK_API::VK_API(GFXDevice& context)
    {
    }

    VK_API::~VK_API()
    {
    }

    void VK_API::beginFrame() {}
    void VK_API::endFrame() {}
    ErrorCode VK_API::initRenderingAPI(I32 argc, char** argv, Configuration& config) { return ErrorCode::NO_ERR;  }
    void VK_API::closeRenderingAPI() {}
    void VK_API::updateClipPlanes(const FrustumClipPlanes& list) {}
    F32 VK_API::getFrameDurationGPU() const { return 0.f;  }
    size_t VK_API::setStateBlock(size_t stateBlockHash) { return 0; }
    void VK_API::flushCommand(const GFX::CommandBuffer::CommandEntry& entry, const GFX::CommandBuffer& commandBuffer) {}
    void VK_API::postFlushCommand(const GFX::CommandBuffer::CommandEntry& entry, const GFX::CommandBuffer& commandBuffer) {}
    vec2<U16> VK_API::getDrawableSize(const DisplayWindow& window) const { return vec2<U16>(1);  }
    U32 VK_API::getHandleFromCEGUITexture(const CEGUI::Texture& textureIn) const { return 0u;  }
    bool VK_API::changeViewportInternal(const Rect<I32>& newViewport) { return true; }
}; //namespace Divide