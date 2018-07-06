#include "Headers/ApplicationTimer.h"
#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

#pragma message("DIVIDE Framework uses U64 (unsigned long long) data types for timing with microsecond resolution!")
#pragma message("Use apropriate conversion in time sensitive code (see core.h)")

namespace Divide {

#if defined(_DEBUG) || defined(_PROFILE)
ProfileTimer::ProfileTimer()
{
	_init = false;
	_paused = false;
	_timer = 0.0;
	_timerAverage = 0.0;
	_timerCounter = 0;
}

ProfileTimer::~ProfileTimer()
{
    ApplicationTimer::getInstance().removeTimer(this);
    SAFE_DELETE(_name);
}

void ProfileTimer::reset() {
    _timerAverage = 0.0;
    _timerCounter = 0;
}

void ProfileTimer::start(){
    _timer = getUsToMs(ApplicationTimer::getInstance().getElapsedTime(true));
}

void ProfileTimer::stop(){
    if(_paused) {
        reset();
        return;
    }
    _timer = getUsToMs(ApplicationTimer::getInstance().getElapsedTime(true)) - _timer;
    _timerAverage = _timerAverage + _timer;
    _timerCounter++;
}

void ProfileTimer::create(const char* name){
    if(_init) return;

    _name = strdup(name);
    _init = true;
    // should never be called twice for the same object
    ApplicationTimer::getInstance().addTimer(this);
}

void ProfileTimer::print() const {
    if(!_paused)
        PRINT_FN("[ %s ] : [ %5.3f ms]", _name, _timerAverage / _timerCounter);
}

void ApplicationTimer::addTimer(ProfileTimer* const timer) {
    _profileTimers.push_back(timer);
}

void ApplicationTimer::removeTimer(ProfileTimer* const timer) {
    for(vectorImpl<ProfileTimer* >::iterator it = _profileTimers.begin(); it != _profileTimers.end(); ++it){
        if(strcmp((*it)->name(), timer->name()) == 0){
            _profileTimers.erase(it);
            return;
        }
    }
}
#endif

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
    _currentTicks.QuadPart = 0LL;
}
#ifdef min
#undef min
#undef max
#endif
///No need for init to be threadsafe
void ApplicationTimer::init(U8 targetFrameRate) {
    assert(!_init);//<prevent double init

    _targetFrameRate = static_cast<U32>(targetFrameRate);

#if defined( OS_WINDOWS )
    if(!QueryPerformanceFrequency(&_ticksPerSecond))
        assert(false && "Current system does not support 'QueryPerformanceFrequency calls!");

    QueryPerformanceCounter(&_startupTicks);
#else
    gettimeofday(&_startupTicks,nullptr);
#endif

    _ticksPerMicrosecond = static_cast<D32>(_ticksPerSecond.QuadPart / 1000000.0);
    _frameDelay = _startupTicks;
    _init = true;
}

namespace {
    static U32 averageCount = 0;
    static F32 maxFps = std::numeric_limits<F32>::min();
    static F32 minFps = std::numeric_limits<F32>::max();
    static F32 averageFps = 0.0f;
    static F32 averageFpsTotal = 0.0f;
};

U64 ApplicationTimer::getElapsedTimeInternal() {
#if defined( OS_WINDOWS )
    QueryPerformanceCounter(&_currentTicks);
#else
    gettimeofday(&_currentTicks,nullptr);
#endif

    return static_cast<U64>((_currentTicks.QuadPart -_startupTicks.QuadPart) / _ticksPerMicrosecond);
}

void ApplicationTimer::update(U32 frameCount){
    _elapsedTimeUs = getElapsedTimeInternal();

    _speedfactor = static_cast<F32>((_currentTicks.QuadPart - _frameDelay.QuadPart) / 
                                    (_ticksPerSecond.QuadPart / static_cast<F32>(_targetFrameRate)));
    _frameDelay = _currentTicks;

    if(_speedfactor <= 0.0f)
       _speedfactor = 1.0f;

    _fps = _targetFrameRate / _speedfactor;
    _frameTime = 1000.0 / _fps;

    if (_benchmark) benchmarkInternal(frameCount);
}

void ApplicationTimer::benchmarkInternal(U32 frameCount){
    //Average FPS
    averageFps += _fps;
    averageCount++;

    //Min/Max FPS (after every target second)
    if(frameCount % _targetFrameRate == 0){
        maxFps = std::max(maxFps, _fps);
        minFps = std::min(minFps, _fps);
    }

    //Every 10 seconds (targeted)
    if(frameCount % (_targetFrameRate * 10) == 0){
        averageFpsTotal += averageFps;

        F32 avgFPS = averageFpsTotal / averageCount;
        PRINT_FN(Locale::get("FRAMERATE_FPS_OUTPUT"), avgFPS, maxFps, minFps, 1000.0f / avgFPS);
#if defined(_DEBUG) || defined(_PROFILE)
        for(vectorImpl<ProfileTimer* >::iterator it = _profileTimers.begin(); it != _profileTimers.end(); ++it){
            (*it)->print();
            (*it)->reset();
        }

#endif
        averageFps = 0;
    }
}

};