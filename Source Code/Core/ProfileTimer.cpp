#include "Headers/ProfileTimer.h"
#include "Headers/ApplicationTimer.h"
#include "Headers/Console.h"

namespace Divide {
namespace Time {

namespace {
    std::array<ProfileTimer, Config::Profile::MAX_PROFILE_TIMERS> g_profileTimers;
    std::array<bool, Config::Profile::MAX_PROFILE_TIMERS> g_profileTimersState;

    bool g_timersInit = false;
};

ScopedTimer::ScopedTimer(ProfileTimer& timer) 
    : _timer(timer)
{
    _timer.start();
}

ScopedTimer::~ScopedTimer()
{
    _timer.stop();
}

ProfileTimer::ProfileTimer()
    : _timer(0.0),
      _timerAverage(0.0),
      _timerCounter(0),
      _globalIndex(0),
      _paused(false)
{
}

ProfileTimer::~ProfileTimer()
{
}

void ProfileTimer::start() {
#if defined(_DEBUG) || defined(_PROFILE)
    _timer = ElapsedMilliseconds(true);
#endif
}

void ProfileTimer::stop() {
#if defined(_DEBUG) || defined(_PROFILE)
    if (!_paused) {
        _timerAverage += ElapsedMilliseconds(true) - _timer;
        _timerCounter++;
    }
#endif
}

void ProfileTimer::pause(const bool state) {
    _paused = state;
}

void ProfileTimer::reset() {
    _timerAverage = 0;
    _timerCounter = 0;
    _paused = false;
}

void ProfileTimer::print() const {
#if defined(_DEBUG) || defined(_PROFILE)
    Console::printfn("[ %s ] : [ %5.3f ms]", _name.c_str(), _timerAverage / _timerCounter);
#endif
}

void ProfileTimer::printAll() {
    for (ProfileTimer& entry : g_profileTimers) {
        if (!g_profileTimersState[entry._globalIndex] || entry._paused) {
            continue;
        }
        entry.print();
        entry.reset();
    }
}

ProfileTimer& ProfileTimer::getNewTimer(const char* timerName) {
    if (!g_timersInit) {
        g_profileTimersState.fill(false);

        U32 index = 0;
        for (ProfileTimer& entry : g_profileTimers) {
            entry._globalIndex = index++;
        }
        g_timersInit = true;
    }

    for (ProfileTimer& entry : g_profileTimers) {
        if (!g_profileTimersState[entry._globalIndex]) {
            entry.reset();
            entry._name = timerName;
            g_profileTimersState[entry._globalIndex] = true;
            return entry;
        }
    }

    assert(false && "Reached max profile timer count!");
    return g_profileTimers[0];
}

void ProfileTimer::removeTimer(ProfileTimer& timer) {
    g_profileTimersState[timer._globalIndex] = false;
}

};  // namespace Time
};  // namespace Divide