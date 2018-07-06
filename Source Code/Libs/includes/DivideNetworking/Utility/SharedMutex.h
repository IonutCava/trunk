/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SHARED_MUTEX_X_
#define _SHARED_MUTEX_X_

#include "Thread.h"

///Thread safety optimised for multiple-reades, single write
typedef boost::shared_mutex SharedLock;
typedef boost::unique_lock< SharedLock > WriteLock;
typedef boost::shared_lock< SharedLock > ReadLock;
typedef boost::upgrade_lock< SharedLock > UpgradableReadLock;
typedef boost::upgrade_to_unique_lock< SharedLock > UpgradeToWriteLock;

template<typename T> struct synchronized
{
public:
  synchronized& operator=(T const& newval)
  {
    ReadLock lock(mutex);
    value = newval;
  }
  operator T() const
  {
    WriteLock lock(mutex);
    return value;
  }
private:
  T value;
  SharedLock mutex;
};

#endif