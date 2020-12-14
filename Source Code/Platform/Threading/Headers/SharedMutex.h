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
#ifndef _SHARED_MUTEX_BOOST_H_
#define _SHARED_MUTEX_BOOST_H_

//#define USE_BOOST_LOCKS

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#if defined(USE_BOOST_LOCKS)
namespace thread = boost;
#else
#include <shared_mutex>
namespace thread = std;
#endif


namespace Divide {

using Mutex = thread::mutex;
using TimedMutex = thread::timed_mutex;

using SharedMutex = thread::shared_mutex;
using SharedTimedMutex = thread::shared_timed_mutex;

template<typename T>
using SharedLock = thread::shared_lock<T>;

template<typename T>
using UniqueLock = thread::unique_lock<T>;

using UpgradeMutex = boost::shared_mutex;
using UpgradeWriteLock = boost::unique_lock<UpgradeMutex>;
using UpgradeReadLock = boost::shared_lock<UpgradeMutex>;
using UpgradeLock = boost::upgrade_lock<UpgradeMutex>;
using UpgradeUniqueLock = boost::upgrade_to_unique_lock<UpgradeMutex>;

template<typename T>
struct ScopedLock final : NonCopyable, NonMovable
{
    explicit ScopedLock(T& mutex, const bool lock, const bool shared)
        : _mutex(mutex),
          _lock(lock),
          _shared(shared)
    {
        if (_lock) {
            if (_shared) {
                _mutex.lock_shared();
            } else {
                _mutex.lock();
            }
        }
    }

    ~ScopedLock()
    {
        if (_lock) {
            if (_shared) {
                _mutex.unlock_shared();
            } else {
                _mutex.unlock();
            }
        }
    }
private:
    T& _mutex;
    const bool _lock = true;
    const bool _shared = true;
};

};  // namespace Divide

#endif //_SHARED_MUTEX_BOOST_H_