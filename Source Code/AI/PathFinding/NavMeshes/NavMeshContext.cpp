#include "Headers/NavMeshContext.h"

#include "Core/Headers/Console.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
namespace AI {
namespace Navigation {
void rcContextDivide::doLog(const rcLogCategory category, const char* msg,
                            const I32 len) {
    switch (category) {
        default:
        case RC_LOG_PROGRESS:
            Console::printfn(Locale::get(_ID("RECAST_CTX_LOG_PROGRESS")), msg);
            break;
        case RC_LOG_WARNING:
            Console::printfn(Locale::get(_ID("RECAST_CTX_LOG_WARNING")), msg);
            break;
        case RC_LOG_ERROR:
            Console::errorfn(Locale::get(_ID("RECAST_CTX_LOG_ERROR")), msg);
            break;
    }
}

void rcContextDivide::doResetTimers() {
    for (I32 i = 0; i < RC_MAX_TIMERS; ++i) _accTime[i] = -1;
}

void rcContextDivide::doStartTimer(const rcTimerLabel label) {
    _startTime[label] = Time::ElapsedMilliseconds(true);
}

void rcContextDivide::doStopTimer(const rcTimerLabel label) {
    const D64 deltaTime = Time::ElapsedMilliseconds(true) - _startTime[label];
    if (_accTime[label] == -1) {
        _accTime[label] = to_I32(deltaTime);
    } else {
        _accTime[label] += to_I32(deltaTime);
    }
}

I32 rcContextDivide::doGetAccumulatedTime(const rcTimerLabel label) const {
    return _accTime[label];
}
};
};  // namespace AI
};  // namespace Divide
