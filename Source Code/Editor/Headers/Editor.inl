/*
   Copyright (c) 2018 DIVIDE-Studio
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

#ifndef _DIVIDE_EDITOR_INL_
#define _DIVIDE_EDITOR_INL_

namespace Divide {

inline bool Editor::isInit() const noexcept {
    return _mainWindow != nullptr;
}

inline bool Editor::running() const noexcept {
    return _running;
}

inline bool Editor::inEditMode() const noexcept {
    return running() && simulationPauseRequested();
}

inline void Editor::setTransformSettings(const TransformSettings& settings) noexcept {
    Attorney::GizmoEditor::setTransformSettings(*_gizmo, settings);
}

inline const TransformSettings& Editor::getTransformSettings() const noexcept {
    return Attorney::GizmoEditor::getTransformSettings(*_gizmo);
}

inline const Rect<I32>& Editor::getTargetViewport() const noexcept {
    return _targetViewport;
}

inline void Editor::setSelectedCamera(Camera* camera) noexcept {
    _selectedCamera = camera;
}

inline Camera* Editor::getSelectedCamera() const noexcept {
    return _selectedCamera;
}

inline bool Editor::simulationPauseRequested() const noexcept {
    return _stepQueue == 0;
}

template<typename T>
inline void Editor::registerUndoEntry(const UndoEntry<T>& entry) {
    _undoManager->registerUndoEntry(entry);
}

inline size_t Editor::UndoStackSize() const noexcept {
    return _undoManager->UndoStackSize();
}

inline size_t Editor::RedoStackSize() const noexcept {
    return _undoManager->RedoStackSize();
}

inline void Editor::toggleMemoryEditor(bool state) noexcept {
    _showMemoryEditor = state;
}

inline bool Editor::scenePreviewFocused() const noexcept {
    return _scenePreviewFocused;
}

inline bool Editor::scenePreviewHovered() const noexcept {
    return _sceneHovered;
}

}; //namespace Divide

#endif //_DIVIDE_EDITOR_INL_