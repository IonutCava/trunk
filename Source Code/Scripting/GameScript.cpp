#include "Headers/GameScript.h"
#include "Headers/ScriptBindings.h"

#include "Managers/Headers/FrameListenerManager.h"

namespace Divide {

GameScript::GameScript(const stringImpl& sourceCode)
    : Script(sourceCode),
      FrameListener()
{
    REGISTER_FRAME_LISTENER(this, 1);

    _script.add(create_chaiscript_bindings());
}

GameScript::GameScript(const stringImpl& scriptPath, FileType fileType)
    : Script(scriptPath, fileType),
      FrameListener()
{
    REGISTER_FRAME_LISTENER(this, 1);

    _script.add(create_chaiscript_bindings());
}

GameScript::~GameScript()
{
    UNREGISTER_FRAME_LISTENER(this);
}

bool GameScript::frameStarted(const FrameEvent& evt) {
    return true;
}

bool GameScript::framePreRenderStarted(const FrameEvent& evt) {
    return true;
}

bool GameScript::framePreRenderEnded(const FrameEvent& evt) {
    return true;
}

bool GameScript::frameRenderingQueued(const FrameEvent& evt) {
    return true;
}

bool GameScript::framePostRenderStarted(const FrameEvent& evt) {
    return true;
}

bool GameScript::framePostRenderEnded(const FrameEvent& evt) {
    return true;
}

bool GameScript::frameEnded(const FrameEvent& evt) {
    return true;
}

}; //namespace Divide
