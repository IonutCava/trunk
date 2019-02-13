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
#ifndef _COMMAND_BUFFER_H_
#define _COMMAND_BUFFER_H_

#include "Commands.h"
#include "Core/TemplateLibraries/Headers/PolyContainer.h"

namespace Divide {

namespace GFX {

class CommandBuffer : private GUIDWrapper, private NonCopyable {
    friend class CommandBufferPool;
  public:
      typedef PolyContainerEntry CommandEntry;
      typedef PolyContainer<GFX::CommandBase, to_base(GFX::CommandType::COUNT)> Container;
  public:
    CommandBuffer() = default;
    ~CommandBuffer() = default;

    // Just a big ol' collection of vectors, so these should be fine
    CommandBuffer(const CommandBuffer& other) = default;
    CommandBuffer & operator=(const CommandBuffer& other) = default;
    CommandBuffer(CommandBuffer&& other) = default;
    CommandBuffer & operator=(CommandBuffer&& other) = default;

    template<typename T>
    inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, T&>::type
    add(const T& command);

    bool validate() const;

    void add(const CommandBuffer& other);

    void clean();

    void batch();

    // Return true if merge is successful
    template<typename T>
    inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, bool>::type
    tryMergeCommands(GFX::CommandType::_enumerated type, T* prevCommand, T* crtCommand, bool& partial) const;

    template<typename T>
    typename std::enable_if<std::is_base_of<CommandBase, T>::value, const T&>::type
    get(const CommandEntry& commandEntry) const;

    template<typename T>
    typename std::enable_if<std::is_base_of<CommandBase, T>::value, T&>::type
    get(const CommandEntry& commandEntry);

    template<typename T>
    typename std::enable_if<std::is_base_of<CommandBase, T>::value, Container::EntryList&>::type
    get();

    template<typename T>
    typename std::enable_if<std::is_base_of<CommandBase, T>::value, const Container::EntryList&>::type
    get() const;

    bool exists(const CommandEntry& commandEntry) const;

    template<typename T>
    typename std::enable_if<std::is_base_of<CommandBase, T>::value, T&>::type
    get(size_t index);

    template<typename T>
    typename std::enable_if<std::is_base_of<CommandBase, T>::value, const T&>::type
    get(size_t index) const;

    bool exists(vec_size_eastl typeIndex, size_t index) const;

    inline vectorEASTL<CommandEntry>& operator()();
    inline const vectorEASTL<CommandEntry>& operator()() const;

    inline vec_size size() const { return _commandOrder.size(); }
    inline void clear();
    inline bool empty() const;

    // Multi-line. indented list of all commands (and params for some of them)
    stringImpl toString() const;

    template<typename T>
    typename std::enable_if<std::is_base_of<CommandBase, T>::value, size_t>::type
    count() const;

  protected:
    template<typename T, CommandType::_enumerated enumVal>
    friend struct Command;

    void toString(const GFX::CommandBase& cmd, GFX::CommandType::_enumerated type, I32& crtIndent, stringImpl& out) const;

  protected:
    vectorEASTL<CommandEntry> _commandOrder;
    PolyContainer<GFX::CommandBase, to_base(GFX::CommandType::COUNT)> _commands;
};

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, void>::type
EnqueueCommand(CommandBuffer& buffer, T& cmd) {
    buffer.add(cmd);
}

}; //namespace GFX
}; //namespace Divide

#endif //_COMMAND_BUFFER_H_

#include "CommandBuffer.inl"