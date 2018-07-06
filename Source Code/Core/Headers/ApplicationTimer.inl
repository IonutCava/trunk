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

#ifndef _CORE_APPLICATION_TIMER_INL_
#define _CORE_APPLICATION_TIMER_INL_

namespace Divide {
namespace Time {

inline void ApplicationTimer::benchmark(bool state) { _benchmark = state; }

inline bool ApplicationTimer::benchmark() const { return _benchmark; }

inline F32 ApplicationTimer::getFps() const { return _frameRateHandler.frameRate(); }

inline F32 ApplicationTimer::getFrameTime() const { return _frameRateHandler.frameTime(); }

inline F32 ApplicationTimer::getSpeedfactor() const { return _speedfactor; }


inline TimeValue ApplicationTimer::getCurrentTicksInternal() const {
    return std::chrono::high_resolution_clock::now();
}

inline U64 ApplicationTimer::getElapsedTimeInternal(const TimeValue& currentTicks) const {
    return static_cast<U64>(std::chrono::duration_cast<USec>(currentTicks - _startupTicks).count());
}

inline U64 ApplicationTimer::getElapsedTime(bool forceUpdate) {
    if (forceUpdate) {
        return getElapsedTimeInternal(getCurrentTicksInternal());
    }
     
    return _elapsedTimeUs;
}

inline F32 FRAME_SPEED_FACTOR() {
    return ApplicationTimer::getInstance().getSpeedfactor();
}

};  // namespace Time
};  // namespace Divide

#endif  //_CORE_APPLICATION_TIMER_INL_