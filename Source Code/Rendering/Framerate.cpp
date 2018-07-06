#include "Headers/Framerate.h"
#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

///No need for init to be threadsafe
void Framerate::Init(U8 targetFrameRate) {
    assert(!_init);//<prevent double init

    _targetFrameRate = static_cast<F32>(targetFrameRate);

#if defined( OS_WINDOWS )
    QueryPerformanceCounter(&_startupTicks);
    if(!QueryPerformanceFrequency(&_ticksPerSecond))
        assert("Current system does not support 'QueryPerformanceFrequency calls!");
#elif defined( OS_APPLE ) // Apple OS X
    //??
#else //Linux
    gettimeofday(&_startupTicks,NULL);
#endif

    _ticksPerMillisecond = _ticksPerSecond.QuadPart / 1000;
    _frameDelay = _startupTicks;
    _init = true;
}

void Framerate::SetSpeedFactor(){
    LI currentTicks;

#if defined( OS_WINDOWS )
    QueryPerformanceCounter(&currentTicks);
    QueryPerformanceFrequency(&_ticksPerSecond);
#elif defined( OS_APPLE ) // Apple OS X
    //??
#else
    gettimeofday(&currentTicks,NULL);
#endif

    _ticksPerMillisecond = _ticksPerSecond.QuadPart / 1000;
    _speedfactor = (currentTicks.QuadPart-_frameDelay.QuadPart) / (_ticksPerSecond.QuadPart / (F32)(_targetFrameRate));

    if(_speedfactor <= 0.0f)
       _speedfactor = 1.0f;

    _frameDelay = currentTicks;
    _fps = _targetFrameRate / _speedfactor;
    _frameTime = 1000.0f / _fps;

    benchmarkInternal();
}

namespace {
    static U16 count = 0;
    static F32 maxFps = std::numeric_limits<F32>::min();
    static F32 minFps = std::numeric_limits<F32>::max();
    static F32 averageFps = 0.0f;
    static bool resetAverage = false;
};

void Framerate::benchmarkInternal(){
    if(!_benchmark)
        return;

    //Average FPS
    averageFps += _fps;

    //Min/Max FPS (after every target second)
    if(count > _targetFrameRate){
        maxFps = Util::max(maxFps, _fps);
        minFps = Util::min(minFps, _fps);
    }

    //Every 10 seconds (targeted)
    if(count > _targetFrameRate * 10){
        averageFps /= count;
        PRINT_FN(Locale::get("FRAMERATE_FPS_OUTPUT"), averageFps, maxFps, minFps, 1000.0f / averageFps);
        count = 0;

        if(resetAverage)
            averageFps = 0.0f;

        resetAverage = !resetAverage;
    }
    ++count;
}