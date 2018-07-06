#include "Headers/PlatformContext.h"

#include "GUI/Headers/GUI.h"
#include "Physics/Headers/PXDevice.h"
#include "Platform/Audio/Headers/SFXDevice.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Input/Headers/InputInterface.h"

namespace Divide {

PlatformContext::PlatformContext(std::unique_ptr<GFXDevice> gfx,
                                 std::unique_ptr<SFXDevice> sfx,
                                 std::unique_ptr<PXDevice> pfx,
                                 std::unique_ptr<GUI> gui,
                                 std::unique_ptr<Input::InputInterface> input)
  : _gfx(std::move(gfx)),
    _sfx(std::move(sfx)),
    _pfx(std::move(pfx)),
    _gui(std::move(gui)),
    _input(std::move(input))
{
}

PlatformContext::~PlatformContext()
{
}

void PlatformContext::idle() {
    _gfx->idle();
    //_sfx->idle();
    _pfx->idle();
    //_gui->idle();
    //_input->idle();
}

}; //namespace Divide