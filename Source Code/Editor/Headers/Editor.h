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

#ifndef _DIVIDE_EDITOR_H_
#define _DIVIDE_EDITOR_H_

#include "Core/Headers/NonCopyable.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Platform/Headers/PlatformDefines.h"
#include "Rendering/Headers/FrameListener.h"

namespace Divide {

class PlatformContext;
class ImwWindowManagerDivide;

class Editor : private NonCopyable, public FrameListener {
  public:
    explicit Editor(PlatformContext& context);
    ~Editor();

    bool init();
    void close();

    void update(const U64 deltaTimeUS);

  protected:
    bool frameStarted(const FrameEvent& evt);
    bool framePreRenderStarted(const FrameEvent& evt);
    bool framePreRenderEnded(const FrameEvent& evt);
    bool frameRenderingQueued(const FrameEvent& evt);
    bool framePostRenderStarted(const FrameEvent& evt);
    bool framePostRenderEnded(const FrameEvent& evt);
    bool frameEnded(const FrameEvent& evt);
  private:
    PlatformContext& _context;
    std::unique_ptr<ImwWindowManagerDivide> _windowManager;

    Time::ProfileTimer& _editorTimer;
    Time::ProfileTimer& _editorUpdateTimer;
    Time::ProfileTimer& _editorRenderTimer;

}; //Editor
}; //namespace Divide

#endif //_DIVIDE_EDITOR_H_