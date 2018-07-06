#include "Headers/ProfileTimer.h"
#include "Headers/ApplicationTimer.h"
#include "Headers/Console.h"

namespace Divide {
namespace Time {

ProfileTimer::~ProfileTimer() {
    ApplicationTimer::getInstance().removeTimer(this);
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

void ProfileTimer::reset() {
#if defined(_DEBUG) || defined(_PROFILE)
    _timerAverage = 0.0;
    _timerCounter = 0;
#endif
}

void ProfileTimer::start() {
#if defined(_DEBUG) || defined(_PROFILE)
#if defined(OS_WINDOWS)
    _oldmask = SetThreadAffinityMask(::GetCurrentThread(), 1);
#endif
    _timer = Time::MicrosecondsToMilliseconds<D32>(
        ApplicationTimer::getInstance().getElapsedTime(true));
#endif
}

void ProfileTimer::stop() {
#if defined(_DEBUG) || defined(_PROFILE)
    if (_paused) {
        reset();
        return;
    }
#if defined(OS_WINDOWS)
    SetThreadAffinityMask(::GetCurrentThread(), _oldmask);
#endif
    _timer = Time::MicrosecondsToMilliseconds<D32>(
                 ApplicationTimer::getInstance().getElapsedTime(true)) -
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
    ApplicationTimer::getInstance().addTimer(this);
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