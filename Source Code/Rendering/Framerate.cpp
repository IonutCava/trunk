#include "Headers/Framerate.h"
#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

#if defined(_DEBUG) || defined(_PROFILE)
ProfileTimer::ProfileTimer() : _init(false), _timer(0.0), _timerAverage(0.0)
{
}

ProfileTimer::~ProfileTimer()
{
    Framerate::getInstance().removeTimer(this);
    SAFE_DELETE(_name);
}

void ProfileTimer::start(){
    _timerAverage = _timerAverage + _timer;
    _timerCounter++;
    _timer = Framerate::getInstance().getElapsedTime();
}

void ProfileTimer::stop(){
    _timer = Framerate::getInstance().getElapsedTime() - _timer;
}

void ProfileTimer::create(const char* name){
    if(_init)
        return;

    _name = strdup(name);
    _init = true;
    // should never be called twice for the same object
    Framerate::getInstance().addTimer(this);
}

void ProfileTimer::print() const {
    PRINT_FN("[ %s ] : [ %5.3f ms]", _name, _timerAverage / _timerCounter);
}

void Framerate::addTimer(ProfileTimer* const timer) {
    _profileTimers.push_back(timer);
}

void Framerate::removeTimer(ProfileTimer* const timer) {
    for(vectorImpl<ProfileTimer* >::iterator it = _profileTimers.begin(); it != _profileTimers.end(); ++it){
        if(strcmp((*it)->name(), timer->name()) == 0){
            _profileTimers.erase(it);
            return;
        }
    }
}
#endif

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
    static U32 count = 0;
    static U32 averageCount = 0;
    static F32 maxFps = std::numeric_limits<F32>::min();
    static F32 minFps = std::numeric_limits<F32>::max();
    static F32 averageFps = 0.0f;
    static F32 averageFpsTotal = 0.0f;
};

void Framerate::benchmarkInternal(){
    if(!_benchmark)
        return;

    //Average FPS
    averageFps += _fps;
    averageCount++;

    //Min/Max FPS (after every target second)
    if(count > _targetFrameRate){
        maxFps = Util::max(maxFps, _fps);
        minFps = Util::min(minFps, _fps);
    }

    //Every 10 seconds (targeted)
    if(count > _targetFrameRate * 10){
        F32 avgFPS = averageFpsTotal / averageCount;
        PRINT_FN(Locale::get("FRAMERATE_FPS_OUTPUT"), avgFPS, maxFps, minFps, 1000.0f / avgFPS);
        averageFpsTotal += averageFps;
#if defined(_DEBUG) || defined(_PROFILE)
        for(vectorImpl<ProfileTimer* >::iterator it = _profileTimers.begin(); it != _profileTimers.end(); ++it){
            (*it)->print();
        }
#endif
        averageFps = 0;
        count = 0;
    }
    ++count;
}