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

#ifndef _SCRIPTING_GAME_SCRIPT_H_
#define _SCRIPTING_GAME_SCRIPT_H_

#include "Headers/Script.h"
#include "Rendering/Headers/FrameListener.h"

namespace Divide {
class GameScriptInstance {
    public:
        void frameStarted();
        void framePreRenderStarted();
        void framePreRenderEnded();
        void frameRenderingQueued();
        void framePostRenderStarted();
        void framePostRenderEnded();
        void frameEnded();
    };

    class GameScript : public Script,
                       public FrameListener {
    public:
        explicit GameScript(const stringImpl& sourceCode);
        explicit GameScript(const stringImpl& scriptPath, FileType fileType);
        ~GameScript();

    protected:
        bool frameStarted(const FrameEvent& evt);
        bool framePreRenderStarted(const FrameEvent& evt);
        bool framePreRenderEnded(const FrameEvent& evt);
        bool frameRenderingQueued(const FrameEvent& evt);
        bool framePostRenderStarted(const FrameEvent& evt);
        bool framePostRenderEnded(const FrameEvent& evt);
        bool frameEnded(const FrameEvent& evt);

    private:
        void addGameInstance();
};

}; //namespace Divide

#endif //_SCRIPTING_GAME_SCRIPT_H_