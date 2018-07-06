#include "stdafx.h"

#include "Headers/ImWindowManagerDivide.h"
#include "Headers/ImWindowDivide.h"

#include "Editor/Headers/Editor.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {

ImwWindowManagerDivide::ImwWindowManagerDivide(Editor& parent)
    : _parent(parent)
{
}

ImwWindowManagerDivide::~ImwWindowManagerDivide()
{
    Destroy();
}

ImWindow::ImwPlatformWindow* ImwWindowManagerDivide::CreatePlatformWindow(ImWindow::EPlatformWindowType eType, ImWindow::ImwPlatformWindow* pParent)
{
    IM_ASSERT(m_pCurrentPlatformWindow == nullptr);

    ImwWindowDivide* pWindow = MemoryManager_NEW ImwWindowDivide(*this, _parent.context(), eType, CanCreateMultipleWindow());
    if (pWindow->Init(pParent)) {
        return (ImWindow::ImwPlatformWindow*)pWindow;
    } else {
        MemoryManager::SAFE_DELETE(pWindow);
        return nullptr;
    }
}

void ImwWindowManagerDivide::update(const U64 deltaTimeUS) {
    for (ImwWindowDivide* win : _windows) {
        win->update(deltaTimeUS);
    }
}

ImVec2 ImwWindowManagerDivide::GetCursorPos()
{
    vec2<I32> cursorPos = _parent.context().app().windowManager().getCursorPosition();
    return ImVec2(cursorPos.x, cursorPos.y);
}

bool ImwWindowManagerDivide::IsLeftClickDown()
{
    return _parent.context().input().getMouseButtonState(0, Input::MouseButton::MB_Left) == Input::InputState::PRESSED;
}


void ImwWindowManagerDivide::renderDrawList(ImDrawData* pDrawData, I64 windowGUID) {
    Attorney::EditorWindowManager::renderDrawList(_parent, pDrawData, windowGUID);
}

void ImwWindowManagerDivide::onCreateWindow(ImwWindowDivide* window) {
    ACKNOWLEDGE_UNUSED(window);

    _windows.push_back(window);
}

void ImwWindowManagerDivide::onDestroyWindow(ImwWindowDivide* window) {
    if (!window->isMainWindow() && window->nativeWindow()) {
        DisplayWindow* windowPtr = window->nativeWindow();
        _parent.context().app().windowManager().destroyWindow(windowPtr);
    }
    _windows.erase(
        std::remove_if(std::begin(_windows), std::end(_windows),
                       [&window](ImwWindowDivide* windowIt)
                       -> bool { return window == windowIt; }),
        std::end(_windows));
}

}; //namespace Divide