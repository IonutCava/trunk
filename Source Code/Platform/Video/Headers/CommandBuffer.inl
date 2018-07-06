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
    //std::shared_ptr<Command> commandPtr = std::make_unique<Command>(command);
    //GFX::CommandType type = commandPtr->_type;
    //vectorAlg::emplace_back(_data, commandPtr);
    _data.emplace_back(std::make_unique<T>(command));
    /*vector<std::share_ptr<Command>>& commands = _commands[to_base(type)];

    vectorAlg::emplace_back(_commandOrder, std::make_pair(type, commands.size()));
    vectorAlg::emplace_back(commands, _data.back()));*/
}

inline void CommandBuffer::add(const CommandBuffer& other) {
    if (!other.empty()) {
        _data.insert(std::end(_data),
                     std::cbegin(other._data),
                     std::cend(other._data));

        /*for (const std::shared_ptr<Command>& cmd : _data) {
            add(*cmd);
        }*/
    }
}

inline vectorEASTL<std::shared_ptr<Command>>& CommandBuffer::operator()() {
    return _data;
}

inline const vectorEASTL<std::shared_ptr<Command>>& CommandBuffer::operator()() const {
    return _data;
}

inline void CommandBuffer::clear() {
    _data.clear();
}

inline bool CommandBuffer::empty() const {
    return _data.empty();
}

}; //namespace GFX
}; //namespace Divide


#endif //_COMMAND_BUFFER_INL_