/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _TASKS_H_
#define _TASKS_H_

#include "SharedMutex.h"
#include "Utility/Headers/GUIDWrapper.h"

namespace Divide {

enum CallbackParam {
    TYPE_INTEGER,
    TYPE_MEDIUM_INTEGER,
    TYPE_SMALL_INTEGER,
    TYPE_UNSIGNED_INTEGER,
    TYPE_MEDIUM_UNSIGNED_INTEGER,
    TYPE_SMALL_UNSIGNED_INTEGER,
    TYPE_STRING,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_CHAR
};
/**
 *@brief Using std::atomic for thread-shared data to avoid locking
 */
class Task : public GUIDWrapper, public std::enable_shared_from_this<Task> {
    typedef boost::signals2::signal<void(U64)> SendCompleted;

   public:
    /**
     * @brief Creates a new Task that runs in a separate thread
     * @param tickInterval The delay (in microseconds) between each callback
     * @param numberOfTicks The number of times to call the callback function
     * before the Task is deleted. 0 = run forever
     * @param f The callback function
     */
    Task(boost::threadpool::pool* tp, U64 tickInterval, I32 numberOfTicks,
         const DELEGATE_CBK<>& f);
    ~Task();

    void updateTickInterval(U64 tickInterval) { _tickInterval = tickInterval; }
    void updateTickCounter(I32 numberOfTicks) {
        _numberOfTicks = numberOfTicks;
    }
    void startTask();
    void stopTask();
    void pauseTask(bool state);

    inline bool isFinished() const { return _done; }

    typedef SendCompleted::slot_type SendCompletedSlot;
    boost::signals2::connection connect(const SendCompletedSlot& slot) {
        return _completionSignal.connect(slot);
    }

   private:
    mutable SharedLock _lock;
    mutable std::atomic<U64> _tickInterval;
    mutable std::atomic<I32> _numberOfTicks;
    mutable std::atomic<bool> _end;
    mutable std::atomic<bool> _paused;
    mutable std::atomic<bool> _done;
    DELEGATE_CBK<> _callback;
    boost::threadpool::pool* _tp;
    SendCompleted _completionSignal;

   protected:
    void run();
};

typedef std::shared_ptr<Task> Task_ptr;

};  // namespace Divide

#endif
