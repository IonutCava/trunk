#include "Headers/ApplicationTimer.h"
#include "Headers/ProfileTimer.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

#pragma message( \
    "DIVIDE Framework uses U64 (unsigned long long) data types for timing with microsecond resolution!")
#pragma message( \
    "Use apropriate conversion in time sensitive code (see ApplicationTimer.h)")

namespace Divide {
namespace Time {

ApplicationTimer::ApplicationTimer()
    : _targetFrameRate(Config::TARGET_FRAME_RATE),
      _speedfactor(1.0f),
      _init(false),
      _benchmark(false),
      _elapsedTimeUs(0UL)
{
    _ticksPerSecond = static_cast<TimeValue>(0);
    _frameDelay = static_cast<TimeValue>(0);
    _startupTicks = static_cast<TimeValue>(0);
}

ApplicationTimer::~ApplicationTimer()
{
}

void ApplicationTimer::addTimer(ProfileTimer* const timer) {
    _profileTimers.push_back(timer);
}

void ApplicationTimer::removeTimer(ProfileTimer* const timer) {
    const stringImpl& timerName = timer->name();
    _profileTimers.erase(
        std::remove_if(std::begin(_profileTimers), std::end(_profileTimers),
                       [&timerName](ProfileTimer* tTimer) -> bool {
                           return tTimer->name().compare(timerName) == 0;
                       }),
        std::end(_profileTimers));
}

/// No need for init to be threadsafe
void ApplicationTimer::init(U8 targetFrameRate) {
    assert(!_init);  //<prevent double init

    _targetFrameRate = to_uint(targetFrameRate);
    getTicksPerSecond(_ticksPerSecond);
    getCurrentTime(_startupTicks);
    _startupTicks =  std::max(_startupTicks, static_cast<TimeValue>(0));
    _frameDelay = _startupTicks;
    _init = true;
}

TimeValue ApplicationTimer::getCurrentTicksInternal() const {
    TimeValue currentTicks;
    getCurrentTime(currentTicks);
    return currentTicks;
}

U64 ApplicationTimer::getElapsedTimeInternal(TimeValue currentTicks) const {
    return Time::SecondsToMicroseconds((currentTicks - _startupTicks) / _ticksPerSecond);
}

void ApplicationTimer::update(U32 frameCount) {
    TimeValue currentTicks = getCurrentTicksInternal();
    _elapsedTimeUs = getElapsedTimeInternal(currentTicks);

    _speedfactor = to_float(currentTicks - _frameDelay) /
                   (_ticksPerSecond / to_float(_targetFrameRate));

    CLAMP<F32>(_speedfactor, 0.0f, 1.0f);

    _frameDelay = currentTicks;

    _fps = _targetFrameRate / _speedfactor;
    _frameTime = 1000.0f / _fps;

    benchmarkInternal(frameCount);
}

namespace {
    static U32 g_averageCount = 0;
    static F32 g_averageFps = 0.0f;
    static F32 g_averageFpsTotal = 0.0f;
    static F32 g_maxFps = std::numeric_limits<F32>::min();
    static F32 g_minFps = std::numeric_limits<F32>::max();
};

void ApplicationTimer::benchmarkInternal(U32 frameCount) {
    if (!_benchmark) {
        return;
    }

    // Average FPS
    g_averageFps += _fps;
    g_averageCount++;

    // Min/Max FPS (Every 5 seconds (targeted))
    if (frameCount % (_targetFrameRate * 5) == 0) {
        g_maxFps = std::max(g_maxFps, _fps);
        g_minFps = std::min(g_minFps, _fps);
    }

    // Every 10 seconds (targeted)
    if (frameCount % (_targetFrameRate * 10) == 0) {
        g_averageFpsTotal += g_averageFps;

        F32 avgFPS = g_averageFpsTotal / g_averageCount;
        Console::printfn(Locale::get("FRAMERATE_FPS_OUTPUT"), avgFPS, g_maxFps,
                         g_minFps, 1000.0f / avgFPS);

#if defined(_DEBUG) || defined(_PROFILE)
        for (ProfileTimer* const timer : _profileTimers) {
            timer->print();
            timer->reset();
        }
#endif
        g_averageFps = 0;
    }
}

};  // namespace Time
};  // namespace Divide
