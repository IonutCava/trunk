#include "Headers/Task.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

#include <chrono>
#include <thread>

namespace Divide {
namespace {
    U64 g_sleepThresholdMS = 5UL;
};

U64 Task::_currentTime = 0UL;

Task::Task(ThreadPool& tp, U64 tickInterval, I32 numberOfTicks,
           const DELEGATE_CBK<>& f)
    : GUIDWrapper(),
      _tp(tp),
      _tickInterval(tickInterval),
      _callback(f)
{
    _numberOfTicks = numberOfTicks;
    _end = false;
    _paused = false;
    _done = false;
}

Task::~Task()
{
    if (_end != true) {
        Console::errorfn(Locale::get(_ID("TASK_DELETE_ACTIVE")));
        stopTask();
    }

    WAIT_FOR_CONDITION(_done || _tp.active() == 0);
}

void Task::updateTickInterval(U64 tickInterval) {
    _tickInterval = std::max(tickInterval, Time::MillisecondsToMicroseconds(1));
}

void Task::updateTickCounter(I32 numberOfTicks) {
    _numberOfTicks = numberOfTicks;
}

void Task::startTask(TaskPriority priority) {
    if (!_tp.schedule(PoolTask(to_uint(priority), DELEGATE_BIND(&Task::run, this))))
    {
        Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")));
    }
}

void Task::stopTask() { 
    _end = true;
}

void Task::pauseTask(bool state) {
    _paused = state;
}

void Task::run() {
    Console::d_printfn(Locale::get(_ID("TASK_START_THREAD")), getGUID(), std::this_thread::get_id());

    U64 lastCallTime = _currentTime;
    // 0 == run forever
    if (_numberOfTicks == 0) {
        _numberOfTicks = -1;
    }

    Application& app = Application::getInstance();

    _done = !_callback;
    while (true) {
        if (_numberOfTicks == 0) {
            _end = true;
        }

        if (_end || app.ShutdownRequested()) {
            break;
        }

        if (_paused) {
            continue;
        }

        if (_currentTime >= (lastCallTime + _tickInterval)) {
            _callback();
            if (_numberOfTicks > 0) {
                _numberOfTicks--;
            }
            lastCallTime = _currentTime;
            // Sleep does not guarantee a maximum wait time, just a minimum, so g_sleepThresholdMS should be enough margin
            if (_tickInterval > Time::MillisecondsToMicroseconds(g_sleepThresholdMS)) {
                // If the tick interval is over 'g_sleepThreshold', sleep for at least half of the duration
                std::this_thread::sleep_for(std::chrono::microseconds(_tickInterval / 2));
            }
        }

    }

    Console::d_printfn(Locale::get(_ID("TASK_DELETE_THREAD")), getGUID(), std::this_thread::get_id());

    if (_onCompletionCbk) {
        _onCompletionCbk(getGUID());
    }

    _done = true;
}
};