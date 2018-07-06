#include "stdafx.h"

#include "Headers/PlatformContext.h"
#include "Headers/XMLEntryData.h"
#include "Headers/Configuration.h"

#include "GUI/Headers/GUI.h"
#include "Editor/Headers/Editor.h"
#include "Physics/Headers/PXDevice.h"
#include "Core/Networking/Headers/LocalClient.h"
#include "Core/Debugging/Headers/DebugInterface.h"
#include "Platform/Audio/Headers/SFXDevice.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Input/Headers/InputInterface.h"

namespace Divide {

PlatformContext::PlatformContext(Application& app, Kernel& kernel)
  : _app(app),
    _kernel(kernel)
{
}

PlatformContext::~PlatformContext()
{
}

void PlatformContext::init() {
    _gfx = std::make_unique<GFXDevice>(_kernel);        // Video
    _sfx = std::make_unique<SFXDevice>(_kernel);        // Audio
    _pfx = std::make_unique<PXDevice>(_kernel);         // Physics
    _gui = std::make_unique<GUI>(_kernel);              // Graphical User Interface
    _entryData = std::make_unique<XMLEntryData>();      // Initial XML data
    _config = std::make_unique<Configuration>();        // XML based configuration
    _client = std::make_unique<LocalClient>(_kernel);   // Network client
    _debug = std::make_unique<DebugInterface>(_kernel); // Debug Interface
    if (Config::Build::ENABLE_EDITOR) {
        _editor = std::make_unique<Editor>(*this);
    }
}

void PlatformContext::terminate() {
    _gfx.reset();
    _sfx.reset();
    _pfx.reset();
    _gui.reset();
    _entryData.reset();
    _config.reset();
    _client.reset();
    _debug.reset();
    _editor.reset();
}

void PlatformContext::beginFrame() {
    _gfx->beginFrame();
    _sfx->beginFrame();
}

void PlatformContext::idle() {
    _app.idle();
    _gfx->idle();
    //_sfx->idle();
    _pfx->idle();
    //_gui->idle();
    _debug->idle();
    if (Config::Build::ENABLE_EDITOR) {
        _editor->idle();
    }
}

void PlatformContext::endFrame() {
    _gfx->endFrame();
    _sfx->endFrame();
}

Input::InputInterface& PlatformContext::input() {
    return activeWindow().inputHandler();
}

DisplayWindow& PlatformContext::activeWindow() {
    return app().windowManager().getActiveWindow();
}

Kernel& PlatformContext::kernel() {
    return _kernel;
}

}; //namespace Divide