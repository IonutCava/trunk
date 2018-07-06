#include "config.h"

#include "Headers/ProfileTimer.h"
#include "Headers/ApplicationTimer.h"
#include "Core/Headers/Console.h"

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
    : _timer(0UL),
      _timerAverage(0UL),
      _timerCounter(0),
      _globalIndex(0),
      _parent(Config::Profile::MAX_PROFILE_TIMERS + 1)
{
}

ProfileTimer::~ProfileTimer()
{
}

void ProfileTimer::start() {
    if (Config::Profile::ENABLE_FUNCTION_PROFILING) {
        _timer = ElapsedMicroseconds(true);
    }
}

void ProfileTimer::stop() {
    if (Config::Profile::ENABLE_FUNCTION_PROFILING) {
        _timerAverage += ElapsedMicroseconds(true) - _timer;
        _timerCounter++;
    }
}

void ProfileTimer::reset() {
    _timerAverage = 0;
    _timerCounter = 0;
}

void ProfileTimer::addChildTimer(ProfileTimer& child) {
    U32 childID = child._globalIndex;
    // must not have a parent
    assert(child._parent > Config::Profile::MAX_PROFILE_TIMERS);
    // must be unique child
    assert(std::find_if(std::cbegin(_children),
                        std::cend(_children),
                        [childID](U32 entry) {
                            return entry == childID;
                        }) == std::cend(_children));

    _children.push_back(childID);
    child._parent = _globalIndex;
}

stringImpl ProfileTimer::print(U32 level) const {
    if (Config::Profile::ENABLE_FUNCTION_PROFILING) {
        stringImpl ret(Util::StringFormat("[ %s ] : [ %5.3f ms]",
                                          _name.c_str(),
                                          MicrosecondsToMilliseconds<F32>(get())));
        for (U32 child : _children) {
            ret.append("\n    " + g_profileTimers[child].print(level + 1));
        }

        for (U32 i = 0; i < level; ++i) {
            ret.insert(0, "    ");
        }

        return ret;
    }

    return "";
}

U64 ProfileTimer::overhead() {
    ProfileTimer test;
    test.start();
    test.stop();
    return test.get();
}

stringImpl ProfileTimer::printAll() {
    stringImpl ret(Util::StringFormat("Profiler overhead: [%d us]\n", overhead()));

    for (ProfileTimer& entry : g_profileTimers) {
        if (!g_profileTimersState[entry._globalIndex] ||
                entry._parent < Config::Profile::MAX_PROFILE_TIMERS ||
                    entry._timerCounter == 0) {
            continue;
        }
        ret.append(entry.print());
        ret.append("\n");
        entry.reset();
    }

    return ret;
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

    DIVIDE_UNEXPECTED_CALL("Reached max profile timer count!");
    return g_profileTimers[0];
}

void ProfileTimer::removeTimer(ProfileTimer& timer) {
    g_profileTimersState[timer._globalIndex] = false;
}

};  // namespace Time
};  // namespace Divide