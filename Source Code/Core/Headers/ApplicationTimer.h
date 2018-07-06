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

#ifndef _CORE_APPLICATION_TIMER_H_
#define _CORE_APPLICATION_TIMER_H_

#include "Core/Math/Headers/MathHelper.h"
#include <chrono>

// Code from http://www.gamedev.net/reference/articles/article1382.asp
// Copyright: "Frame Rate Independent Movement" by Ben Dilts
namespace Divide {
namespace Time {

    namespace {
        struct HighResolutionRimer {
            HighResolutionRimer() : _startTime(takeTimeStamp())
            {
            }

            void restart() {
                _startTime = takeTimeStamp();
            }

            D64 elapsed() const {
                return D64(takeTimeStamp() - _startTime) * 1e-9;
            }

            U64 elapsedNanoseconds() const {
                return takeTimeStamp() - _startTime;
            }
        protected:
            static U64 takeTimeStamp() {
                return U64(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            }
        private:
            U64 _startTime;
        };
    };

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimeValue;

class FrameRateHandler {
    public:
        FrameRateHandler();
        ~FrameRateHandler();

        void tick(const U64 deltaTime);
        void init(U32 targetFrameRate, const U64 startTime);

        inline F32 minFrameRate() const { return _minFPS; }
        inline F32 maxFrameRate() const { return _maxFPS; }
        inline F32 frameRate() const { return _fps; }
        inline F32 frameTime() const { return 1000.0f / frameRate(); }
        inline F32 averageFrameRate() const { return _averageFps; }

    private:
        F32 _fps;
        U32 _frameCount;

        F32 _averageFps;
        U64 _averageFpsCount;

        F32 _minFPS;
        F32 _maxFPS;

        U64 _tickTimeStamp;
};

class ProfileTimer;
DEFINE_SINGLETON(ApplicationTimer)
    typedef std::chrono::microseconds USec;
    typedef std::chrono::milliseconds MSec;
    typedef std::chrono::seconds Sec;

  public:
    void init(U8 targetFrameRate);
    void update();

    void benchmark(bool state);
    bool benchmark() const;

    F32 getFps() const;
    F32 getFrameTime() const;
    F32 getSpeedfactor() const;
    U64 getElapsedTime(bool forceUpdate = false);

  protected:
    ApplicationTimer();
    ~ApplicationTimer();

    void benchmarkInternal(const U64 elapsedTime);

    inline TimeValue getCurrentTicksInternal() const;
    inline U64 getElapsedTimeInternal(const TimeValue& currentTicks) const;

    friend class ProfileTimer;
    void addTimer(ProfileTimer* const timer);
    void removeTimer(ProfileTimer* const timer);
    vectorImpl<ProfileTimer*> _profileTimers;

  private:
    bool _init;
    FrameRateHandler _frameRateHandler;
    F32 _speedfactor;
    U32 _targetFrameRate;
    // Previous frame's time stamp
    TimeValue _frameDelay;
    // Time stamp at class initialization
    TimeValue _startupTicks;
    // Measure average FPS and output max/min/average fps to console
    bool _benchmark;
    U64  _lastBenchmarkTimeStamp;
    std::atomic<U64> _elapsedTimeUs;
END_SINGLETON

/// The following functions force a timer update 
/// (a call to query performance timer).
inline U64 ElapsedMicroseconds(bool forceUpdate = false) {
    return ApplicationTimer::instance().getElapsedTime(forceUpdate);
}

inline D64 ElapsedMilliseconds(bool forceUpdate = false) {
    return MicrosecondsToMilliseconds<D64>(ElapsedMicroseconds(forceUpdate));
}
inline D64 ElapsedSeconds(bool forceUpdate = false) {
    return MicrosecondsToSeconds<D64>(ElapsedMicroseconds(forceUpdate));
}

};  // namespace Time
};  // namespace Divide

#endif  //_CORE_APPLICATION_TIMER_H_

#include "ApplicationTimer.inl"
