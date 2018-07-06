#include "Headers/DXWrapper.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Utility/Headers/Localization.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

namespace Divide {

ErrorCode DX_API::initRenderingAPI(I32 argc, char** argv, const Configuration& config) {
    Console::printfn(Locale::get(_ID("START_D3D_API")));
    fillEnumTables();
    // CEGUI::System::create(CEGUI::Direct3D10Renderer::create(
    // /*myD3D10Device*/nullptr ));
    return ErrorCode::DX_INIT_ERROR;
}

void DX_API::closeRenderingAPI() {}

void DX_API::changeViewport(const vec4<I32>& newViewport) const {}

void DX_API::registerCommandBuffer(const ShaderBuffer& commandBuffer) const {
    ACKNOWLEDGE_UNUSED(commandBuffer);
}

void DX_API::beginFrame() {}

void DX_API::endFrame(bool swapBuffers) {
    ACKNOWLEDGE_UNUSED(swapBuffers);
}

void DX_API::updateClipPlanes() {}

void DX_API::drawText(const vectorImpl<GUITextBatchEntry>& batch) {
    ACKNOWLEDGE_UNUSED(batch);
}

bool DX_API::draw(const GenericDrawCommand& cmd) { 
    ACKNOWLEDGE_UNUSED(cmd);
    return false;
}

void DX_API::flushCommandBuffer(const CommandBuffer& commandBuffer) {
    ACKNOWLEDGE_UNUSED(commandBuffer);
}

void DX_API::syncToThread(const std::thread::id& threadID) {
    ACKNOWLEDGE_UNUSED(threadID);
}

size_t DX_API::setStateBlock(size_t stateBlockHash) {
    ACKNOWLEDGE_UNUSED(stateBlockHash);
    return 0;
}

void DX_API::pushDebugMessage(const char* message, I32 id) {
    ACKNOWLEDGE_UNUSED(message);
    ACKNOWLEDGE_UNUSED(id);
}

void DX_API::popDebugMessage() {

}

};