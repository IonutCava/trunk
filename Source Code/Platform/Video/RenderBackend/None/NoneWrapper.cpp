#include "stdafx.h"

#include "Headers/NoneWrapper.h"

namespace Divide {
    NONE_API::NONE_API(GFXDevice& context)
    {
        ACKNOWLEDGE_UNUSED(context);
    }

    NONE_API::~NONE_API()
    {
    }

    void NONE_API::idle() {

    }

    void NONE_API::beginFrame(DisplayWindow& window, bool global) {
        ACKNOWLEDGE_UNUSED(window);
        ACKNOWLEDGE_UNUSED(global);
    }

    void NONE_API::endFrame(DisplayWindow& window, bool global) {
        ACKNOWLEDGE_UNUSED(window);
        ACKNOWLEDGE_UNUSED(global);
    }

    RenderAPI NONE_API::renderAPI() const {
        return RenderAPI::None;
    }

    ErrorCode NONE_API::initRenderingAPI(I32 argc, char** argv, Configuration& config) {
        ACKNOWLEDGE_UNUSED(argc);
        ACKNOWLEDGE_UNUSED(argv);
        ACKNOWLEDGE_UNUSED(config);

        return ErrorCode::NO_ERR;
    }

    void NONE_API::closeRenderingAPI() {
    }

    F32 NONE_API::getFrameDurationGPU() const {
        return 0.f;
    }

    size_t NONE_API::setStateBlock(size_t stateBlockHash) {
        ACKNOWLEDGE_UNUSED(stateBlockHash);

        return 0;
    }

    void NONE_API::flushCommand(const GFX::CommandBuffer::CommandEntry& entry, const GFX::CommandBuffer& commandBuffer) {
        ACKNOWLEDGE_UNUSED(entry);
        ACKNOWLEDGE_UNUSED(commandBuffer);
    }

    void NONE_API::preFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
        ACKNOWLEDGE_UNUSED(commandBuffer);
    }

    void NONE_API::postFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
        ACKNOWLEDGE_UNUSED(commandBuffer);
    }

    vec2<U16> NONE_API::getDrawableSize(const DisplayWindow& window) const {
        ACKNOWLEDGE_UNUSED(window);

        return vec2<U16>(1);
    }

    U32 NONE_API::getHandleFromCEGUITexture(const CEGUI::Texture& textureIn) const {
        ACKNOWLEDGE_UNUSED(textureIn);

        return 0u;
    }

    bool NONE_API::setViewport(const Rect<I32>& newViewport) {
        ACKNOWLEDGE_UNUSED(newViewport);

        return true;
    }

    void NONE_API::onThreadCreated(const std::thread::id& threadID) {
        ACKNOWLEDGE_UNUSED(threadID);
    }

}; //namespace Divide