#include "stdafx.h"

#include "Headers/DXWrapper.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Utility/Headers/Localization.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Predefined/Headers/Box3D.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Geometry/Shapes/Predefined/Headers/Text3D.h"

namespace Divide {

ErrorCode DX_API::initRenderingAPI(I32 argc, char** argv, Configuration& config) {
    Console::printfn(Locale::get(_ID("START_D3D_API")));
    fillEnumTables();
    // CEGUI::System::create(CEGUI::Direct3D10Renderer::create(
    // /*myD3D10Device*/nullptr ));
    return ErrorCode::DX_INIT_ERROR;
}

void DX_API::closeRenderingAPI() {}

void DX_API::registerCommandBuffer(const ShaderBuffer& commandBuffer) const {
    ACKNOWLEDGE_UNUSED(commandBuffer);
}

void DX_API::beginFrame() {}

void DX_API::endFrame() {}

void DX_API::updateClipPlanes() {}

void DX_API::flushCommandBuffer(GFX::CommandBuffer& commandBuffer) {
    ACKNOWLEDGE_UNUSED(commandBuffer);
}

size_t DX_API::setStateBlock(size_t stateBlockHash) {
    ACKNOWLEDGE_UNUSED(stateBlockHash);
    return 0;
}

bool DX_API::changeViewportInternal(const vec4<I32>& viewport) {
    return true;
}
};