#include "Headers/Task.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

#include <chrono>
#include <thread>

namespace Divide {
    Task::Task(boost::threadpool::pool* tp, U64 tickInterval, I32 numberOfTicks, const DELEGATE_CBK<>& f) : GUIDWrapper(),
        _tp(tp),
        _tickInterval(tickInterval),
        _numberOfTicks(numberOfTicks),
        _callback(f),
        _end(false),
        _paused(false),
        _done(false)
    {
    }

    Task::~Task()
    {
        if (_end != true) {
            ERROR_FN(Locale::get("TASK_DELETE_ACTIVE"));
            stopTask();
        }
        while (!_done) {
        }
    }

    void Task::startTask(){
        DIVIDE_ASSERT(_tp != nullptr, "Task error: ThreadPool pointer is invalid!");
        if (!_tp->schedule(DELEGATE_BIND(&Task::run, this))) {
            ERROR_FN(Locale::get("TASK_SCHEDULE_FAIL"));
        }
    }

    void Task::stopTask(){
        _end = true;
    }

    void Task::pauseTask(bool state){
        _paused = state;
    }

    void Task::run(){
        D_PRINT_FN(Locale::get("TASK_START_THREAD"), std::this_thread::get_id());
        U64 lastCallTime = GETTIME();

        // 0 == run forever
        if (_numberOfTicks == 0){
            _numberOfTicks = -1;
        }

        _done = false;

        while (true) {
            if (_numberOfTicks == 0) {
                _end = true;
            }

            while ((_paused && !_end) || 
                   (Application::getInstance().mainLoopPaused() && 
                    !Application::getInstance().ShutdownRequested())) {
                continue;
            }
            if (_end || Application::getInstance().ShutdownRequested()) {
                break;
            }

            U64 nowTime = GETUSTIME(true);
            if (nowTime > (lastCallTime + _tickInterval)) {
                _callback();
                lastCallTime = GETUSTIME(true);
            }

            if (_numberOfTicks > 0) {
                _numberOfTicks--;
            }
        }

        D_PRINT_FN(Locale::get("TASK_DELETE_THREAD"), std::this_thread::get_id());

        _completionSignal(getGUID());
        _done = true;
    }
};