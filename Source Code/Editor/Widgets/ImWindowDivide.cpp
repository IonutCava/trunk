#include "stdafx.h"

#include "Headers/ImWindowDivide.h"
#include "Headers/ImWindowManagerDivide.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {

ImwWindowDivide::ImwWindowDivide(ImwWindowManagerDivide& parent, PlatformContext& context, ImWindow::EPlatformWindowType eType, bool bCreateState)
    : ImwPlatformWindow(eType, bCreateState)
    , PlatformContextComponent(context)
    , _parent(parent)
    , _pWindow(nullptr)
    , _isMainWindow(false)
    , _windowGUID(-1)
    , _deltaTimeUS(0)
{
    Attorney::WindowManagerWindow::onCreateWindow(_parent, this);
}

ImwWindowDivide::~ImwWindowDivide()
{
    Attorney::WindowManagerWindow::onDestroyWindow(_parent, this);
}

void ImwWindowDivide::onDestroyPlatformWindow() {
    _pWindow = nullptr;
}

bool ImwWindowDivide::Init(ImwPlatformWindow* parent)
{
    ImwWindowDivide* pMainDivide= ((ImwWindowDivide*)parent);

    if (pMainDivide != nullptr) {
        WindowDescriptor descriptor;
        descriptor.title = "ImWindow";
        descriptor.targetDisplay = 0;
        descriptor.dimensions.set(_context.config().runtime.resolution);
        descriptor.clearColour.set(DefaultColours::RED);
        SetBit(descriptor.flags, to_base(WindowDescriptor::Flags::HIDDEN));

        if (GetType() != ImWindow::E_PLATFORM_WINDOW_TYPE_MAIN && 
            GetType() != ImWindow::E_PLATFORM_WINDOW_TYPE_SECONDARY)
        {
            ClearBit(descriptor.flags, to_base(WindowDescriptor::Flags::DECORATED));
        }

        ErrorCode err = ErrorCode::NO_ERR;
        U32 idx = _context.app().windowManager().createWindow(descriptor, err);
        if (err != ErrorCode::NO_ERR) {
            RestoreState();
            return false;
        }
        _pWindow = &_context.app().windowManager().getWindow(idx);
    } else {
        _pWindow = &_context.app().windowManager().getWindow(0u);
        _isMainWindow = true;
    }
    _pWindow->destroyCbk([this]() { onDestroyPlatformWindow();} );

    SetState();
    ImGuiIO& io = ImGui::GetIO();

    io.RenderDrawListsFn = nullptr;
    io.ImeWindowHandle = (void*)_pWindow->handle()._handle;
    RestoreState();

    _windowGUID = _pWindow->getGUID();

    _pWindow->addEventListener(WindowEvent::CLOSE_REQUESTED, [this](const DisplayWindow::WindowEventArgs& args) { ACKNOWLEDGE_UNUSED(args); OnClose();});
    _pWindow->addEventListener(WindowEvent::GAINED_FOCUS, [this](const DisplayWindow::WindowEventArgs& args) { OnFocus(args._flag);});
    _pWindow->addEventListener(WindowEvent::RESIZED, [this](const DisplayWindow::WindowEventArgs& args) { OnSize(args.x, args.y);});
    _pWindow->addEventListener(WindowEvent::MOUSE_BUTTON, [this](const DisplayWindow::WindowEventArgs& args) { OnMouseButton(args.id, args._flag);});
    _pWindow->addEventListener(WindowEvent::MOUSE_MOVE, [this](const DisplayWindow::WindowEventArgs& args) { OnMouseMove(args.x, args.y);});
    _pWindow->addEventListener(WindowEvent::MOUSE_WHEEL, [this](const DisplayWindow::WindowEventArgs& args) { OnMouseWheel(args._mod);});
    _pWindow->addEventListener(WindowEvent::KEY_PRESS, [this](const DisplayWindow::WindowEventArgs& args) { OnKey(args._key, args._flag);});
    _pWindow->addEventListener(WindowEvent::TEXT, [this](const DisplayWindow::WindowEventArgs& args) { OnUTF8(args._text);});
    if (GetType() == ImWindow::E_PLATFORM_WINDOW_TYPE_DRAG_PREVIEW) {
        //_pWindow->opacity(128);
    }

    return true;
}

ImVec2 ImwWindowDivide::GetPosition() const
{
    if (_pWindow != nullptr) {
        return ImVec2(to_F32(_pWindow->getPosition().x),
                      to_F32(_pWindow->getPosition().y));
    }
    return ImVec2(1.0f, 1.0f);
}

ImVec2 ImwWindowDivide::GetSize() const
{
    return _size;
}

ImVec2 ImwWindowDivide::GetDrawableSize() const
{
    return _drawableSize;
}

bool ImwWindowDivide::IsWindowMaximized() const
{
    if (_pWindow != nullptr) {
        return _pWindow->maximized();
    }
    return false;
}

bool ImwWindowDivide::IsWindowMinimized() const {
    if (_pWindow != nullptr) {
        return _pWindow->minimized();
    } 

    return false;
}

void ImwWindowDivide::Show(bool bShow) {
    if (_pWindow != nullptr) {
        _pWindow->hidden(!bShow);
    }
}

void ImwWindowDivide::SetSize(int iWidth, int iHeight)
{
    if (_pWindow != nullptr) {
        vec2<U16> dimensions(iWidth, iHeight);
        _pWindow->setDimensions(dimensions);
    }
}

void ImwWindowDivide::SetPosition(int iX, int iY)
{
    if (_pWindow != nullptr) {
        _pWindow->setPosition(iX, iY);
    }
}

void ImwWindowDivide::SetWindowMaximized(bool bMaximized)
{
    if (_pWindow != nullptr) {
        _pWindow->maximized(bMaximized);
    }
}

void ImwWindowDivide::SetWindowMinimized()
{
    if (_pWindow != nullptr) {
        _pWindow->minimized(true);
    }
}

void ImwWindowDivide::SetTitle(const char* pTitle)
{
    if (_pWindow != nullptr) {
        _pWindow->title(pTitle);
    }
}

void ImwWindowDivide::update(const U64 deltaTimeUS) {
    _deltaTimeUS = deltaTimeUS;
}

void ImwWindowDivide::PreUpdate()
{

}

void ImwWindowDivide::Render()
{
    if (!m_bNeedRender) {
        return;
    }

    SetState();
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = Time::MicrosecondsToSeconds<float>(_deltaTimeUS);
    io.DisplaySize = GetSize();
    io.DisplayFramebufferScale = GetDrawableSize();
    ImGui::Render();
    _parent.renderDrawList(ImGui::GetDrawData(), _windowGUID);
    RestoreState();
}

bool ImwWindowDivide::OnClose() {
    ImwPlatformWindow::OnClose();
    return true;
}

void ImwWindowDivide::OnFocus(bool bHasFocus)
{
    if (!bHasFocus) {
        OnLoseFocus();
    }
}

void ImwWindowDivide::OnSize(int iWidth, int iHeight)
{
    ACKNOWLEDGE_UNUSED(iWidth);
    ACKNOWLEDGE_UNUSED(iHeight);
    if (_pWindow != nullptr) {
        _size = ImVec2(_pWindow->getDimensions().x, _pWindow->getDimensions().y);
        vec2<U16> display_size = _pWindow->getDrawableSize();
        _drawableSize = ImVec2(_size.x > 0 ? ((float)display_size.w / _size.x) : 0, _size.y > 0 ? ((float)display_size.h / _size.y) : 0);
    }
}

void ImwWindowDivide::OnMouseButton(int iButton, bool bDown)
{
    ((ImGuiContext*)m_pState)->IO.MouseDown[iButton] = bDown;
}

void ImwWindowDivide::OnMouseMove(int iX, int iY)
{
    ((ImGuiContext*)m_pState)->IO.MousePos = ImVec2((float)iX, (float)iY);
}

void ImwWindowDivide::OnMouseWheel(int iStep)
{
    ((ImGuiContext*)m_pState)->IO.MouseWheel += iStep;
}

void ImwWindowDivide::OnKey(Input::KeyCode eKey, bool bDown)
{
    ((ImGuiContext*)m_pState)->IO.KeysDown[eKey] = bDown;
}

void ImwWindowDivide::OnUTF8(const char* text)
{
    ((ImGuiContext*)m_pState)->IO.AddInputCharactersUTF8(text);
}

}; //namespace Divide