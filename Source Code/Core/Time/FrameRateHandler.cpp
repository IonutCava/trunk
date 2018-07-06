#include "stdafx.h"

#include "Headers/FrameRateHandler.h"
#include "Headers/ApplicationTimer.h"

namespace Divide {
namespace Time {

FrameRateHandler::FrameRateHandler() : _fps(0.0f),
    _frameCount(0),
    _averageFps(0.0f),
    _averageFpsCount(0UL),
    _minFPS(0.0f),
    _maxFPS(0.0f),
    _tickTimeStamp(0UL),
    _targetFrameRate(0)
{
    memset(&_framerateSecPerFrame, 0, sizeof(F32) * FRAME_ARRAY_SIZE);
    _previousElapsedSeconds = 0.0f;
    _framerate = 0.0f;
    _framerateSecPerFrameIdx = 0;
    _framerateSecPerFrameAccum = 0.0f;
}

FrameRateHandler::~FrameRateHandler()
{
}

void FrameRateHandler::init(U32 targetFrameRate)
{
    _targetFrameRate = targetFrameRate;
    reset();  
}

void FrameRateHandler::reset() {
    _fps = to_F32(_targetFrameRate) * 0.5f;
    _frameCount = 0;
    _averageFps = 0.0f;
    _averageFpsCount = 0;
    _tickTimeStamp = 0;
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

    F32 elapsedSeconds = Time::MicrosecondsToSeconds(elapsedTime);
    F32 deltaSeconds = elapsedSeconds - _previousElapsedSeconds;
    _framerateSecPerFrameAccum += deltaSeconds - _framerateSecPerFrame[_framerateSecPerFrameIdx];
    _framerateSecPerFrame[_framerateSecPerFrameIdx] = deltaSeconds;
    _framerateSecPerFrameIdx = (_framerateSecPerFrameIdx + 1) % FRAME_ARRAY_SIZE;
    _framerate = 1.0f / (_framerateSecPerFrameAccum / (F32)FRAME_ARRAY_SIZE);
    _previousElapsedSeconds = elapsedSeconds;
}

}; //namespace Time
}; //namespace Divide