#include "Headers/ApplicationTimer.h"
#include "Headers/ProfileTimer.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
namespace Time {

FrameRateHandler::FrameRateHandler() : _fps(0.0f),
                                       _frameCount(0),
                                       _averageFps(0.0f),
                                       _averageFpsCount(0UL),
                                       _minFPS(0.0f),
                                       _maxFPS(0.0f),
                                       _tickTimeStamp(0UL)
{
}

FrameRateHandler::~FrameRateHandler()
{
}

void FrameRateHandler::init(U32 targetFrameRate, const U64 startTime)
{
    _tickTimeStamp = startTime;
    _fps = to_float(targetFrameRate) * 0.5f;
    _frameCount = 0;
    _averageFps = 0.0f;
    _averageFpsCount = 0;

    _minFPS = _maxFPS = _fps;
}

void FrameRateHandler::tick(const U64 elapsedTime) 
{
    static const F32 smoothing = 0.9f;
    static const F32 fpsLimitDiff = 30.0f;

    _frameCount++;
    U64 timeDiff = elapsedTime - _tickTimeStamp;
    if (timeDiff > Time::MillisecondsToMicroseconds(250)) {
        _tickTimeStamp = elapsedTime;
        F32 newFPS = _frameCount / Time::MicrosecondsToSeconds<F32>(timeDiff);
        // Frame rate
        _fps = (_fps * smoothing) + (newFPS * (1.0f - smoothing));
        _frameCount = 0;

        // Average frame rate
        ++_averageFpsCount;
        _averageFps += (_fps - _averageFps) / _averageFpsCount;

        // Min/max frame rate
        if (IS_IN_RANGE_INCLUSIVE(_averageFps,
                                  _minFPS - fpsLimitDiff,
                                  _maxFPS + fpsLimitDiff)) {
            _maxFPS = std::max(_averageFps, _maxFPS);
            _minFPS = std::min(_averageFps, _minFPS);
        }
    }

}

ApplicationTimer::ApplicationTimer()
    : _targetFrameRate(Config::TARGET_FRAME_RATE),
      _speedfactor(1.0f),
      _init(false),
      _benchmark(false),
      _elapsedTimeUs(0UL),
      _lastBenchmarkTimeStamp(0UL)
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
    _frameRateHandler.init(targetFrameRate, 0);
    _init = true;
}

void ApplicationTimer::update() {
    TimeValue currentTicks = getCurrentTicksInternal();
    _elapsedTimeUs = getElapsedTimeInternal(currentTicks);

    USec duration = std::chrono::duration_cast<USec>(currentTicks - _frameDelay);
    _speedfactor = Time::MicrosecondsToSeconds<F32>(static_cast<U64>(duration.count() * _targetFrameRate));

    _frameDelay = currentTicks;

    U64 elapsedTime = getElapsedTimeInternal(currentTicks);
    _frameRateHandler.tick(elapsedTime);
    benchmarkInternal(elapsedTime);
}

void ApplicationTimer::benchmarkInternal(const U64 elapsedTime) {
    if (!_benchmark) {
        return;
    }

    if (elapsedTime - _lastBenchmarkTimeStamp > Time::SecondsToMicroseconds(10)) {
        _lastBenchmarkTimeStamp = elapsedTime;
        Console::printfn(Locale::get(_ID("FRAMERATE_FPS_OUTPUT")),
                         _frameRateHandler.averageFrameRate(),
                         _frameRateHandler.maxFrameRate(),
                         _frameRateHandler.minFrameRate(),
                         1000.0f / _frameRateHandler.averageFrameRate());

#if !defined(_RELEASE)
        for (ProfileTimer* const timer : _profileTimers) {
            timer->print();
            timer->reset();
        }
#endif
    }
}

};  // namespace Time
};  // namespace Divide
