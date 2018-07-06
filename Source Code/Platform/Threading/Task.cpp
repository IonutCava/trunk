#include "Headers/Task.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

#include <chrono>
#include <thread>

namespace Divide {
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
        Console::errorfn(Locale::get("TASK_DELETE_ACTIVE"));
        stopTask();
    }

    WAIT_FOR_CONDITION(_done);
}

void Task::startTask(TaskPriority priority) {
    if (!_tp.schedule(PoolTask(to_uint(priority), DELEGATE_BIND(&Task::run, this)))) {
        Console::errorfn(Locale::get("TASK_SCHEDULE_FAIL"));
    }
}

void Task::stopTask() { 
    _end = true;
}

void Task::pauseTask(bool state) {
    _paused = state;
}

void Task::run() {
    Console::d_printfn(Locale::get("TASK_START_THREAD"), getGUID(), std::this_thread::get_id());

    U64 lastCallTime = Time::ElapsedMicroseconds(true);
    // 0 == run forever
    if (_numberOfTicks == 0) {
        _numberOfTicks = -1;
    }

    Application& app = Application::getInstance();

    _done = false;
    while (true) {
        if (_numberOfTicks == 0) {
            _end = true;
        }

        while ((_paused && !_end) ||
            (app.mainLoopPaused() && !app.ShutdownRequested())) {
            continue;
        }

        if (_end || app.ShutdownRequested()) {
            break;
        }

        U64 nowTime = Time::ElapsedMicroseconds(true);
        if (nowTime >= (lastCallTime + _tickInterval)) {
            _callback();
            if (_numberOfTicks > 0) {
                _numberOfTicks--;
            }
            lastCallTime = nowTime;
        }

   }

    Console::d_printfn(Locale::get("TASK_DELETE_THREAD"), getGUID(), std::this_thread::get_id());

    if (_onCompletionCbk) {
        _onCompletionCbk(getGUID());
    }

    _done = true;
}
};