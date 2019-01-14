#include "stdafx.h"

#include "Headers/VKWrapper.h"

namespace Divide {
    VK_API::VK_API(GFXDevice& context)
    {
        ACKNOWLEDGE_UNUSED(context);
    }

    VK_API::~VK_API()
    {
    }

    void VK_API::idle() {

    }

    void VK_API::beginFrame(DisplayWindow& window, bool global) {
        ACKNOWLEDGE_UNUSED(window);
        ACKNOWLEDGE_UNUSED(global);
    }

    void VK_API::endFrame(DisplayWindow& window, bool global) {
        ACKNOWLEDGE_UNUSED(window);
        ACKNOWLEDGE_UNUSED(global);
    }

    ErrorCode VK_API::initRenderingAPI(I32 argc, char** argv, Configuration& config) {
        ACKNOWLEDGE_UNUSED(argc);
        ACKNOWLEDGE_UNUSED(argv);
        ACKNOWLEDGE_UNUSED(config);

        return ErrorCode::NO_ERR;
    }

    void VK_API::closeRenderingAPI() {
    }

    void VK_API::updateClipPlanes(const FrustumClipPlanes& list) {
        ACKNOWLEDGE_UNUSED(list);
    }

    F32 VK_API::getFrameDurationGPU() const {
        return 0.f;
    }

    size_t VK_API::setStateBlock(size_t stateBlockHash) {
        ACKNOWLEDGE_UNUSED(stateBlockHash);

        return 0;
    }

    void VK_API::flushCommand(const GFX::CommandBuffer::CommandEntry& entry, const GFX::CommandBuffer& commandBuffer) {
        ACKNOWLEDGE_UNUSED(entry);
        ACKNOWLEDGE_UNUSED(commandBuffer);
    }

    void VK_API::postFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
        ACKNOWLEDGE_UNUSED(commandBuffer);
    }

    vec2<U16> VK_API::getDrawableSize(const DisplayWindow& window) const {
        ACKNOWLEDGE_UNUSED(window);

        return vec2<U16>(1);
    }

    U32 VK_API::getHandleFromCEGUITexture(const CEGUI::Texture& textureIn) const {
        ACKNOWLEDGE_UNUSED(textureIn);

        return 0u;
    }

    bool VK_API::setViewport(const Rect<I32>& newViewport) {
        ACKNOWLEDGE_UNUSED(newViewport);

        return true;
    }

    void VK_API::onThreadCreated(const std::thread::id& threadID) {
        ACKNOWLEDGE_UNUSED(threadID);
    }
}; //namespace Divide