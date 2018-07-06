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
#ifndef _CORE_TIME_PROFILE_TIMER_H_
#define _CORE_TIME_PROFILE_TIMER_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {
namespace Time {

class ApplicationTimer;
class ProfileTimer {
   public:
    ProfileTimer();
    ~ProfileTimer();

    void start();
    void stop();
    void reset();
    stringImpl print(U32 level = 0) const;

    U64 get() const;
    U64 getChildTotal() const;
    const stringImpl& name() const;

    static stringImpl printAll();
    static ProfileTimer& getNewTimer(const char* timerName);
    static void removeTimer(ProfileTimer& timer);

    static U64 overhead();

   // time data
   protected:
    stringImpl _name;
    U64 _timer;
    U64 _timerAverage;
    U32 _timerCounter;
    U32 _globalIndex;

   // timer <-> timer relationship
   public:
    void addChildTimer(ProfileTimer& child);
    void removeChildTimer(ProfileTimer& child);

   protected:
     vectorImpl<U32> _children;
     U32 _parent;
     ApplicationTimer& _appTimer;
};

class ScopedTimer : private NonCopyable {
public:
    explicit ScopedTimer(ProfileTimer& timer);
    ~ScopedTimer();

private:
    ProfileTimer& _timer;
};

ProfileTimer& ADD_TIMER(const char* timerName);
void REMOVE_TIMER(ProfileTimer*& timer);

void START_TIMER(ProfileTimer& timer);
void STOP_TIMER(ProfileTimer& timer);
void PRINT_TIMER(ProfileTimer& timer);

};  // namespace Time
};  // namespace Divide

#endif  //_CORE_TIME_PROFILE_TIMER_H_

#include "ProfileTimer.inl"
