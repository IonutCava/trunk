#include "core.h"

#include "Headers/Task.h"
Task::~Task(){
    if(_end != true){
        ERROR_FN(Locale::get("TASK_DELETE_ACTIVE"));
    }
    while(!_done){/*wait*/}
}

void Task::startTask(){
    DIVIDE_ASSERT(_tp != nullptr, "Task error: ThreadPool pointer is invalid!");
    if(!_tp->schedule(DELEGATE_BIND(&Task::run, this))){
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
        while(true){
            if(_end) break;

            while(_paused) 
                boost::this_thread::sleep_for(boost::chrono::milliseconds(_tickInterval));

            if(_tickInterval > 0) 
                boost::this_thread::sleep_for(boost::chrono::milliseconds(_tickInterval));

            _callback();

            if(_numberOfTicks > 0){
                _numberOfTicks--;
            }else if(_numberOfTicks == 0){
                _end = true;
            }else{
                //_numberOfTicks == -1, run forever
            }
        }
    //}catch(const boost::thread_interrupted&){}

    D_PRINT_FN(Locale::get("TASK_DELETE_THREAD"), boost::this_thread::get_id());

    _completionSignal(getGUID());
    _done = true;
}