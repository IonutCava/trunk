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
    IM_ASSERT(m_pCurrentPlatformWindow == NULL);

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
    POINT oPoint;
    ::GetCursorPos(&oPoint);
    return ImVec2((float)oPoint.x, (float)oPoint.y);
}

bool ImwWindowManagerDivide::IsLeftClickDown()
{
    return GetAsyncKeyState(VK_LBUTTON) != 0;
}

}; //namespace Divide