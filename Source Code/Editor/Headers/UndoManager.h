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

#pragma once
#pragma once
#ifndef _UNDO_MANAGER_H_
#define _UNDO_MANAGER_H_

#include "Platform/Video/Headers/PushConstant.h"

namespace Divide {
    struct IUndoEntry {
        GFX::PushConstantType _type = GFX::PushConstantType::COUNT;
        stringImpl _name = "";
        void* _data = nullptr;
        std::function<void(const void*)> _dataSetter = {};
        DELEGATE<void, const char*> _onChangedCbk;

        virtual void swapValues() = 0;
        virtual void apply() = 0;
    };

    template<typename T>
    struct UndoEntry : public IUndoEntry {
        T oldVal = {};
        T newVal = {};

        void swapValues() override {
            std::swap(oldVal, newVal);
        }

        void apply() override {
            if (_dataSetter) {
                _dataSetter(&oldVal);
            } else {
                *static_cast<T*>(_data) = oldVal;
            }
            if (_onChangedCbk) {
                _onChangedCbk(_name.c_str());
            }
        }
    };

    class UndoManager {

    public:
        using UndoStack = std::deque<std::shared_ptr<IUndoEntry>>;

    public:
        UndoManager(U32 maxSize);
        ~UndoManager() = default;

        bool Undo();
        bool Redo();

        const stringImpl& lasActionName() const;

        template<typename T>
        void registerUndoEntry(const UndoEntry<T>& entry) {
            auto entryPtr = std::make_shared<UndoEntry<T>>(entry);
            _undoStack.push_back(entryPtr);
            if (to_U32(_undoStack.size()) >= _maxSize) {
                _undoStack.pop_front();
            }
        }

    private:
        bool apply(const std::shared_ptr<IUndoEntry>& entry);

    private:
        const U32 _maxSize = 10;
        UndoStack _undoStack;
        UndoStack _redoStack;
        stringImpl _lastActionName;
    };

}; //namespace Divide
#endif //_UNDO_MANAGER_H_