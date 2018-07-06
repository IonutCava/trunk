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

#if defined(USE_BOOST_POLY)
#include <boost/poly_collection/base_collection.hpp>
#endif

namespace Divide {

#if !defined(USE_BOOST_POLY)
    template<typename T, size_t N>
    struct PolyContainer {

        template<typename U>
        inline typename std::enable_if<std::is_base_of<T, U>::value, void>::type
        insert(size_t index, std::shared_ptr<U> cmd) {
            _collection[index].push_back(cmd);
        }

        template<typename U>
        inline typename std::enable_if<std::is_base_of<T, U>::value, void>::type
            insert(size_t index, std::shared_ptr<const U> cmd) {
            _collection[index].push_back(std::const_pointer_cast<U>(cmd));
        }

        inline T& get(size_t index, size_t entry) {
            return *_collection[index][entry];
        }

        inline const T& get(size_t index, size_t entry) const {
            return *_collection[index][entry];
        }

        inline vec_size_eastl size(size_t index) {
            return _collection[index].size();
        }

        inline void reserve(size_t size) {
            for (auto& col : _collection) {
                col.reserve(size);
            }
        }

        inline void reserve(size_t index, size_t size) {
             _collection[index].reserve(size);
        }

        inline void clear(bool clearMemory = false) {
            if (clearMemory) {
                for (auto& col : _collection) {
                    col.clear();
                }
            } else {
                for (auto& col : _collection) {
                    col.resize(0);
                }
            }
        }

        inline void clear(size_t index, bool clearMemory = false) {
            if (clearMemory) {
                _collection[index].clear();
            }  else {
                _collection[index].resize(0);
            }
        }

        inline bool empty() const {
            for (auto col : _collection) {
                if (!col.empty()) {
                    return false;
                }
            }

            return true;
        }

        std::array<vectorEASTL<std::shared_ptr<T>>, N> _collection;
    };
#endif

namespace GFX {

class CommandBuffer {
    friend class CommandBufferPool;

  public:
    typedef std::pair<GFX::CommandType, vec_size> CommandEntry;

  public:
    CommandBuffer();

    ~CommandBuffer() = default;

    // Just a big ol' collection of vectors, so these should be fine
    CommandBuffer(const CommandBuffer& other) = default;
    CommandBuffer & operator=(const CommandBuffer& other) = default;
    CommandBuffer(CommandBuffer&& other) = default;
    CommandBuffer & operator=(CommandBuffer&& other) = default;

    template<typename T>
    inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, void>::type
    add(const T* command);

    bool validate() const;

    void add(const CommandBuffer& other);

    void clean();

    void batch();

    // Return true if merge is successful
    template<typename T>
    bool tryMergeCommands(T* prevCommand, T* crtCommand) const;

    template<typename T>
    typename std::enable_if<std::is_base_of<CommandBase, T>::value, const T&>::type
    getCommand(const CommandEntry& commandEntry) const;

    inline vectorEASTL<CommandEntry>& operator()();
    inline const vectorEASTL<CommandEntry>& operator()() const;

    inline vec_size size() const { return _commandOrder.size(); }
    inline void clear();
    inline bool empty() const;

    // Multi-line. indented list of all commands (and params for some of them)
    stringImpl toString() const;

  protected:
    template<typename T, CommandType enumVal>
    friend struct Command;

    void toString(const GFX::CommandBase& cmd, I32& crtIndent, stringImpl& out) const;
    bool resetMerge(GFX::CommandType type) const;

#if defined(USE_BOOST_POLY)
    const std::type_info& getType(GFX::CommandType type) const;
#endif

    template<typename T>
    typename std::enable_if<std::is_base_of<CommandBase, T>::value, T*>::type
    getCommandInternal(const CommandEntry& commandEntry);

    template<typename T>
    bool registerType();

  protected:
      vectorEASTL<CommandEntry> _commandOrder;

#if defined(USE_BOOST_POLY)
    boost::base_collection<GFX::CommandBase, dvd_allocator<GFX::CommandBase>> _commands;
#else
    PolyContainer<GFX::CommandBase, to_base(GFX::CommandType::COUNT)> _commands;
#endif
};

template<typename T>
inline typename std::enable_if<std::is_base_of<CommandBase, T>::value, void>::type
EnqueueCommand(CommandBuffer& buffer, T& cmd) {
    cmd.addToBuffer(buffer);
}

}; //namespace GFX
}; //namespace Divide

#endif //_COMMAND_BUFFER_H_

#include "CommandBuffer.inl"