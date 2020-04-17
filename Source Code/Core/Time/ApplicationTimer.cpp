#include "stdafx.h"

#include "config.h"

#include "Headers/ApplicationTimer.h"
#include "Headers/ProfileTimer.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

#include "Core/Headers/StringHelper.h"

namespace Divide {
namespace Time {

ApplicationTimer::ApplicationTimer() noexcept
{
    std::atomic_init(&_elapsedTimeUs, 0ULL);
    _startupTicks = getCurrentTicksInternal();
    _frameDelay = _startupTicks;
}

void ApplicationTimer::update() {
    const TimeValue currentTicks = getCurrentTicksInternal();
    _elapsedTimeUs = getElapsedTimeInternal(currentTicks);

    const USec duration = std::chrono::duration_cast<USec>(currentTicks - _frameDelay);
    _speedfactor = Time::MicrosecondsToSeconds<F32>(static_cast<U64>(duration.count() * _targetFrameRate));

    _frameDelay = currentTicks;

    const U64 elapsedTime = getElapsedTimeInternal(currentTicks);
    _frameRateHandler.tick(elapsedTime);
    
    if_constexpr (Config::Profile::BENCHMARK_PERFORMANCE) {

        if (elapsedTime - _lastBenchmarkTimeStamp > Time::MillisecondsToMicroseconds(Config::Profile::BENCHMARK_FREQUENCY))
        {
            F32 fps = 0.f;
            F32 frameTime = 0.f;
            _frameRateHandler.frameRateAndTime(fps, frameTime);

            _lastBenchmarkTimeStamp = elapsedTime;
            _lastBenchmarkReport = 
                Util::StringFormat(Locale::get(_ID("FRAMERATE_FPS_OUTPUT")),
                                   fps,
                                   _frameRateHandler.averageFrameRate(),
                                   _frameRateHandler.maxFrameRate(),
                                   _frameRateHandler.minFrameRate(),
                                   frameTime);
        }
    }
}

};  // namespace Time
};  // namespace Divide
