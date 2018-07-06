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

#ifndef _IM_WINDOW_DIVIDE_H_
#define _IM_WINDOW_DIVIDE_H_

#include "ImwPlatformWindow.h"

#include "Platform/Headers/DisplayWindow.h"
#include "Platform/Input/Headers/InputInterface.h"

namespace Divide {
    
FWD_DECLARE_MANAGED_CLASS(Texture);
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);
class ImwWindowDivide : public ImWindow::ImwPlatformWindow
{
    friend class ImwWindowManagerOGL;
    public:
    ImwWindowDivide(PlatformContext& context, bool bMain, bool bIsDragWindow, bool bCreateState);
    virtual ~ImwWindowDivide();

    virtual bool Init(ImWindow::ImwPlatformWindow* parent) override;

    virtual ImVec2 GetPosition() const override;
    virtual ImVec2 GetSize() const override;
    virtual bool IsWindowMaximized() const override;

    virtual void Show() override;
    virtual void Hide() override;
    virtual void SetSize(int iWidth, int iHeight) override;
    virtual void SetPosition(int iX, int iY) override;
    virtual void SetWindowMaximized(bool bMaximized) override;
    virtual void SetTitle(const char* pTtile) override;

    protected:
    virtual void PreUpdate() override;
    virtual void Render() override;
    virtual void Destroy() override;
    virtual void StartDrag() override;
    virtual void StopDrag() override;
    virtual bool IsDraging() override;

    bool OnClose();
    void OnFocus(bool bHasFocus);
    void OnSize(int iWidth, int iHeight);
    void OnMouseButton(int iButton, bool bDown);
    void OnMouseMove(int iX, int iY);
    void OnMouseWheel(int iStep);
    void OnKey(Divide::Input::KeyCode eKey, bool bDown);
    void OnChar(int iChar);

    void RenderDrawList(ImDrawData* pDrawData);

    DisplayWindow* m_pWindow;
    Texture_ptr    m_texture;

  private:
    static U32 m_windowCount;

    bool m_isMainWindow = false;
    PlatformContext& m_context;
    ShaderProgram_ptr m_imWindowProgram;
};
}; //namespace Divide

#endif //_IM_WINDOW_DIVIDE_H_