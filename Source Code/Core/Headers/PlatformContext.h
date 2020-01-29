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
class Editor;
class TaskPool;
class GFXDevice;
class SFXDevice;
class PXDevice;
class Application;
class LocalClient;
class XMLEntryData;
class Configuration;
class DisplayWindow;
class DebugInterface;

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
    enum class ComponentType : U32 {
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

    void beginFrame(U32 mask = to_base(ComponentType::ALL));
    void idle(U32 mask = to_base(ComponentType::ALL));
    void endFrame(U32 mask = to_base(ComponentType::ALL));

    inline void beginFrame(ComponentType component) { beginFrame(to_base(component)); }
    inline void idle(ComponentType component) { idle(to_base(component)); }
    inline void endFrame(ComponentType component) { endFrame(to_base(component)); }

    void terminate();

    inline Application& app()  noexcept { return _app; }
    inline const Application& app() const noexcept { return _app; }

    inline GFXDevice& gfx() { return *_gfx; }
    inline const GFXDevice& gfx() const { return *_gfx; }

    inline GUI& gui() { return *_gui; }
    inline const GUI& gui() const { return *_gui; }

    inline SFXDevice& sfx() { return *_sfx; }
    inline const SFXDevice& sfx() const { return *_sfx; }

    inline PXDevice& pfx() { return *_pfx; }
    inline const PXDevice& pfx() const { return *_pfx; }

    inline XMLEntryData& entryData() { return *_entryData; }
    inline const XMLEntryData& entryData() const { return *_entryData; }

    inline Configuration& config() { return *_config; }
    inline const Configuration& config() const { return *_config; }

    inline LocalClient& client() { return *_client; }
    inline const LocalClient& client() const { return *_client; }

    inline DebugInterface& debug() { return *_debug; }
    inline const DebugInterface& debug() const { return *_debug; }

    inline Editor& editor() { return *_editor; }
    inline const Editor& editor() const { return *_editor; }

    inline TaskPool& taskPool(TaskPoolType type) {return *_taskPool[to_base(type)]; }
    inline const TaskPool& taskPool(TaskPoolType type) const { return *_taskPool[to_base(type)]; }

    inline Input::InputHandler& input() { return *_inputHandler; }
    inline const Input::InputHandler& input() const { return *_inputHandler; }

    Kernel& kernel();
    const Kernel& kernel() const;

    DisplayWindow& activeWindow();

    protected:
    void onThreadCreated(const std::thread::id& threadID);

    private:
    /// Main application instance
    Application& _app;
    /// Main app's kernel
    Kernel& _kernel;

    /// Task pools
    std::array<TaskPool*, to_base(TaskPoolType::COUNT)> _taskPool;
    /// Access to the GPU
    GFXDevice* _gfx;
    /// The graphical user interface
    GUI* _gui;
    /// Access to the audio device
    SFXDevice* _sfx;
    /// Access to the physics system
    PXDevice* _pfx;
    /// XML configuration data
    XMLEntryData* _entryData;
    Configuration* _config;
    /// Networking client
    LocalClient* _client;
    /// Debugging interface: read only / editable variables
    DebugInterface* _debug;
    /// Game editor
    Editor* _editor;
    /// Input handler
    Input::InputHandler* _inputHandler;
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
