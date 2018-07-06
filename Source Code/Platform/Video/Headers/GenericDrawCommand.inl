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

#ifndef _GENERIC_DRAW_COMMAND_INL_
#define _GENERIC_DRAW_COMMAND_INL_

namespace Divide {

inline U32  GenericDrawCommand::renderMask()                              const { return _renderOptions; }

inline void GenericDrawCommand::LoD(U8 lod)                                     { _lodIndex = lod; }
inline void GenericDrawCommand::drawCount(U16 count)                            { _drawCount = count; }
inline void GenericDrawCommand::drawToBuffer(U8 index)                          { _drawToBuffer = index; }
inline void GenericDrawCommand::commandOffset(U32 offset)                       { _commandOffset = offset; }
inline void GenericDrawCommand::patchVertexCount(U32 vertexCount)               { _patchVertexCount = vertexCount; }
inline void GenericDrawCommand::primitiveType(PrimitiveType type)               { _type = type; }
inline void GenericDrawCommand::sourceBuffer(VertexDataInterface* sourceBuffer) { _sourceBuffer = sourceBuffer; }

inline U8 GenericDrawCommand::LoD()                            const { return _lodIndex; }
inline U16 GenericDrawCommand::drawCount()                     const { return _drawCount; }
inline U8  GenericDrawCommand::drawToBuffer()                  const { return _drawToBuffer; }
inline U32 GenericDrawCommand::commandOffset()                 const { return _commandOffset; }
inline U32 GenericDrawCommand::patchVertexCount()              const { return _patchVertexCount; }
inline IndirectDrawCommand& GenericDrawCommand::cmd()                { return _cmd; }
inline PrimitiveType GenericDrawCommand::primitiveType()       const { return _type; }
inline VertexDataInterface* GenericDrawCommand::sourceBuffer() const { return _sourceBuffer; }
inline const IndirectDrawCommand& GenericDrawCommand::cmd()    const { return _cmd; }

template<typename T>
void GenericCommandBuffer::add(const T& command) {
    static_assert(std::is_base_of<Command, T>::value,"GenericCommandBuffer error: Unknown command type!");

    _data.emplace_back(std::make_unique<T>(command));

    if (command._type == CommandType::BIND_PIPELINE ||
        command._type == CommandType::DRAW_COMMANDS) {
        rebuildCaches();
    }
}

void GenericCommandBuffer::add(const GenericCommandBuffer& other) {
    _data.insert(std::end(_data),
                 std::cbegin(other._data),
                 std::cend(other._data));

    rebuildCaches();
}

inline vectorImpl<std::shared_ptr<Command>>& GenericCommandBuffer::operator()() {
    return _data;
}

inline const vectorImpl<std::shared_ptr<Command>>& GenericCommandBuffer::operator()() const {
    return _data;
}

inline const vectorImpl<Pipeline*>& GenericCommandBuffer::getPipelines() const {
    return _pipelineCache;
}

const vectorImpl<GenericDrawCommand*>& GenericCommandBuffer::getDrawCommands() const {
    return _drawCommandsCache;
}

void GenericCommandBuffer::clear() {
    _data.clear();
    _pipelineCache.clear();
    _drawCommandsCache.clear();
}

}; //namespace Divide
#endif //_GENERIC_DRAW_COMMAND_INL