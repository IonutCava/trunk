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

#ifndef _DIVIDE_EDITOR_SAMPLE_H_
#define _DIVIDE_EDITOR_SAMPLE_H_

#include "Core/Headers/NonCopyable.h"
#include "Platform/Headers/PlatformDefines.h"
#include "Rendering/Headers/FrameListener.h"

#include "Widgets/Headers/ImWindowManagerDivide.h"

namespace Divide {
using namespace ImWindow;

class StyleEditorWindow : public ImwWindow
{
  public:
    StyleEditorWindow();
    void OnGui();
};


class NodeWindow : public ImwWindow
{
  public:
    NodeWindow();
    virtual void OnGui();
};

class MyStatusBar : public ImwStatusBar
{
  public:
    MyStatusBar();
    virtual void OnStatusBar();

    float m_fTime;
};

class MyToolBar : public ImwToolBar
{
  public:
    MyToolBar();
    virtual void OnToolBar();
};

class MyMenu : public ImwMenu
{
  public:
    MyMenu();

    virtual void OnMenu();
};

class MyImwWindow3 : public ImwWindow
{
  public:
    MyImwWindow3(const char* pTitle = "MyImwWindow3");
    virtual void OnGui();
};

class MyImwWindow2 : public ImwWindow
{
  public:
    MyImwWindow2(const char* pTitle = "MyImwWindow2");
    virtual void OnGui();
};

class MyImwWindowFillSpace : public ImwWindow
{
  public:
    MyImwWindowFillSpace();
    virtual void OnGui();
};

class MyImwWindow : public ImwWindow, ImwMenu
{
  public:
    MyImwWindow(const char* pTitle = "MyImwWindow");
    virtual void OnGui();
    virtual void OnContextMenu();
    virtual void OnMenu();

    char m_pText[512];
};

void InitSample();
}; //namespace Divide

#endif //_DIVIDE_EDITOR_SAMPLE_H_
