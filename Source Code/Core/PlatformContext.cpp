#include "stdafx.h"

#include "Headers/PlatformContext.h"
#include "Headers/XMLEntryData.h"
#include "Headers/Configuration.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/TaskPool.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

#include "GUI/Headers/GUI.h"
#include "Editor/Headers/Editor.h"
#include "Physics/Headers/PXDevice.h"
#include "Core/Networking/Headers/LocalClient.h"
#include "Core/Debugging/Headers/DebugInterface.h"
#include "Platform/Audio/Headers/SFXDevice.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Input/Headers/InputHandler.h"

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
    _taskPool = std::make_unique<TaskPool>();
    _gfx = std::make_unique<GFXDevice>(_kernel);        // Video
    _sfx = std::make_unique<SFXDevice>(_kernel);        // Audio
    _pfx = std::make_unique<PXDevice>(_kernel);         // Physics
    _gui = std::make_unique<GUI>(_kernel);              // Graphical User Interface
    _entryData = std::make_unique<XMLEntryData>();      // Initial XML data
    _config = std::make_unique<Configuration>();        // XML based configuration
    _client = std::make_unique<LocalClient>(_kernel);   // Network client
    _debug = std::make_unique<DebugInterface>(_kernel); // Debug Interface
    _inputHandler = std::make_unique<Input::InputHandler>(_kernel, _app);

    if (Config::Build::ENABLE_EDITOR) {
        _editor = std::make_unique<Editor>(*this);
    }
}

void PlatformContext::terminate() {
    _taskPool->shutdown();
    _taskPool.reset();
    _editor.reset();
    _inputHandler.reset();
    _entryData.reset();
    _config.reset();
    _client.reset();
    _debug.reset();
    _gui->destroy(); 
    _gui.reset();

    Console::printfn(Locale::get(_ID("STOP_PHYSICS_INTERFACE")));
    _pfx->closePhysicsAPI();
    _pfx.reset();

    Console::printfn(Locale::get(_ID("STOP_HARDWARE")));
    _sfx->closeAudioAPI();
    _sfx.reset();

    _gfx->closeRenderingAPI();
    _gfx.reset();
}

void PlatformContext::beginFrame() {
    _gfx->beginFrame(app().windowManager().getMainWindow(), true);
    _sfx->beginFrame();
}

void PlatformContext::idle() {
    _taskPool->flushCallbackQueue();

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
    _gfx->endFrame(app().windowManager().getMainWindow(), true);
    _sfx->endFrame();
}

DisplayWindow& PlatformContext::activeWindow() {
    return app().windowManager().getMainWindow();
}

Kernel& PlatformContext::kernel() {
    return _kernel;
}

const Kernel& PlatformContext::kernel() const {
    return _kernel;
}

void PlatformContext::onThreadCreated(const std::thread::id& threadID) {
    _gfx->onThreadCreated(threadID);
}

}; //namespace Divide