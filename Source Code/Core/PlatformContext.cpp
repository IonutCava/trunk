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
  :  _app(app)
  ,  _kernel(kernel)
  ,  _taskPool{}
  ,  _gfx(MemoryManager_NEW GFXDevice(_kernel))        // Video
  ,  _sfx(MemoryManager_NEW SFXDevice(_kernel))        // Audio
  ,  _pfx(MemoryManager_NEW PXDevice(_kernel))         // Physics
  ,  _gui(MemoryManager_NEW GUI(_kernel))              // Graphical User Interface
  ,  _entryData(MemoryManager_NEW XMLEntryData())      // Initial XML data
  ,  _config(MemoryManager_NEW Configuration())        // XML based configuration
  ,  _client(MemoryManager_NEW LocalClient(_kernel))   // Network client
  ,  _debug(MemoryManager_NEW DebugInterface(_kernel)) // Debug Interface
  ,  _inputHandler(MemoryManager_NEW Input::InputHandler(_kernel, _app))
  ,  _editor(Config::Build::ENABLE_EDITOR ? MemoryManager_NEW Editor(*this) : nullptr)
{
    for (U8 i = 0; i < to_U8(TaskPoolType::COUNT); ++i) {
        _taskPool[i] = MemoryManager_NEW TaskPool();
    }
}


PlatformContext::~PlatformContext()
{
    assert(_gfx == nullptr);
}

void PlatformContext::terminate() {
    for (U32 i = 0; i < to_U32(TaskPoolType::COUNT); ++i) {
        MemoryManager::DELETE(_taskPool[i]);
    }
    MemoryManager::DELETE(_editor);
    MemoryManager::DELETE(_inputHandler);
    MemoryManager::DELETE(_entryData);
    MemoryManager::DELETE(_config);
    MemoryManager::DELETE(_client);
    MemoryManager::DELETE(_debug);
    MemoryManager::DELETE(_gui);
    MemoryManager::DELETE(_pfx);

    Console::printfn(Locale::get(_ID("STOP_HARDWARE")));

    MemoryManager::DELETE(_sfx);
    MemoryManager::DELETE(_gfx);
}

void PlatformContext::beginFrame(U32 componentMask) {
    OPTICK_EVENT();

    if (BitCompare(componentMask, SystemComponentType::GFXDevice)) {
        _gfx->beginFrame(app().windowManager().getMainWindow(), true);
    }

    if (BitCompare(componentMask, SystemComponentType::SFXDevice)) {
        _sfx->beginFrame();
    }
}

void PlatformContext::idle(U32 componentMask) {
    OPTICK_EVENT();

    for (U32 i = 0; i < to_U32(TaskPoolType::COUNT); ++i) {
        _taskPool[i]->flushCallbackQueue();
    }

    if (BitCompare(componentMask, SystemComponentType::Application)) {
       
    }

    if (BitCompare(componentMask, SystemComponentType::GFXDevice)) {
        _gfx->idle();
    }
    if (BitCompare(componentMask, SystemComponentType::SFXDevice)) {
        //_sfx->idle();
    }
    if (BitCompare(componentMask, SystemComponentType::PXDevice)) {
        _pfx->idle();
    }
    if (BitCompare(componentMask, SystemComponentType::GUI)) {
        //_gui->idle();
    }
    if (BitCompare(componentMask, SystemComponentType::DebugInterface)) {
        _debug->idle();
    }

    if_constexpr(Config::Build::ENABLE_EDITOR) {
        if (BitCompare(componentMask, SystemComponentType::Editor)) {
            _editor->idle();
        }
    }
}

void PlatformContext::endFrame(U32 componentMask) {
    OPTICK_EVENT();

    if (BitCompare(componentMask, SystemComponentType::GFXDevice)) {
        _gfx->endFrame(app().windowManager().getMainWindow(), true);
    }

    if (BitCompare(componentMask, SystemComponentType::SFXDevice)) {
        _sfx->endFrame();
    }
}

DisplayWindow& PlatformContext::activeWindow() {
    return app().windowManager().getMainWindow();
}

const DisplayWindow& PlatformContext::activeWindow() const {
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