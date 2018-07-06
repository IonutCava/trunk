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
inline typename std::enable_if<std::is_base_of<Command, T>::value, void>::type
CommandBuffer::add(const T& command) {
    GFX::CommandType type = command._type;
    _commands.insert(command);
    _commandOrder.emplace_back(type, _commands.size(getType(type)) - 1);
    command.onAdd(this);
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

}; //namespace GFX
}; //namespace Divide


#endif //_COMMAND_BUFFER_INL_