#include "stdafx.h"

#include "Headers/Editor.h"
#include "Headers/Sample.h"
#include "Editor/Widgets/Headers/ImWindowManagerDivide.h"

#include "Managers/Headers/FrameListenerManager.h"

namespace Divide {

Editor::Editor(PlatformContext& context)
    : _context(context),
      _windowManager(std::make_unique<ImwWindowManagerDivide>(context)),
      _editorTimer(Time::ADD_TIMER("Main Editor Timer")),
      _editorUpdateTimer(Time::ADD_TIMER("Editor Update Timer")),
      _editorRenderTimer(Time::ADD_TIMER("Editor Render Timer"))
{
    _editorTimer.addChildTimer(_editorUpdateTimer);
    _editorTimer.addChildTimer(_editorRenderTimer);
    REGISTER_FRAME_LISTENER(this, 99999);
}

Editor::~Editor()
{
    UNREGISTER_FRAME_LISTENER(this);
    close();
}

bool Editor::init() {
    if (_windowManager->Init()) {
        InitSample();
        return true;
    }
    return false;
}

void Editor::close() {
    ImGui::Shutdown();
}

void Editor::update(const U64 deltaTimeUS) {
    ACKNOWLEDGE_UNUSED(deltaTimeUS);
    Time::ScopedTimer timer(_editorUpdateTimer);
    if (_windowManager->Run(false)) {

    }
}

bool Editor::frameStarted(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    //ImGui::NewFrame();
    return true;
}

bool Editor::framePreRenderStarted(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    return true;
}

bool Editor::framePreRenderEnded(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    return true;
}

bool Editor::frameRenderingQueued(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    return true;
}

bool Editor::framePostRenderStarted(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    return true;
}

bool Editor::framePostRenderEnded(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);
    Time::ScopedTimer timer(_editorRenderTimer);
    // Render ImWindow stuff
    ImGui::GetIO().DeltaTime = Time::MicrosecondsToSeconds<float>(evt._timeSinceLastFrameUS);
    return _windowManager->Run(true);
}

bool Editor::frameEnded(const FrameEvent& evt) {
    ACKNOWLEDGE_UNUSED(evt);

    return true;
}

}; //namespace Divide
