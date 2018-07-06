#include "Headers/ApplicationTimer.h"
#include "Headers/ProfileTimer.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

#pragma message("DIVIDE Framework uses U64 (unsigned long long) data types for timing with microsecond resolution!")
#pragma message("Use apropriate conversion in time sensitive code (see ApplicationTimer.h)")

namespace Divide {
namespace Time {

ApplicationTimer::ApplicationTimer() : _targetFrameRate(Config::TARGET_FRAME_RATE),
                                       _ticksPerMicrosecond(0.0),
                                       _speedfactor(1.0f),
                                       _elapsedTimeUs(0ULL),
                                       _init(false),
                                       _benchmark(false)
{
    _ticksPerSecond.QuadPart = 0LL;
    _frameDelay.QuadPart = 0LL;
    _startupTicks.QuadPart = 0LL;
}

ApplicationTimer::~ApplicationTimer()
{
}

void ApplicationTimer::addTimer(ProfileTimer* const timer) {
    _profileTimers.push_back(timer);
}

void ApplicationTimer::removeTimer(ProfileTimer* const timer) {
    const stringImpl& timerName = timer->name();
    _profileTimers.erase(std::remove_if(_profileTimers.begin(), _profileTimers.end(),
        [&timerName](ProfileTimer* tTimer)->bool {
        return tTimer->name().compare(timerName) == 0;
    }),
        _profileTimers.end());
}

///No need for init to be threadsafe
void ApplicationTimer::init(U8 targetFrameRate) {
    assert(!_init);//<prevent double init

    _targetFrameRate = static_cast<U32>(targetFrameRate);

#if defined( OS_WINDOWS )
    bool queryAvailable = QueryPerformanceFrequency(&_ticksPerSecond) != 0;
    DIVIDE_ASSERT(queryAvailable, "Current system does not support 'QueryPerformanceFrequency calls!");
    QueryPerformanceCounter(&_startupTicks);
#else
    gettimeofday(&_startupTicks, nullptr);
#endif

    _ticksPerMicrosecond = static_cast<D32>(_ticksPerSecond.QuadPart / 1000000.0);
    _frameDelay = _startupTicks;
    _init = true;
}

ApplicationTimer::LI ApplicationTimer::getCurrentTicksInternal() const {
    LI currentTicks;
    currentTicks.QuadPart = 0;
#if defined( OS_WINDOWS )
    QueryPerformanceCounter(&currentTicks);
#else
    gettimeofday(&currentTicks,nullptr);
#endif
    return currentTicks;
}

U64 ApplicationTimer::getElapsedTimeInternal(LI currentTicks) const {
    return static_cast<U64>((currentTicks.QuadPart - _startupTicks.QuadPart) / _ticksPerMicrosecond);
}

void ApplicationTimer::update(U32 frameCount) {
    LI currentTicks = getCurrentTicksInternal();
    _elapsedTimeUs = getElapsedTimeInternal(currentTicks);

    _speedfactor = static_cast<F32>((currentTicks.QuadPart - _frameDelay.QuadPart) /
        (_ticksPerSecond.QuadPart / static_cast<F32>(_targetFrameRate)));
    _frameDelay = currentTicks;

    if (_speedfactor <= 0.0f) {
        _speedfactor = 1.0f;
    }

    _fps = _targetFrameRate / _speedfactor;
    _frameTime = 1000.0 / _fps;

    benchmarkInternal(frameCount);
}

namespace {
    static U32 g_averageCount = 0;
    static F32 g_maxFps = std::numeric_limits<F32>::min();
    static F32 g_minFps = std::numeric_limits<F32>::max();
    static F32 g_averageFps = 0.0f;
    static F32 g_averageFpsTotal = 0.0f;
};

void ApplicationTimer::benchmarkInternal(U32 frameCount){
    if (!_benchmark) {
        return;
    }

    //Average FPS
    g_averageFps += _fps;
    g_averageCount++;

    //Min/Max FPS (after every target second)
    if (frameCount % _targetFrameRate == 0) {
        g_maxFps = std::max(g_maxFps, _fps);
        g_minFps = std::min(g_minFps, _fps);
    }

    //Every 10 seconds (targeted)
    if (frameCount % (_targetFrameRate * 10) == 0) {
        g_averageFpsTotal += g_averageFps;

        F32 avgFPS = g_averageFpsTotal / g_averageCount;
        Console::printfn(Locale::get("FRAMERATE_FPS_OUTPUT"), avgFPS, g_maxFps, g_minFps, 1000.0f / avgFPS);

#        if defined(_DEBUG) || defined(_PROFILE)
        for (ProfileTimer* const timer : _profileTimers) {
            timer->print();
            timer->reset();
        }

#    endif
        g_averageFps = 0;
    }
}

}; //namespace Time
}; //namespace Divide