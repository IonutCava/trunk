#include "stdafx.h"

#include "Headers/UndoManager.h"

namespace Divide {

UndoManager::UndoManager(U32 maxSize)
    : _maxSize(maxSize)
{
}

bool UndoManager::Undo() {
    if (!_undoStack.empty()) {
        auto entry = _undoStack.back();
        bool ret = apply(entry);
        _undoStack.pop_back();
        if (ret) {
            entry->swapValues();
            _redoStack.push_back(entry);
        }
        return ret;
    }

    return false;
}

bool UndoManager::Redo() {
    if (!_redoStack.empty()) {
        auto entry = _redoStack.back();
        bool ret = apply(entry);
        _redoStack.pop_back();
        if (ret) {
            entry->swapValues();
            _undoStack.push_back(entry);
        }
        return ret;
    }
    return false;
}

bool UndoManager::apply(const std::shared_ptr<IUndoEntry>& entry) {
    if (entry != nullptr && (entry->_data != nullptr || entry->_dataSetter)) {
        entry->apply();
        return true;
    }
    return false;
}

}; //namespace Divide 