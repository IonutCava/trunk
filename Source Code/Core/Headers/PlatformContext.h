/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _PLATFORM_CONTEXT_H_
#define _PLATFORM_CONTEXT_H_

#include <memory>

namespace Divide {

class GUI;
class Kernel;
class Editor;
class GFXDevice;
class SFXDevice;
class PXDevice;
class Application;
class LocalClient;
class XMLEntryData;
class Configuration;
class DisplayWindow;
class DebugInterface;

namespace Input {
    class InputInterface;
};

class PlatformContext {
public:
    explicit PlatformContext(Application& app, Kernel& kernel);
    ~PlatformContext();

    void beginFrame();
    void idle();
    void endFrame();
    void init();
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

    Kernel& kernel();
    DisplayWindow& activeWindow();
    Input::InputInterface& input();

    private:
    /// Main application instance
    Application& _app;
    /// Main app's kernel
    Kernel& _kernel;
    /// Access to the GPU
    std::unique_ptr<GFXDevice> _gfx;
    /// The graphical user interface
    std::unique_ptr<GUI> _gui;
    /// Access to the audio device
    std::unique_ptr<SFXDevice> _sfx;
    /// Access to the physics system
    std::unique_ptr<PXDevice> _pfx;
    /// XML configuration data
    std::unique_ptr<XMLEntryData>  _entryData;
    std::unique_ptr<Configuration> _config;
    /// Networking client
    std::unique_ptr<LocalClient> _client;
    /// Debugging interface: read only / editable variables
    std::unique_ptr<DebugInterface> _debug;
    /// Game editor
    std::unique_ptr<Editor> _editor;
};
}; //namespace Divide

#endif //_PLATFORM_CONTEXT_H_
