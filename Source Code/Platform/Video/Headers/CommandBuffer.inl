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

#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexDataInterface.h"

#ifndef _COMMAND_BUFFER_INL_
#define _COMMAND_BUFFER_INL_

namespace Divide {
namespace GFX {

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, void>::type
CommandBuffer::add(const T* command) {
    T* ptr = nullptr;
    {
        WriteLock w_lock(T::s_PoolMutex);
        ptr = T::s_Pool.newElement(*command);
    }

    _commandOrder.emplace_back(_commands.insert(static_cast<vec_size_eastl>(command->_type),
                                                std::shared_ptr<T>(ptr, [](T* cmd)
                                                {
                                                    WriteLock w_lock(T::s_PoolMutex);
                                                    T::s_Pool.deleteElement(cmd);
                                                    cmd = nullptr;
                                                })));
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, T*>::type
CommandBuffer::getCommandInternal(const CommandEntry& commandEntry) {
    return static_cast<T*>(&_commands.get(commandEntry));
}

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, const T&>::type
CommandBuffer::getCommand(const CommandEntry& commandEntry) const {
    return static_cast<const T&>(_commands.get(commandEntry));
}

inline vectorEASTL<CommandBuffer::CommandEntry>& CommandBuffer::operator()() {
    return _commandOrder;
}

inline const vectorEASTL<CommandBuffer::CommandEntry>& CommandBuffer::operator()() const {
    return _commandOrder;
}

inline void CommandBuffer::clear() {
    _commandOrder.resize(0);
    _commands.clear();
}

inline bool CommandBuffer::empty() const {
    return _commandOrder.empty();
}

template<>
inline bool CommandBuffer::tryMergeCommands(DrawCommand* prevCommand, DrawCommand* crtCommand) const {

    prevCommand->_drawCommands.insert(eastl::cend(prevCommand->_drawCommands),
        eastl::cbegin(crtCommand->_drawCommands),
        eastl::cend(crtCommand->_drawCommands));

    auto batch = [](GenericDrawCommand& previousIDC, GenericDrawCommand& currentIDC)  -> bool {
        if (compatible(previousIDC, currentIDC) &&
            // Batchable commands must share the same buffer
            previousIDC._sourceBuffer->getGUID() == currentIDC._sourceBuffer->getGUID())
        {
            U32 prevCount = previousIDC._drawCount;
            if (previousIDC._cmd.baseInstance + prevCount != currentIDC._cmd.baseInstance) {
                return false;
            }
            // If the rendering commands are batchable, increase the draw count for the previous one
            previousIDC._drawCount = to_U16(prevCount + currentIDC._drawCount);
            // And set the current command's draw count to zero so it gets removed from the list later on
            currentIDC._drawCount = 0;

            return true;
        }

        return false;
    };

    vectorEASTL<GenericDrawCommand>& commands = prevCommand->_drawCommands;
    vec_size previousCommandIndex = 0;
    vec_size currentCommandIndex = 1;
    const vec_size commandCount = commands.size();
    for (; currentCommandIndex < commandCount; ++currentCommandIndex) {
        GenericDrawCommand& previousCommand = commands[previousCommandIndex];
        GenericDrawCommand& currentCommand = commands[currentCommandIndex];
        if (!batch(previousCommand, currentCommand))
        {
            previousCommandIndex = currentCommandIndex;
        }
    }

    commands.erase(eastl::remove_if(eastl::begin(commands),
                                    eastl::end(commands),
                                    [](const GenericDrawCommand& cmd) -> bool {
                                    return cmd._drawCount == 0;
                                }),
                  eastl::end(commands));
    return true;
}

template<>
inline bool CommandBuffer::tryMergeCommands(BindDescriptorSetsCommand* prevCommand, BindDescriptorSetsCommand* crtCommand) const {
    return Merge(*prevCommand->_set, *crtCommand->_set);
}

template<>
inline bool CommandBuffer::tryMergeCommands(SendPushConstantsCommand* prevCommand, SendPushConstantsCommand* crtCommand) const {
    return Merge(prevCommand->_constants, crtCommand->_constants);
}

template<typename T>
inline bool CommandBuffer::tryMergeCommands(T* prevCommand, T* crtCommand) const {
    return false;
}

template<>
inline bool CommandBuffer::tryMergeCommands(GFX::CommandBase* prevCommand, GFX::CommandBase* crtCommand) const {
    assert(prevCommand != nullptr && crtCommand != nullptr);
    switch (prevCommand->_type) {
    case GFX::CommandType::DRAW_COMMANDS:
        return tryMergeCommands(static_cast<DrawCommand*>(prevCommand), static_cast<DrawCommand*>(crtCommand));
    case GFX::CommandType::BIND_DESCRIPTOR_SETS:
        return tryMergeCommands(static_cast<BindDescriptorSetsCommand*>(prevCommand), static_cast<BindDescriptorSetsCommand*>(crtCommand));
    case GFX::CommandType::SEND_PUSH_CONSTANTS:
        return tryMergeCommands(static_cast<SendPushConstantsCommand*>(prevCommand), static_cast<SendPushConstantsCommand*>(crtCommand));
    }
    return false;
}

}; //namespace GFX
}; //namespace Divide


#endif //_COMMAND_BUFFER_INL_