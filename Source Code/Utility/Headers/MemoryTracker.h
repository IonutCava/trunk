/*
Copyright (c) 2016 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _MEMORY_TRACKER_H_
#define _MEMORY_TRACKER_H_

#include "Core/Math/Headers/MathHelper.h"
#include <atomic>

namespace Divide {
// not totally compatible with smart pointers. Oh well ... -Ionut
namespace MemoryManager {
/// From https://www.relisoft.com/book/tech/9new.html
class MemoryTracker {
   private:
    class Entry {
       public:
        Entry(char const* file, size_t line, size_t size)
            : _file(file),
              _line(line),
              _size(size)
        {
        }

        Entry()
            : Entry("", 0, 0)
        {
        }

        inline char const* File() const {
            return _file;
        }

        inline size_t Line() const {
            return _line;
        }

        inline size_t Size() const {
            return _size;
        }

       private:
        char const* _file;
        size_t _line;
        size_t _size;
    };

    class Lock {
       public:
        Lock(MemoryTracker& tracer) 
            : _tracer(tracer)
        {
            _tracer.lock();
        }
        ~Lock()
        {
            _tracer.unlock();
        }

       private:
        MemoryTracker& _tracer;
    };

    typedef hashMapImplAligned<void*, Entry>::iterator iterator;
    typedef hashMapImplAligned<void*, Entry>::const_iterator citerator;
    friend class Lock;

   public:
    MemoryTracker()
        : _locked(false)
    {
        Ready = true;
    }

    ~MemoryTracker()
    {
        Ready = false;
        bool leakDetected = false;
        size_t sizeLeaked = 0;
        Dump(leakDetected, sizeLeaked);
    }

    inline void Add(void* p, size_t size, char const* file, size_t line) {
        if (!_locked) {
            WriteLock w_lock(_mutex);
            MemoryTracker::Lock lock(*this);
            hashAlg::emplace(_map, p, Entry(file, line, size));
        }
    }

    inline void Remove(void* p) {
        if (!_locked) {
            if (!MemoryTracker::LogAllAllocations) {
                WriteLock w_lock(_mutex);
                MemoryTracker::Lock lock(*this);
                iterator it = _map.find(p);
                if (it != std::cend(_map)) {
                    _map.erase(it);
                } else {
                    assert(false && "Called custom delete on non-custom allocated pointer!");
                }
            }
        }
    }

    inline stringImpl Dump(bool& leakDetected, size_t& sizeLeaked) {
        stringImpl output;
        sizeLeaked = 0;
        leakDetected = false;

        MemoryTracker::Lock lock(*this);
        WriteLock w_lock(_mutex);
        if (!_map.empty()) {
            stringImpl msg = "";
            if (MemoryTracker::LogAllAllocations) {
                msg = "memory allocations detected";
            } else {
                msg = "memory leaks detected";
            }
            output.append(Util::StringFormat("%d %s\n", _map.size(), msg.c_str()));
            size_t totalUsage = 0;
            for (citerator it = std::cbegin(_map); 
                           it != std::cend(_map);
                           ++it)
            {
                const Entry& entry = it->second;
                size_t crtSize = entry.Size();
                totalUsage += crtSize;

                output.append(entry.File());
                output.append(", ");
                output.append(to_stringImpl(entry.Line()).c_str());
                output.append(Util::StringFormat("( %d bytes / %d bytes (%5.2f Mb)) \n", crtSize, totalUsage, (totalUsage / 1024.0f) / 1024));
                sizeLeaked += entry.Size();
            }
            leakDetected = true;
            _map.clear();
        }
        if (!MemoryTracker::LogAllAllocations && sizeLeaked > 0) {
            output.append("Total leaked bytes: ");
            output.append(to_stringImpl(sizeLeaked).c_str());
            output.append("\n");
        } 
        return output;
    }

    static bool Ready;
    static bool LogAllAllocations;

   private:
    inline void lock() {
        _locked = true;
    }

    inline void unlock() {
        _locked = false;
    }

   private:
    mutable SharedLock _mutex;
    hashMapImplAligned<void*, Entry> _map;
    std::atomic_bool _locked;
};

extern MemoryTracker AllocTracer;
};  // namespace MemoryManager
};  // namespace Divide
#endif  //_MEMORY_TRACKER_H_