#include "stdafx.h"

#include "Headers/GameScript.h"
#include "Headers/ScriptBindings.h"

#include "Managers/Headers/FrameListenerManager.h"

namespace Divide {

GameScript::GameScript(const stringImpl& sourceCode)
    : Script(sourceCode),
      FrameListener()
{
    REGISTER_FRAME_LISTENER(this, 1);

    _script->add(create_chaiscript_bindings());
    addGameInstance();
}

GameScript::GameScript(const stringImpl& scriptPath, FileType fileType)
    : Script(scriptPath, fileType),
      FrameListener()
{
    REGISTER_FRAME_LISTENER(this, 1);

    _script->add(create_chaiscript_bindings());
    addGameInstance();
}

GameScript::~GameScript()
{
    UNREGISTER_FRAME_LISTENER(this);
}

void GameScript::addGameInstance() {
    chaiscript::ModulePtr m = chaiscript::ModulePtr(new chaiscript::Module());
    chaiscript::utility::add_class<GameScriptInstance>(*m,
        "GameScriptInstance",
        { 
            chaiscript::constructor<GameScriptInstance()>(),
            chaiscript::constructor<GameScriptInstance(const GameScriptInstance &)>()
        },
        {
            { chaiscript::fun(&GameScriptInstance::frameStarted), "frameStarted"  },
            { chaiscript::fun(&GameScriptInstance::framePreRenderStarted), "framePreRenderStarted" },
            { chaiscript::fun(&GameScriptInstance::framePreRenderEnded), "framePreRenderEnded" },
            { chaiscript::fun(&GameScriptInstance::frameRenderingQueued), "frameRenderingQueued" },
            { chaiscript::fun(&GameScriptInstance::framePostRenderStarted), "framePostRenderStarted" },
            { chaiscript::fun(&GameScriptInstance::framePostRenderEnded), "framePostRenderEnded" },
            { chaiscript::fun(&GameScriptInstance::frameEnded), "frameEnded" }
        }
    );

    _script->add(m);
}

bool GameScript::frameStarted(const FrameEvent& evt) noexcept {
    return true;
}

bool GameScript::framePreRenderStarted(const FrameEvent& evt) noexcept {
    return true;
}

bool GameScript::framePreRenderEnded(const FrameEvent& evt) noexcept {
    return true;
}

bool GameScript::frameRenderingQueued(const FrameEvent& evt) noexcept {
    return true;
}

bool GameScript::framePostRenderStarted(const FrameEvent& evt) noexcept {
    return true;
}

bool GameScript::framePostRenderEnded(const FrameEvent& evt) noexcept {
    return true;
}

bool GameScript::frameEnded(const FrameEvent& evt) noexcept {
    return true;
}

}; //namespace Divide
