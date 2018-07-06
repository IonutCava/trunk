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

ErrorCode DX_API::initRenderingAPI(I32 argc, char** argv) {
    Console::printfn(Locale::get(_ID("START_D3D_API")));
    fillEnumTables();
    // CEGUI::System::create(CEGUI::Direct3D10Renderer::create(
    // /*myD3D10Device*/nullptr ));
    return ErrorCode::DX_INIT_ERROR;
}

void DX_API::closeRenderingAPI() {}

void DX_API::changeViewport(const vec4<I32>& newViewport) const {}

void DX_API::registerCommandBuffer(const ShaderBuffer& commandBuffer) const {}

bool DX_API::makeTexturesResident(const TextureDataContainer& textureData) {
    return true;
}

bool DX_API::makeTextureResident(const TextureData& textureData) {
    return true;
}

void DX_API::beginFrame() {}

void DX_API::endFrame(bool swapBuffers) {}

void DX_API::toggleDepthWrites(bool state) {}
void DX_API::toggleRasterization(bool state) {}

void DX_API::updateClipPlanes() {}

void DX_API::drawText(const TextLabel& textLabel, const vec2<F32>& position) {}

void DX_API::draw(const GenericDrawCommand& cmd) {}

void DX_API::syncToThread(std::thread::id threadID) {}

void DX_API::activateStateBlock(const RenderStateBlock& newBlock,
                                const RenderStateBlock& oldBlock) const {}
};