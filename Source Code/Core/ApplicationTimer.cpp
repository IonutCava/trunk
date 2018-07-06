#include "Headers/ApplicationTimer.h"
#include "Headers/ProfileTimer.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
namespace Time {

//USE THIS: http://gamedev.stackexchange.com/questions/83159/simple-framerate-counter
ApplicationTimer::ApplicationTimer()
    : _targetFrameRate(Config::TARGET_FRAME_RATE),
      _speedfactor(1.0f),
      _init(false),
      _benchmark(false),
      _elapsedTimeUs(0UL)
{
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
    _startupTicks = getCurrentTicksInternal();
    _frameDelay = _startupTicks;

    _init = true;
}

void ApplicationTimer::update() {
    TimeValue currentTicks = getCurrentTicksInternal();
    _elapsedTimeUs = getElapsedTimeInternal(currentTicks);

    USec duration = std::chrono::duration_cast<USec>(currentTicks - _frameDelay);
    _speedfactor = Time::MicrosecondsToSeconds<F32>(static_cast<U64>(duration.count() * _targetFrameRate));

    _frameDelay = currentTicks;

    _fps = _targetFrameRate / _speedfactor;
    _frameTime = 1000.0f / _fps;

    benchmarkInternal();
}

namespace {

    static const U32 g_minMaxFPSIntervalSec = Time::Seconds(5);
    static const U32 g_averageFPSIntervalSec = Time::Seconds(10);

    static U64 g_frameCount = 0;
    static U32 g_averageCount = 0;
    static F32 g_averageFps = 0.0f;
    static F32 g_averageFpsTotal = 0.0f;
    static F32 g_maxFps = std::numeric_limits<F32>::min();
    static F32 g_minFps = std::numeric_limits<F32>::max();
};

void ApplicationTimer::benchmarkInternal() {
    g_frameCount++;

    if (!_benchmark) {
        return;
    }

    // Average FPS
    g_averageFps += _fps;
    g_averageCount++;

    // Min/Max FPS (Every 5 seconds (targeted))
    if (g_frameCount % (_targetFrameRate * g_minMaxFPSIntervalSec) == 0) {
        g_maxFps = std::max(g_maxFps, _fps);
        g_minFps = std::min(g_minFps, _fps);
    }

    // Every 10 seconds (targeted)
    if (g_frameCount % (_targetFrameRate * g_averageFPSIntervalSec) == 0) {
        g_averageFpsTotal += g_averageFps;

        F32 avgFPS = g_averageFpsTotal / g_averageCount;
        Console::printfn(Locale::get(_ID("FRAMERATE_FPS_OUTPUT")), avgFPS, g_maxFps, g_minFps, 1000.0f / avgFPS);

#if !defined(_RELEASE)
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
