/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _PLATFORM_CONTEXT_H_
#define _PLATFORM_CONTEXT_H_

namespace Divide {

class GUI;
class Kernel;
class Server;
class Editor;
class GFXDevice;
class SFXDevice;
class PXDevice;
class Application;
class LocalClient;
class XMLEntryData;
class ParamHandler;
class DisplayWindow;
class DebugInterface;

struct Configuration;

namespace Attorney {
    class PlatformContextKernel;
};

namespace Input {
    class InputHandler;
};

enum class TaskPoolType : U8 {
    HIGH_PRIORITY = 0,
    LOW_PRIORITY,
    COUNT
};

class PlatformContext {
    friend class Attorney::PlatformContextKernel;

 public:
    enum class SystemComponentType : U32 {
        Application = 1 << 1,
        GFXDevice = 1 << 2,
        SFXDevice = 1 << 3,
        PXDevice = 1 << 4,
        GUI = 1 << 5,
        XMLData = 1 << 6,
        Configuration = 1 << 7,
        LocalClient = 1 << 8,
        DebugInterface = 1 << 9,
        Editor = 1 << 10,
        InputHandler = 1 << 11,
        COUNT = 11,
        ALL = Application | GFXDevice | SFXDevice | PXDevice | GUI | XMLData |
              Configuration | LocalClient | DebugInterface | Editor | InputHandler
    };

 public:
    explicit PlatformContext(Application& app, Kernel& kernel);
    ~PlatformContext();

    void beginFrame(U32 mask = to_base(SystemComponentType::ALL));
    void idle(U32 mask = to_base(SystemComponentType::ALL));
    void endFrame(U32 mask = to_base(SystemComponentType::ALL));

    inline void beginFrame(SystemComponentType component) { beginFrame(to_base(component)); }
    inline void idle(SystemComponentType component) { idle(to_base(component)); }
    inline void endFrame(SystemComponentType component) { endFrame(to_base(component)); }

    void terminate();

    inline [[nodiscard]] Application& app()  noexcept { return _app; }
    inline [[nodiscard]] const Application& app() const noexcept { return _app; }

    inline [[nodiscard]] GFXDevice& gfx() noexcept { return *_gfx; }
    inline [[nodiscard]] const GFXDevice& gfx() const noexcept { return *_gfx; }

    inline [[nodiscard]] GUI& gui() noexcept { return *_gui; }
    inline [[nodiscard]] const GUI& gui() const noexcept { return *_gui; }

    inline [[nodiscard]] SFXDevice& sfx() noexcept { return *_sfx; }
    inline [[nodiscard]] const SFXDevice& sfx() const noexcept { return *_sfx; }

    inline [[nodiscard]] PXDevice& pfx() noexcept { return *_pfx; }
    inline [[nodiscard]] const PXDevice& pfx() const noexcept { return *_pfx; }

    inline [[nodiscard]] XMLEntryData& entryData() noexcept { return *_entryData; }
    inline [[nodiscard]] const XMLEntryData& entryData() const noexcept { return *_entryData; }

    inline [[nodiscard]] Configuration& config() noexcept { return *_config; }
    inline [[nodiscard]] const Configuration& config() const noexcept { return *_config; }

    inline [[nodiscard]] LocalClient& client() noexcept { return *_client; }
    inline [[nodiscard]] const LocalClient& client() const noexcept { return *_client; }

    inline [[nodiscard]] Server& server() noexcept { return *_server; }
    inline [[nodiscard]] const Server& server() const noexcept { return *_server; }

    inline [[nodiscard]] DebugInterface& debug() noexcept { return *_debug; }
    inline [[nodiscard]] const DebugInterface& debug() const noexcept { return *_debug; }

    inline [[nodiscard]] Editor& editor() noexcept { return *_editor; }
    inline [[nodiscard]] const Editor& editor() const noexcept { return *_editor; }

    inline [[nodiscard]] TaskPool& taskPool(TaskPoolType type) noexcept {return *_taskPool[to_base(type)]; }
    inline [[nodiscard]] const TaskPool& taskPool(TaskPoolType type) const noexcept { return *_taskPool[to_base(type)]; }

    inline [[nodiscard]] Input::InputHandler& input() noexcept { return *_inputHandler; }
    inline [[nodiscard]] const Input::InputHandler& input() const noexcept { return *_inputHandler; }

    inline [[nodiscard]] ParamHandler& paramHandler() noexcept { return *_paramHandler; }
    inline [[nodiscard]] const ParamHandler& paramHandler() const noexcept { return *_paramHandler; }

    [[nodiscard]] Kernel& kernel();
    [[nodiscard]] const Kernel& kernel() const;

    [[nodiscard]] DisplayWindow& mainWindow();
    [[nodiscard]] const DisplayWindow& mainWindow() const;

  protected:
    void onThreadCreated(const std::thread::id& threadID) const;

  private:
    /// Main application instance
    Application& _app;
    /// Main app's kernel
    Kernel& _kernel;

    /// Task pools
    std::array<TaskPool*, to_base(TaskPoolType::COUNT)> _taskPool;
    /// Access to the GPU
    GFXDevice* _gfx = nullptr;
    /// The graphical user interface
    GUI* _gui = nullptr;
    /// Access to the audio device
    SFXDevice* _sfx = nullptr;
    /// Access to the physics system
    PXDevice* _pfx = nullptr;
    /// XML configuration data
    XMLEntryData* _entryData = nullptr;
    Configuration* _config = nullptr;
    /// Networking client
    LocalClient* _client = nullptr;
    /// Networking server
    Server* _server = nullptr;
    /// Debugging interface: read only / editable variables
    DebugInterface* _debug = nullptr;
    /// Game editor
    Editor* _editor = nullptr;
    /// Input handler
    Input::InputHandler* _inputHandler = nullptr;
    /// Param handler
    ParamHandler* _paramHandler = nullptr;
};

namespace Attorney {
    class PlatformContextKernel {
    private:
        static void onThreadCreated(PlatformContext& context, const std::thread::id& threadID) {
            context.onThreadCreated(threadID);
        }

        friend class Divide::Kernel;
    };
};  // namespace Attorney

}; //namespace Divide

#endif //_PLATFORM_CONTEXT_H_
