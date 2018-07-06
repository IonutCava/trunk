#include "stdafx.h"

#include "Headers/ImWindowManagerDivide.h"
#include "Headers/ImWindowDivide.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {

ImwWindowManagerDivide::ImwWindowManagerDivide(PlatformContext& context)
    : _context(context)
{
}

ImwWindowManagerDivide::~ImwWindowManagerDivide()
{
    Destroy();
}

ImWindow::ImwPlatformWindow* ImwWindowManagerDivide::CreatePlatformWindow(ImWindow::EPlatformWindowType eType, ImWindow::ImwPlatformWindow* pParent)
{
    IM_ASSERT(m_pCurrentPlatformWindow == nullptr);

    ImwWindowDivide* pWindow = MemoryManager_NEW ImwWindowDivide(_context, eType, CanCreateMultipleWindow());
    if (pWindow->Init(pParent)) {
        return (ImWindow::ImwPlatformWindow*)pWindow;
    } else {
        MemoryManager::SAFE_DELETE(pWindow);
        return nullptr;
    }
}

ImVec2 ImwWindowManagerDivide::GetCursorPos()
{
    return ImVec2(vec2<F32>(_context.app().windowManager().getCursorPosition()));
}

bool ImwWindowManagerDivide::IsLeftClickDown()
{
    return _context.input().getMouseButtonState(0, Input::MouseButton::MB_Left) == Input::InputState::PRESSED;
}

}; //namespace Divide