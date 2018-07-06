/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _SHARED_MUTEX_X_
#define _SHARED_MUTEX_X_

#include "Thread.h"
#include <mutex>

namespace Divide {

/// Thread safety optimised for multiple-reades, single write
typedef boost::shared_mutex SharedLock;
typedef boost::unique_lock<SharedLock> WriteLock;
typedef boost::shared_lock<SharedLock> ReadLock;
typedef boost::upgrade_lock<SharedLock> UpgradableReadLock;
typedef boost::upgrade_to_unique_lock<SharedLock> UpgradeToWriteLock;

template <typename T>
struct synchronized {
   public:
    synchronized& operator=(T const& newval) {
        ReadLock lock(mutex);
        value = newval;
    }
    operator T() const {
        WriteLock lock(mutex);
        return value;
    }

   private:
    T value;
    SharedLock mutex;
};

};  // namespace Divide

#endif