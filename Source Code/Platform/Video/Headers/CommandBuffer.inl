/*
Copyright (c) 2017 DIVIDE-Studio
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
inline void CommandBuffer::add(const T& command) {
    static_assert(std::is_base_of<Command, T>::value, "CommandBuffer error: Unknown command type!");
    if (!command.empty()) {
        _data.emplace_back(std::make_unique<T>(command));
        rebuildCaches();
    }
}

inline void CommandBuffer::add(const CommandBuffer& other) {
    _data.insert(std::end(_data),
                    std::cbegin(other._data),
                    std::cend(other._data));

    rebuildCaches();
}

inline vectorImpl<std::shared_ptr<Command>>& CommandBuffer::operator()() {
    return _data;
}

inline const vectorImpl<std::shared_ptr<Command>>& CommandBuffer::operator()() const {
    return _data;
}

inline const vectorImpl<Pipeline*>& CommandBuffer::getPipelines() const {
    return _pipelineCache;
}

inline const vectorImpl<PushConstants*>& CommandBuffer::getPushConstants() const {
    return _pushConstantsCache;
}

inline const vectorImpl<DescriptorSet*>& CommandBuffer::getDescriptorSets() const {
    return _descriptorSetCache;
}

inline const vectorImpl<GenericDrawCommand*>& CommandBuffer::getDrawCommands() const {
    return _drawCommandsCache;
}

inline void CommandBuffer::clear() {
    _data.clear();
    _pipelineCache.clear();
    _pushConstantsCache.clear();
    _descriptorSetCache.clear();
    _drawCommandsCache.clear();
}

inline bool CommandBuffer::empty() const {
    return _data.empty();
}

}; //namespace GFX
}; //namespace Divide


#endif //_COMMAND_BUFFER_INL_