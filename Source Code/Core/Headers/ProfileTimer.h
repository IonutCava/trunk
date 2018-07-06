/*
   Copyright (c) 2015 DIVIDE-Studio
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

    void create(const stringImpl& name);
    void start();
    void stop();
    void print() const;
    void reset();

    inline D32 get() const { return _timer; }
    inline bool init() const { return _init; }
    inline void pause(const bool state) { _paused = state; }
    inline const stringImpl& name() const { return _name; }

   protected:
    stringImpl _name;
    std::atomic_bool _paused;
    std::atomic_bool _init;
    std::atomic<D32> _timer;
#if defined(_DEBUG) || defined(_PROFILE)
    std::atomic<D32> _timerAverage;
    std::atomic_int _timerCounter;
#endif
};

};  // namespace Time
};  // namespace Divide

#endif  //_CORE_PROFILE_TIMER_H_

#include "ProfileTimer.inl"
