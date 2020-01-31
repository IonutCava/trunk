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

#ifndef _COMMAND_BUFFER_INL_
#define _COMMAND_BUFFER_INL_


namespace Divide {
namespace GFX {

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, T*>::type
CommandBuffer::allocateCommand() {
    constexpr U8 index = static_cast<U8>(T::EType);

    const I24 cmdIndex = _commandCount[index]++;
    _commandOrder.emplace_back(index, cmdIndex);

    return static_cast<T*>(_commands.getPtrOrNull(index, cmdIndex));
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, T&>::type
CommandBuffer::add(const T& command) {
    T* mem = allocateCommand<T>();
    if (mem != nullptr) {
        *mem = command;
    } else {
        mem = CmdAllocator<T>::allocate(command);
        _commands.insert<T>(to_base(T::EType),
                            deleted_unique_ptr<CommandBase>(
                                mem,
                                [](CommandBase *& cmd) {
                                    CmdAllocator<T>::deallocate((T*&)(cmd));
                                }
                            ));
    }

    _batched = false;
    return *mem;
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, T&>::type
CommandBuffer::add(T&& command) {
    T* mem = allocateCommand<T>();
    if (mem != nullptr) {
        *mem = command;
    } else {
        mem = CmdAllocator<T>::allocate(command);
        _commands.insert<T>(to_base(T::EType),
                            deleted_unique_ptr<CommandBase>(
                                mem,
                                [](CommandBase *& cmd) {
                                    CmdAllocator<T>::deallocate((T*&)(cmd));
                                }
                            ));
    }

    _batched = false;
    return *mem;
}


template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, T&>::type
CommandBuffer::get(const CommandEntry& commandEntry) noexcept {
    return static_cast<T&>(_commands.get(commandEntry));
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, const T&>::type
CommandBuffer::get(const CommandEntry& commandEntry) const noexcept {
    return static_cast<const T&>(_commands.get(commandEntry));
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, CommandBuffer::Container::EntryList&>::type
CommandBuffer::get() noexcept {
    return _commands.get(to_base(T::EType));
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, const CommandBuffer::Container::EntryList&>::type
CommandBuffer::get() const noexcept {
    return _commands.get(to_base(T::EType));
}

inline bool CommandBuffer::exists(const CommandEntry& commandEntry) const noexcept {
    return _commands.exists(commandEntry._typeIndex, commandEntry._elementIndex);
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, T&>::type
CommandBuffer::get(I24 index) noexcept {
    return get<T>({to_base(T::EType), index});
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, const T&>::type
CommandBuffer::get(I24 index) const noexcept {
    return get<T>({to_base(T::EType), index });
}

inline bool CommandBuffer::exists(U8 typeIndex, I24 index) const noexcept {
    return _commands.exists(typeIndex, index);
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, T*>::type
CommandBuffer::getPtr(const CommandEntry& commandEntry) noexcept {
    return static_cast<T*>(_commands.getPtr(commandEntry));
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, const T*>::type
CommandBuffer::getPtr(const CommandEntry& commandEntry) const noexcept {
    return static_cast<const T*>(_commands.getPtr(commandEntry));
}

template<typename T>
typename std::enable_if<std::is_base_of<CommandBase, T>::value, T*>::type
CommandBuffer::getPtr(I24 index) noexcept {
    return _commands.getPtr(to_base(T::EType), index);
}

template<typename T>
typename std::enable_if<std::is_base_of<CommandBase, T>::value, const T*>::type
CommandBuffer::getPtr(I24 index) const noexcept {
    return _commands.getPtr(to_base(T::EType), index);
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, size_t>::type
CommandBuffer::count() const noexcept {
    return _commands.size(to_base(T::EType));
}

inline CommandBuffer::CommandOrderContainer& CommandBuffer::operator()() noexcept {
    return _commandOrder;
}

inline const CommandBuffer::CommandOrderContainer& CommandBuffer::operator()() const noexcept {
    return _commandOrder;
}

inline void CommandBuffer::clear(bool clearMemory) {
    _commandCount.fill(0);
    if (clearMemory) {
        _commandOrder.clear();
        _commands.clear(true);
    } else {
        _commandOrder.resize(0);
    }

    _batched = true;
}

inline void CommandBuffer::nuke() {
    _commandOrder.clear();
    _commandCount.fill(0);
    _commands.nuke();
    _batched = true;
}

inline bool CommandBuffer::empty() const noexcept {
    return _commandOrder.empty();
}

template<>
inline bool CommandBuffer::tryMergeCommands(GFX::CommandType type, DrawCommand* prevCommand, DrawCommand* crtCommand, bool& partial) const {
    OPTICK_EVENT();
    ACKNOWLEDGE_UNUSED(type);

    vectorEASTLFast<GenericDrawCommand>& commands = prevCommand->_drawCommands;
    commands.insert(eastl::cend(commands),
                    eastl::make_move_iterator(eastl::begin(crtCommand->_drawCommands)),
                    eastl::make_move_iterator(eastl::end(crtCommand->_drawCommands)));
    crtCommand->_drawCommands.resize(0);

    partial = false;
    bool merged = true;
    while (merged) {
        merged = mergeDrawCommands(commands, true);
        merged = mergeDrawCommands(commands, false) || merged;
        if (merged) {
            partial = true;
        }
    }

    return true;
}

template<>
inline bool CommandBuffer::tryMergeCommands(GFX::CommandType type, BindDescriptorSetsCommand* prevCommand, BindDescriptorSetsCommand* crtCommand, bool& partial) const {
    OPTICK_EVENT();
    ACKNOWLEDGE_UNUSED(type);

    return Merge(prevCommand->_set, crtCommand->_set, partial);
}

template<>
inline bool CommandBuffer::tryMergeCommands(GFX::CommandType type, SendPushConstantsCommand* prevCommand, SendPushConstantsCommand* crtCommand, bool& partial) const {
    OPTICK_EVENT();
    ACKNOWLEDGE_UNUSED(type);

    return Merge(prevCommand->_constants, crtCommand->_constants, partial);
}

template<>
inline bool CommandBuffer::tryMergeCommands(GFX::CommandType type, DrawTextCommand* prevCommand, DrawTextCommand* crtCommand, bool& partial) const {
    OPTICK_EVENT();
    ACKNOWLEDGE_UNUSED(type);

    partial = false;
    prevCommand->_batch._data.insert(std::cend(prevCommand->_batch._data),
                                     std::cbegin(crtCommand->_batch._data),
                                     std::cend(crtCommand->_batch._data));
    return true;
}

template<>
inline bool CommandBuffer::tryMergeCommands(GFX::CommandType type, SetScissorCommand* prevCommand, SetScissorCommand* crtCommand, bool& partial) const {
    OPTICK_EVENT();
    ACKNOWLEDGE_UNUSED(type);

    partial = false;
    return prevCommand->_rect == crtCommand->_rect;
}

template<>
inline bool CommandBuffer::tryMergeCommands(GFX::CommandType type, SetViewportCommand* prevCommand, SetViewportCommand* crtCommand, bool& partial) const {
    OPTICK_EVENT();
    ACKNOWLEDGE_UNUSED(type);

    partial = false;
    return prevCommand->_viewport == crtCommand->_viewport;
}

template<>
inline bool CommandBuffer::tryMergeCommands(GFX::CommandType type, BindPipelineCommand* prevCommand, BindPipelineCommand* crtCommand, bool& partial) const {
    OPTICK_EVENT();
    ACKNOWLEDGE_UNUSED(type);

    partial = false;
    return *prevCommand->_pipeline == *crtCommand->_pipeline;
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, bool>::type
CommandBuffer::tryMergeCommands(GFX::CommandType type, T* prevCommand, T* crtCommand, bool& partial) const {
    OPTICK_EVENT();

    assert(prevCommand != nullptr && crtCommand != nullptr);
    switch (type) {
        case GFX::CommandType::DRAW_COMMANDS:
            return tryMergeCommands(type, static_cast<DrawCommand*>(prevCommand), static_cast<DrawCommand*>(crtCommand), partial);
        case GFX::CommandType::BIND_DESCRIPTOR_SETS:
            return tryMergeCommands(type, static_cast<BindDescriptorSetsCommand*>(prevCommand), static_cast<BindDescriptorSetsCommand*>(crtCommand), partial);
        case GFX::CommandType::SEND_PUSH_CONSTANTS:
            return tryMergeCommands(type, static_cast<SendPushConstantsCommand*>(prevCommand), static_cast<SendPushConstantsCommand*>(crtCommand), partial);
        case GFX::CommandType::DRAW_TEXT:
            return tryMergeCommands(type, static_cast<DrawTextCommand*>(prevCommand), static_cast<DrawTextCommand*>(crtCommand), partial);
        case GFX::CommandType::SET_SCISSOR:
            return tryMergeCommands(type, static_cast<SetScissorCommand*>(prevCommand), static_cast<SetScissorCommand*>(crtCommand), partial);
        case GFX::CommandType::SET_VIEWPORT:
            return tryMergeCommands(type, static_cast<SetViewportCommand*>(prevCommand), static_cast<SetViewportCommand*>(crtCommand), partial);
        case GFX::CommandType::BIND_PIPELINE:
            return tryMergeCommands(type, static_cast<BindPipelineCommand*>(prevCommand), static_cast<BindPipelineCommand*>(crtCommand), partial);
    }
    return false;
}

}; //namespace GFX
}; //namespace Divide


#endif //_COMMAND_BUFFER_INL_