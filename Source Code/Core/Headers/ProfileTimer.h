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

#ifndef _CORE_PROFILE_TIMER_H_
#define _CORE_PROFILE_TIMER_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {
namespace Time {

class ProfileTimer {
   public:
    ProfileTimer();
    ~ProfileTimer();

    void start();
    void stop();
    void pause(const bool state);
    void reset();
    void print() const;

    inline D64 get() const { return _timer; }
    inline const stringImpl& name() const { return _name; }

    static void printAll();
    static ProfileTimer& getNewTimer(const char* timerName);
    static void removeTimer(ProfileTimer& timer);

   protected:
    stringImpl _name;
    D64 _timer;
    D64 _timerAverage;
    U32 _timerCounter;
    U32 _globalIndex;
    bool _paused;
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

#endif  //_CORE_PROFILE_TIMER_H_

#include "ProfileTimer.inl"
