#include "Headers/ProfileTimer.h"
#include "Headers/ApplicationTimer.h"
#include "Headers/Console.h"

namespace Divide {
namespace Time {

ScopedTimer::ScopedTimer(ProfileTimer& timer) : _timer(timer)
{
    _timer.start();
}

ScopedTimer::~ScopedTimer()
{
    _timer.stop();
}

ProfileTimer::ProfileTimer() {
    _init = false;
    _paused = false;
    _timer = 0.0;
#if defined(_DEBUG) || defined(_PROFILE)
    _timerAverage = 0.0;
    _timerCounter = 0;
#endif
}

ProfileTimer::~ProfileTimer()
{
    ApplicationTimer::instance().removeTimer(this);
}

void ProfileTimer::reset() {
#if defined(_DEBUG) || defined(_PROFILE)
    _timerAverage = 0.0;
    _timerCounter = 0;
#endif
}

void ProfileTimer::start() {
#if defined(_DEBUG) || defined(_PROFILE)
    _timer = Time::MicrosecondsToMilliseconds<D32>(
        ApplicationTimer::instance().getElapsedTime(true));
#endif
}

void ProfileTimer::stop() {
#if defined(_DEBUG) || defined(_PROFILE)
    if (_paused) {
        reset();
        return;
    }

    _timer = Time::MicrosecondsToMilliseconds<D32>(
                 ApplicationTimer::instance().getElapsedTime(true)) -
             _timer;
    _timerAverage = _timerAverage + _timer;
    _timerCounter++;
#endif
}

void ProfileTimer::create(const stringImpl& name) {
    if (_init) {
        return;
    }

    _name = name;
    _init = true;
    // should never be called twice for the same object
    ApplicationTimer::instance().addTimer(this);
}

void ProfileTimer::print() const {
#if defined(_DEBUG) || defined(_PROFILE)
    if (!_paused) {
        Console::printfn("[ %s ] : [ %5.3f ms]", _name.c_str(),
                         _timerAverage / _timerCounter);
    }
#endif
}

};  // namespace Time
};  // namespace Divide