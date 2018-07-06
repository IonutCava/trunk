/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _MUTEX_X_
#define _MUTEX_X_

#include "Thread.h"

///Thread safety optimised for multiple-reades, single write
typedef boost::shared_mutex Lock;
typedef boost::unique_lock< Lock > WriteLock;
typedef boost::shared_lock< Lock > ReadLock;
typedef boost::upgrade_lock< Lock > UpgradableReadLock; 
typedef boost::upgrade_to_unique_lock< Lock > UpgradeToWriteLock; 


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
  Lock mutex;
};

#endif