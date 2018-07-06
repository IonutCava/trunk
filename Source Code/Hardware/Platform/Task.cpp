#include "Headers/Task.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Utility/Headers/Localization.h"
#include <boost/thread.hpp>

Task::~Task(){
    if (_end != true) {
        ERROR_FN(Locale::get("TASK_DELETE_ACTIVE"));
    }
    while (!_done) {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
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
    //try	{ //< threadpool does not support exceptions
        D_PRINT_FN(Locale::get("TASK_START_THREAD"), boost::this_thread::get_id());
        while(true) {
            while (_paused) {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(_tickInterval));
            }
            if (_end || Application::getInstance().ShutdownRequested()) {
                break;
            }

            if (_tickInterval > 0) {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(_tickInterval));
            }

            _callback();

            if (_numberOfTicks > 0) {
                _numberOfTicks--;
            } else if(_numberOfTicks == 0) {
                _end = true;
            }
        }
    //}catch(const boost::thread_interrupted&){}

    D_PRINT_FN(Locale::get("TASK_DELETE_THREAD"), boost::this_thread::get_id());

    _completionSignal(getGUID());
    _done = true;
}