#include "stdafx.h"

#include "Headers/FrameRateHandler.h"

namespace Divide {
namespace Time {


void FrameRateHandler::tick(const U64 deltaTimeUS) noexcept {
    const F32 elapsedSeconds = Time::MicrosecondsToSeconds<F32>(deltaTimeUS);
    const F32 deltaSeconds = elapsedSeconds - _previousElapsedSeconds;
    _framerateSecPerFrameAccum += deltaSeconds - _framerateSecPerFrame[_framerateSecPerFrameIdx];
    _framerateSecPerFrame[_framerateSecPerFrameIdx] = deltaSeconds;
    _framerateSecPerFrameIdx = (_framerateSecPerFrameIdx + 1) % FRAME_ARRAY_SIZE;
    _framerate = 1.0f / (_framerateSecPerFrameAccum / to_F32(FRAME_ARRAY_SIZE));
    _previousElapsedSeconds = elapsedSeconds;

    _averageFPS += _framerate;

    if (_frameCount > FRAME_AVG_DELAY_COUNT) {
        _maxFPS = std::max(_framerate, _maxFPS);
        _minFPS = std::min(_framerate, _minFPS);
    }

    // Min/max frame rate
    if (++_frameCount > FRAME_AVG_RESET_COUNT) {
        _averageFPS = 0.0f;
        _frameCount = 0;
        _minFPS = std::numeric_limits<F32>::max();
        _maxFPS = std::numeric_limits<F32>::lowest();
    }
}

}; //namespace Time
}; //namespace Divide