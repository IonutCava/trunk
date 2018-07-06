#include "stdafx.h"

#include "config.h"

#include "Headers/ApplicationTimer.h"
#include "Headers/ProfileTimer.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

#include "Core/Headers/StringHelper.h"

namespace Divide {
namespace Time {

ApplicationTimer::ApplicationTimer()
    : _targetFrameRate(Config::TARGET_FRAME_RATE),
      _speedfactor(1.0f),
      _elapsedTimeUs(0UL),
      _lastBenchmarkTimeStamp(0UL)
{
    _startupTicks = getCurrentTicksInternal();
    _frameDelay = _startupTicks;
    _frameRateHandler.init(_targetFrameRate);
}

ApplicationTimer::~ApplicationTimer()
{
}

void ApplicationTimer::update() {
    TimeValue currentTicks = getCurrentTicksInternal();
    _elapsedTimeUs = getElapsedTimeInternal(currentTicks);

    USec duration = std::chrono::duration_cast<USec>(currentTicks - _frameDelay);
    _speedfactor = Time::MicrosecondsToSeconds<F32>(static_cast<U64>(duration.count() * _targetFrameRate));

    _frameDelay = currentTicks;

    U64 elapsedTime = getElapsedTimeInternal(currentTicks);
    _frameRateHandler.tick(elapsedTime);
    
    if (Config::Profile::BENCHMARK_PERFORMANCE) {

        if (elapsedTime - _lastBenchmarkTimeStamp > Time::MillisecondsToMicroseconds(Config::Profile::BENCHMARK_FREQUENCY))
        {
            _lastBenchmarkTimeStamp = elapsedTime;
            _lastBenchmarkReport = 
                Util::StringFormat(Locale::get(_ID("FRAMERATE_FPS_OUTPUT")),
                                   _frameRateHandler.averageFrameRate(),
                                   _frameRateHandler.maxFrameRate(),
                                   _frameRateHandler.minFrameRate(),
                                   1000.0f / _frameRateHandler.averageFrameRate());
        }
    }
}

};  // namespace Time
};  // namespace Divide
